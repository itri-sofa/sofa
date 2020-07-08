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
#include "common/mem_manager.h"
#include "common/rmutex.h"
#include "io_manager/mqbiobox.h"
#include "io_manager/biocbh.h"
#include "nextprepare.h"
#include "diskgrp.h"
#include "system_metadata/Dedicated_map.h"
#include "lfsm_core.h"
#include "lfsm_blk_dev_private.h"
#include "io_manager/io_common.h"
#include "ioctl.h"

struct block_device_operations sftl_fops = {
    .owner = THIS_MODULE,
    .ioctl = lfsm_ioctl,
};

int32_t create_disk(lfsm_dev_t * mydev, sector64 totalPages)
{
    struct gendisk *disk;
    int32_t err;

    do {
        /* Setup the block device callback functions */
        //spin_lock_init(&mydev->queue_lock);
        if (IS_ERR_OR_NULL(disk = alloc_disk(lfsmCfg.max_disk_partition))) {
            err = -ENOMEM;
            break;
        }

        mydev->disk = disk;
        if (IS_ERR_OR_NULL(disk->queue = blk_alloc_queue(GFP_KERNEL))) {
            err = -ENOMEM;
            break;
        }

        blk_queue_io_min(disk->queue, FLASH_PAGE_SIZE);
        blk_queue_make_request(disk->queue, request_enqueue);
        disk->queue->nr_requests = 20480;   // 10240
        disk->queue->limits.max_sectors = 126;  // For MAX_FLASH_PAGE_NUM
        disk->queue->backing_dev_info->ra_pages = (34 * 1024) / PAGE_SIZE;  //limit read_ahead for MAX_FLASH_PAGE_NUM
        disk->queue->queuedata = disk;
        disk->major = mydev->lfsm_major;
        disk->first_minor = 0;
//    disk->minors = 1;
        disk->fops = &sftl_fops;
        disk->private_data = &lfsmdev;
        disk->flags |= GENHD_FL_SUPPRESS_PARTITION_INFO;

        sprintf(disk->disk_name, blk_devName);

        set_capacity(disk,
                     (sector_t) (totalPages << SECTORS_PER_FLASH_PAGE_ORDER));

        //20151109 Lego: enable discard, in lfsm discard = trim
        //queue_flag_set(QUEUE_FLAG_DISCARD, disk->queue);
        set_bit(QUEUE_FLAG_DISCARD, &disk->queue->queue_flags);
        disk->queue->limits.discard_granularity = FLASH_PAGE_SIZE;
        disk->queue->limits.discard_alignment = 0;
        disk->queue->limits.max_discard_sectors = (INT_MAX / SECTOR_SIZE);  // 2G

        LOGINFO("lfsmdev.logical_capacity:%llu pages\n", (sector64) totalPages);
        add_disk(disk);

        err = 0;
    } while (0);

    return err;
}

void remove_disk(lfsm_dev_t * mydev)
{
    if (IS_ERR_OR_NULL(mydev->disk)) {
        return;
    }

    del_gendisk(lfsmdev.disk);
    blk_cleanup_queue(lfsmdev.disk->queue);
    put_disk(lfsmdev.disk);
}

int32_t init_blk_device(int8_t * blk_dev_n)
{
    int32_t blk_majorN;

    //    ------------------------------------------
    //    Register a new block dev named "lfsm" in the kernel
    //    ------------------------------------------
    blk_majorN = 0;

    memset(blk_devName, '\0', BLK_DEV_NAME_LEN);

    strcpy(blk_devName, blk_dev_n);
    LOGINFO("Registering block device with major number........");
    blk_majorN = register_blkdev(lfsmdev.lfsm_major, "lfsm");
    printk(" %d name %s DONE\n", blk_majorN, blk_devName);

    if (blk_majorN <= 0) {
        blk_majorN = -EBUSY;
        LOGERR("fail register blk dev %d\n", blk_majorN);
    }

    return blk_majorN;
}

void rm_blk_device(int32_t blk_majorN)
{
    if (blk_majorN > 0) {
        unregister_blkdev(blk_majorN, blk_devName);
    }
}
