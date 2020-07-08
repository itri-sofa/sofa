/*
 * Copyright (c) 2015-2025 Industrial Technology Research Institute.
 *
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#include <linux/kthread.h>
#include <linux/bio.h>
#include <linux/fs.h>
#include "config/lfsm_setting.h"
#include "config/lfsm_feature_setting.h"
#include "config/lfsm_cfg.h"
#include "common/common.h"
#include "common/mem_manager.h"
#include "common/rmutex.h"
#include "io_manager/mqbiobox.h"
#include "io_manager/biocbh.h"
#include "nextprepare.h"
#include "diskgrp.h"
#include "system_metadata/Dedicated_map.h"
#include "lfsm_core.h"
#include "io_manager/io_common.h"
#include "devio.h"
#include "batchread.h"
#include "special_cmd.h"

void sbrm_end_bio_generic_read(struct bio *bio)
{
    struct bio_container *entry;
    lfsm_dev_t *td;

    entry = bio->bi_private;
    td = entry->lfsm_dev;

    if (bio->bi_status != BLK_STS_OK) {
        entry->io_queue_fin = blk_status_to_errno(bio->bi_status);
//        printk("%s: Get a bio %p bioc %p err message %d\n",__FUNCTION__,entry, bio, err);
    }

    if (atomic_dec_and_test(&entry->cn_wait)) { // all pages have result
        if (entry->io_queue_fin == 0) {
            entry->io_queue_fin = 1;
        }
        biocbh_io_done(&td->biocbh, entry, true);
    }
}
