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
#include <linux/types.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/wait.h>
#include "config/lfsm_setting.h"
#include "config/lfsm_feature_setting.h"
#include "config/lfsm_cfg.h"
#include "common/common.h"
#include "common/rmutex.h"
#include "nextprepare.h"
#include "io_manager/mqbiobox.h"
#include "io_manager/biocbh.h"
#include "diskgrp.h"
#include "system_metadata/Dedicated_map.h"
#include "lfsm_core.h"
#include "EU_create.h"

int32_t SNextPrepare_alloc(SNextPrepare_t * pNext, int32_t eu_usage)
{
    struct HListGC *h;

    h = gc[get_next_disk()];
    pNext->eu_main = FreeList_delete(h, eu_usage, false);
    h = gc[get_next_disk()];
    pNext->eu_backup = FreeList_delete(h, eu_usage, false);

    atomic_set(&pNext->fg_next_dirty, 0);
    return 0;
}

void SNextPrepare_renew(SNextPrepare_t * pNext, int32_t eu_usage)
{
    struct HListGC *h;

    if (atomic_read(&pNext->fg_next_dirty) == 1) {
        if (FreeList_insert_withlock(pNext->eu_main) < 0) {
            LFSM_ASSERT(0);
        }

        if (FreeList_insert_withlock(pNext->eu_backup) < 0) {
            LFSM_ASSERT(0);
        }

        h = gc[get_next_disk()];
        LFSM_RMUTEX_LOCK(&h->whole_lock);
        pNext->eu_main = FreeList_delete_with_log(&lfsmdev, h, eu_usage, false);
        LFSM_RMUTEX_UNLOCK(&h->whole_lock);

        h = gc[get_next_disk()];
        LFSM_RMUTEX_LOCK(&h->whole_lock);
        pNext->eu_backup =
            FreeList_delete_with_log(&lfsmdev, h, eu_usage, false);
        LFSM_RMUTEX_UNLOCK(&h->whole_lock);

        atomic_set(&pNext->fg_next_dirty, 0);
        LOGINFO("done with fg= %d\n", atomic_read(&pNext->fg_next_dirty));
    }
}

void SNextPrepare_get_new(SNextPrepare_t * pnext,
                          struct EUProperty **pret_peu_main,
                          struct EUProperty **pret_peu_backup, int32_t eu_usage)
{
    // below code is kind of damage recovery after assert rised
    if (atomic_read(&pnext->fg_next_dirty) == 1) {
        LFSM_ASSERT2(0, "ERROR: next is dirty eu{%llu, %llu}\n",
                     pnext->eu_main->start_pos, pnext->eu_backup[0]->start_pos);
        EU_erase(pnext->eu_main);
        EU_erase(pnext->eu_backup);
        EU_param_init(pnext->eu_main, eu_usage, 0);
        EU_param_init(pnext->eu_backup, eu_usage, 0);
    }
    // the actual code should be written in the function
    *pret_peu_main = pnext->eu_main;
    *pret_peu_backup = pnext->eu_backup;
}

void SNextPrepare_rollback(SNextPrepare_t * pnext, struct EUProperty *peu_main,
                           struct EUProperty *peu_backup)
{
    pnext->eu_main = peu_main;
    pnext->eu_backup = peu_backup;
    return;
}

void SNextPrepare_put_old(SNextPrepare_t * pnext, struct EUProperty *peu_main,
                          struct EUProperty *peu_backup)
{
    pnext->eu_main = peu_main;
    pnext->eu_backup = peu_backup;
    atomic_set(&pnext->fg_next_dirty, 1);
}
