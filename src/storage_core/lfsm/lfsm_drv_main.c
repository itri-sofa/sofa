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
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/wait.h>
#include "config/lfsm_setting.h"
#include "config/lfsm_feature_setting.h"
#include "config/lfsm_cfg.h"
#include "common/common.h"
#include "common/rmutex.h"
#include "io_manager/mqbiobox.h"
#include "io_manager/biocbh.h"
#include "nextprepare.h"
#include "diskgrp.h"
#include "system_metadata/Dedicated_map.h"
#include "lfsm_core.h"
#include "sysapi_linux_kernel.h"
#include "io_manager/io_common.h"
#include "lfsm_drv_main_private.h"

#include "perf_monitor/calculate_iops.h"

int8_t _get_sparedisks(int8_t * spare_msg)
{
    int8_t spare_reply[SPARE_OUTSIZE];
    int8_t nm_dev[32];
    struct shotswap *phs;
    int32_t i, len;

    len = 0;
    phs = &lfsmdev.hotswap;
    len += sprintf(spare_reply + len, "%d,", phs->cn_spare_disk);
    for (i = 0; i < phs->cn_spare_disk; i++) {
        if (IS_ERR_OR_NULL
            (lookup_bdev
             (sysapi_bdev_id2name(phs->ar_id_spare_disk[i], nm_dev)))) {
            len +=
                sprintf(spare_reply + len, "%s,SizeUnknow,removed,",
                        sysapi_bdev_id2name(phs->ar_id_spare_disk[i],
                                            nm_dev) + 5);
        } else if (phs->ar_actived[i] == 0) {
            len += sprintf(spare_reply + len, "%s,SizeUnknow,failed,",
                           sysapi_bdev_id2name(phs->ar_id_spare_disk[i],
                                               nm_dev) + 5);
        } else {
            len += sprintf(spare_reply + len, "%s,SizeUnknow,normal,",
                           sysapi_bdev_id2name(phs->ar_id_spare_disk[i],
                                               nm_dev) + 5);
        }
    }

    if (0 == len || 0 == phs->cn_spare_disk) {
        sprintf(spare_reply, "None,");
    }

    memcpy(spare_msg, spare_reply, sizeof(spare_reply));

    return 0;
}

int8_t _get_groupdisks(uint32_t groupId, int8_t * gdisk_msg)
{
    int8_t gdisk_reply[GROUPDISK_OUTSIZE];
    struct block_device *bdev;
    int8_t *devName;
    uint64_t diskSize, groupPhyCap, groupCap, groupDiskUsed;
    int32_t i, j, len, diskIndex;
    struct HListGC *h;

    groupPhyCap = 0;
    groupDiskUsed = 0;
    len = 0;
    len += sprintf(gdisk_reply + len, "%d,", lfsmdev.hpeutab.cn_disk);
    for (i = 0; i < lfsmdev.cn_pgroup; i++) {
        if (groupId != i) {
            continue;
        }

        if (lfsmdev.ar_pgroup[i].state == epg_good) {
            for (j = 0; j < lfsmdev.hpeutab.cn_disk; j++) {
                diskIndex = HPEU_EU2HPEU_OFF(j, i);
                devName = grp_ssds.ar_drives[diskIndex].nm_dev;
                diskSize = grp_ssds.ar_drives[diskIndex].disk_size;
                groupPhyCap += diskSize << SECTOR_ORDER;
                h = gc[diskIndex];
                groupDiskUsed +=
                    (diskSize << SECTOR_ORDER) -
                    (h->free_list_number << EU_ORDER);
                //LOGINFO("h->free_list_number=%llu\n", h->free_list_number);
                len += sprintf(gdisk_reply + len, "%s,%llu,normal,",
                               devName + 5, diskSize << SECTOR_ORDER);
            }
        } else {
            for (j = 0; j < lfsmdev.hpeutab.cn_disk; j++) {
                diskIndex = HPEU_EU2HPEU_OFF(j, i);
                devName = grp_ssds.ar_drives[diskIndex].nm_dev;
                diskSize = grp_ssds.ar_drives[diskIndex].disk_size;
                groupPhyCap += diskSize << SECTOR_ORDER;
                h = gc[diskIndex];
                groupDiskUsed +=
                    (diskSize << SECTOR_ORDER) -
                    (h->free_list_number << EU_ORDER);
                bdev = lookup_bdev(grp_ssds.ar_drives[diskIndex].nm_dev);
                if (IS_ERR_OR_NULL(bdev)) {
                    len += sprintf(gdisk_reply + len, "%s,%llu,removed,",
                                   devName + 5, diskSize << SECTOR_ORDER);
                } else if (diskgrp_isfaildrive(&grp_ssds, diskIndex)) {
                    len += sprintf(gdisk_reply + len, "%s,%llu,failed,",
                                   devName + 5, diskSize << SECTOR_ORDER);
                } else if (grp_ssds.ar_subdev[diskIndex].load_from_prev ==
                           sys_ready) {
                    len +=
                        sprintf(gdisk_reply + len, "%s,%llu,rebuilding,",
                                devName + 5, diskSize << SECTOR_ORDER);
                } else {
                    len += sprintf(gdisk_reply + len, "%s,%llu,read-only,",
                                   devName + 5, diskSize << SECTOR_ORDER);
                }
            }
        }
    }

    groupCap = lfsmdev.logical_capacity / lfsmdev.cn_pgroup;
    groupCap = groupCap << FLASH_PAGE_ORDER;
    len +=
        sprintf(gdisk_reply + len, "%llu,%llu,%llu,", groupPhyCap, groupCap,
                groupDiskUsed);

    if (0 == len || 0 == lfsmdev.cn_pgroup) {
        sprintf(gdisk_reply, "None,");
    }

    memcpy(gdisk_msg, gdisk_reply, sizeof(gdisk_reply));

    return 0;
}

uint32_t _get_groups(void)
{
    uint32_t cnt_pgroup;

    cnt_pgroup = lfsmCfg.cnt_pgroup;

    return cnt_pgroup;
}

uint64_t _get_physicalCap(void)
{
    uint64_t cap_bytes;

    //cap_bytes = lfsmdev.physical_capacity;
    cap_bytes = grp_ssds.capacity << SECTOR_ORDER;

    return cap_bytes;
}

int32_t lfsm_set_config(uint32_t index, uint32_t val, int8_t * strVal,
                        int8_t * sub_setting, uint32_t set_len)
{
    return set_lfsm_config(index, val, strVal, sub_setting, set_len);
}

EXPORT_SYMBOL(lfsm_set_config);

int32_t lfsm_init_driver(void)
{
    int32_t ret;

    ret = 0;
    if (!lfsm_init_state) {
        ret = lfsm_init(&lfsmCfg, 0, 0, 0);
        if (!ret) {
            lfsm_init_state = 1;
        }
    } else {
        LOGINFO("Driver has been initialized\n");
    }

    return ret;
}

EXPORT_SYMBOL(lfsm_init_driver);

int32_t lfsm_submit_bio(uint64_t sect_s, uint32_t sect_len, struct bio *mybio)
{
    int32_t ret;

    ret = 0;

    mybio->bi_iter.bi_sector = sect_s;
    mybio->bi_iter.bi_size = sect_len << SECTOR_ORDER;

    request_enqueue(NULL, mybio);

    return ret;
}

EXPORT_SYMBOL(lfsm_submit_bio);

int32_t lfsm_submit_MD_request(uint64_t lpn_src, uint32_t lpn_len,
                               uint64_t lpn_dest)
{
    int32_t i, ret;
    struct D_BMT_E dbmta;
    uint64_t pbno;

    ret = 0;

    for (i = 0; i < lpn_len; i++) {
        //LOGINFO("copying metadata from lpn_src %llu to lpn_dest %llu\n", lpn_src+i, lpn_dest+i);
        if ((ret = bmt_lookup(&lfsmdev, lfsmdev.ltop_bmt, lpn_src + i, false, 1, &dbmta)) < 0) {    //not here?
            LOGERR("Fail to bmt lookup for lpn: %lld, ret=%d\n", lpn_src + i,
                   ret);
            break;
        }

        if ((pbno = (uint64_t) dbmta.pbno) == PBNO_SPECIAL_NEVERWRITE) {
            ret = PPDM_BMT_cache_remove_pin(lfsmdev.ltop_bmt, lpn_src + i, 1);
            continue;
        }

        PPDM_BMT_cache_insert_one_pending(&lfsmdev, lpn_dest + i, lpn_src + i,
                                          1, false, 0, true);
    }

    //submit_copy_request(lpn_src, lpn_dest, SECTORS_PAGE);
    return ret;
}

EXPORT_SYMBOL(lfsm_submit_MD_request);

//NOTE: capacity in bytes
uint64_t lfsm_get_disk_capacity(void)
{
    uint64_t cap_bytes;

    cap_bytes = lfsmdev.logical_capacity << FLASH_PAGE_ORDER;
    //cap_bytes = get_lfsm_capacity();

    return cap_bytes;
}

EXPORT_SYMBOL(lfsm_get_disk_capacity);

uint32_t lfsm_get_gc_seu_resv(void)
{
    return lfsmCfg.gc_seu_reserved;
}

EXPORT_SYMBOL(lfsm_get_gc_seu_resv);

int8_t lfsm_get_sparedisks(int8_t * spare_msg)
{
    return _get_sparedisks(spare_msg);
}

EXPORT_SYMBOL(lfsm_get_sparedisks);

int8_t lfsm_get_groupdisks(uint32_t groupId, int8_t * gdisk_msg)
{
    return _get_groupdisks(groupId, gdisk_msg);
}

EXPORT_SYMBOL(lfsm_get_groupdisks);

uint32_t lfsm_get_groups(void)
{
    return _get_groups();
}

EXPORT_SYMBOL(lfsm_get_groups);

uint64_t lfsm_get_physicalCap(void)
{
    return _get_physicalCap();
}

EXPORT_SYMBOL(lfsm_get_physicalCap);

uint64_t lfsm_cal_resp_iops(void)
{
    uint64_t respond_iops;
    respond_iops = atomic_read(&cal_respond_iops);
    return respond_iops;
}

EXPORT_SYMBOL(lfsm_cal_resp_iops);

uint64_t lfsm_cal_req_iops(void)
{
    uint64_t request_iops;
    request_iops = atomic_read(&cal_request_iops);
    return request_iops;
}

EXPORT_SYMBOL(lfsm_cal_req_iops);

int32_t load_lfsm_drv(void)
{
    int32_t ret;

    ret = 0;

    LOGINFO("Loading LFSM driver.....\n");

    lfsm_init_state = 0;
    init_lfsm_config();

    return ret;
}

void unload_lfsm_drv(void)
{
    LOGINFO("removing LFSM driver.....\n");

    lfsm_cleanup(&lfsmCfg, true);
}
