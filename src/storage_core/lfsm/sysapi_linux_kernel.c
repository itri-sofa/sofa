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
#include <linux/version.h>
#include <linux/types.h>        // atomic_t
#include <linux/blkdev.h>
#include "config/lfsm_setting.h"
#include "config/lfsm_feature_setting.h"
#include "config/lfsm_cfg.h"
#include "common/common.h"
#include "sysapi_linux_kernel.h"

sector64 sysapi_get_timer_ns(void)
{
    struct timespec tm;

    getnstimeofday(&tm);
    return (sector64) tm.tv_sec * 1000000000 + tm.tv_nsec;
}

sector64 sysapi_get_timer_us(void)
{
    struct timeval tm;
    do_gettimeofday(&tm);
    return (sector64) tm.tv_sec * 1000000 + tm.tv_usec;
}

sysapi_bdev *sysapi_bdev_open(int8_t * nm_bdev)
{
    sysapi_bdev *bdev;
    bdev = blkdev_get_by_path(nm_bdev, O_RDWR, NULL);
    return bdev;
}

int64_t sysapi_bdev_getsize(sysapi_bdev * bdev)
{
    return get_capacity(bdev->bd_disk);
}

void sysapi_bdev_close(sysapi_bdev * bdev)
{
    blkdev_put(bdev, O_RDWR);
}

int32_t sysapi_bdev_name2id(int8_t * nm_org)
{
    int8_t *nm_dev, *nm;
    int32_t disk_id, i;

    int32_t len;

    nm = nm_org;
    if (strncmp(nm, "/dev/", 5) == 0) {
        nm += 5;
    }

    if (IS_ERR_OR_NULL(nm) || strcmp(nm, "lfsm") == 0) {
        LOGINFO
            ("sysapi_bdev_name2id return -EMODEV because nm=null or nm=lfsm \n");
        return -ENODEV;
    }

    len = strlen(nm);
    disk_id = -1;

    if (len <= 3 || (len <= 4 && (48 > nm[3] || nm[3] > 57))) {

        LOGINFO("sysapi_bdev_name2id= %s \n", nm);

        if (IS_ERR_OR_NULL(nm_dev = strstr(nm, "sd"))) {/** search the word "sd"  **/
            return -ENODEV;
        }

        for (i = 0; i < MAX_SUPPORT_SSD; i++) {
            if (strcmp(lfsmCfg.ssd_path[i], &nm_dev[strlen("sd")]) == 0) {

                disk_id = i;
                break;
            }
        }

    } else {
        for (i = 0; i < MAX_SUPPORT_SSD; i++) {
            if (strcmp(lfsmCfg.ssd_path[i], nm) == 0) {
                disk_id = i;
                break;
            }
        }
    }

    return disk_id;
}

int8_t *sysapi_bdev_id2name(int32_t id_disk, int8_t * nm)
{
    uint32_t len;
    if (id_disk < 0 || id_disk >= MAXCN_HOTSWAP_SPARE_DISK) {
        return NULL;
    }

    if (IS_ERR_OR_NULL(lfsmCfg.ssd_path[id_disk])) {    // search the word "sd"
        return NULL;
    }

    len = strlen(lfsmCfg.ssd_path[id_disk]);

    if (len <= 1
        || (len <= 2
            && (48 > *(lfsmCfg.ssd_path[1]) || *(lfsmCfg.ssd_path[1]) > 57))) {
        sprintf(nm, "/dev/sd%s", lfsmCfg.ssd_path[id_disk]);
    } else {
        // LOGINFO("len ==2 || >2 : %s \n", lfsmCfg.ssd_path[id_disk]);
        sprintf(nm, "/dev/%s", lfsmCfg.ssd_path[id_disk]);
    }

    return nm;
}
