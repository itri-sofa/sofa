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
#include <linux/bio.h>
#include "../config/lfsm_setting.h"
#include "../config/lfsm_feature_setting.h"
#include "../config/lfsm_cfg.h"
#include "../common/common.h"
#include "../common/rmutex.h"
#include "../common/mem_manager.h"
#include "../io_manager/mqbiobox.h"
#include "../io_manager/biocbh.h"
#include "../nextprepare.h"
#include "../diskgrp.h"
#include "Dedicated_map.h"
#include "../lfsm_core.h"
#include "../EU_access.h"
#include "../EU_create.h"
#include "../io_manager/io_common.h"
#include "Super_map.h"
#include "../spare_bmt_ul.h"
#include "../devio.h"

// tifa: 2^nK safety 2012/2/13
/*
 * Description: bio end function of ddmap read/write
 */
static void end_bio_dedicated_page(struct bio *bio)
{
    lfsm_dev_t *td;

    td = bio->bi_private;
    LFSM_ASSERT(td != NULL);
    if (!bio->bi_status) {
        td->DeMap.fg_io_result = 1;
    } else {
        td->DeMap.fg_io_result = blk_status_to_errno(bio->bi_status);
    }
    bio->bi_iter.bi_size = HARD_PAGE_SIZE;
    wake_up_interruptible(&td->DeMap.wq);

    return;
}

int32_t ddmap_get_ondisk_size(int32_t cn_disks)
{
    // dev_state + BMT_map + BMT_redundant_map + erase_head + meta_logger
    return sizeof(sddmap_disk_hder_t) + (MAX_BMT_EUS * (sizeof(sector64) * 2)) +
        cn_disks * (sizeof(enum edev_state) +
                    sizeof(erase_head_t) + sizeof(sector64) * MLOG_EU_NUM);
}

/*
 * Description: alloc and init next EU for dedicated map
 */
int32_t dedicated_map_eunext_init(SDedicatedMap_t * pDeMap)
{
    return SNextPrepare_alloc(&pDeMap->next, EU_DEDICATEMAP);
}

/*
 * Description: alloc and init main and backup EU for dedicated map.
 *              Each time will get EU from different disk for wear leveling
 */
void dedicated_map_eu_init(SDedicatedMap_t * pDeMap)
{
    struct HListGC *h;

    h = gc[get_next_disk()];
    pDeMap->eu_main = FreeList_delete(h, EU_DEDICATEMAP, false);
    h = gc[get_next_disk()];
    pDeMap->eu_backup = FreeList_delete(h, EU_DEDICATEMAP, false);
    //pDeMap->eu_main->log_frontier = 0; AN:20140828 setting in free_list_delete
    //pDeMap->eu_backup->log_frontier = 0;

    dedicated_map_eunext_init(pDeMap);
}

sector64 ddmap_get_BMT_redundant_map(sddmap_disk_hder_t * pheader, int32_t idx,
                                     int32_t cn_backup)
{
    sector64 *pBMT_map;

    if (pheader->off_BMT_map * sizeof(tp_ondisk_unit) <
        sizeof(sddmap_disk_hder_t)) {
        LOGERR("pheader %p off_BMT_redundant_map = %d < %lu\n", pheader,
               pheader->off_BMT_map,
               sizeof(sddmap_disk_hder_t) / sizeof(tp_ondisk_unit));
        return ~0;
    }

    pBMT_map = (sector64 *) ((int8_t *) pheader +
                             ((pheader->off_BMT_map) * sizeof(tp_ondisk_unit)));
    return pBMT_map[MAX_BMT_EUS * cn_backup + idx];
}

static void ddmap_set_BMT_redundant_map(sddmap_disk_hder_t * pheader,
                                        int32_t idx, sector64 v)
{
    sector64 *pBMT_map;

    if (pheader->off_BMT_map * sizeof(tp_ondisk_unit) <
        sizeof(sddmap_disk_hder_t)) {
        LOGERR("set pheader %p off_BMT_redundant_map = %d < %lu\n", pheader,
               pheader->off_BMT_map,
               sizeof(sddmap_disk_hder_t) / sizeof(tp_ondisk_unit));
        return;
    }

    pBMT_map = (sector64 *) ((int8_t *) pheader +
                             ((pheader->off_BMT_map) * sizeof(tp_ondisk_unit)));
    pBMT_map[MAX_BMT_EUS + idx] = v;
    //LOGINFO("Save the pos of backup BMT[%d] : %llds (EU %lld)\n",
    //    idx,v,v>>SECTORS_PER_SEU_ORDER);
    return;
}

sector64 ddmap_get_BMT_map(sddmap_disk_hder_t * pheader, int32_t idx)
{
    sector64 *pBMT_map;

    if (pheader->off_BMT_map * sizeof(tp_ondisk_unit) <
        sizeof(sddmap_disk_hder_t)) {
        LOGERR("pheader %p off_BMT_map = %d < %lu\n", pheader,
               pheader->off_BMT_map,
               sizeof(sddmap_disk_hder_t) / sizeof(tp_ondisk_unit));
        return ~0;
    }

    pBMT_map =
        (sector64 *) ((int8_t *) pheader +
                      (pheader->off_BMT_map * sizeof(tp_ondisk_unit)));

    return pBMT_map[idx];
}

static void ddmap_set_BMT_map(sddmap_disk_hder_t * pheader, int32_t idx,
                              sector64 v)
{
    sector64 *pBMT_map;

    if (pheader->off_BMT_map * sizeof(tp_ondisk_unit) <
        sizeof(sddmap_disk_hder_t)) {
        LOGERR("set pheader %p off_BMT_map = %d < %lu\n", pheader,
               pheader->off_BMT_map,
               sizeof(sddmap_disk_hder_t) / sizeof(tp_ondisk_unit));
        return;
    }

    pBMT_map =
        (sector64 *) ((int8_t *) pheader +
                      (pheader->off_BMT_map * sizeof(tp_ondisk_unit)));
    pBMT_map[idx] = v;
    //LOGINFO("Save the pos of main BMT[%d] : %llds (EU %lld)\n",
    //    idx,v,v>>SECTORS_PER_SEU_ORDER);

    return;
}

struct external_ecc_head *ddmap_get_ext_ecc(sddmap_disk_hder_t * pheader,
                                            int32_t idx)
{
    struct external_ecc_head *pret;
    if (pheader->off_ext_ecc * sizeof(tp_ondisk_unit) <
        sizeof(sddmap_disk_hder_t)) {
        LOGERR("pheader %p off_ext_ecc = %d < %lu\n", pheader,
               pheader->off_ext_ecc,
               sizeof(sddmap_disk_hder_t) / sizeof(tp_ondisk_unit));
        return NULL;
    }
    pret = (struct external_ecc_head *)((int8_t *) pheader +
                                        (pheader->off_ext_ecc *
                                         sizeof(tp_ondisk_unit)));

    return &pret[idx];
}

erase_head_t *ddmap_get_erase_map(sddmap_disk_hder_t * pheader, int32_t idx)
{
    erase_head_t *pret;
    if (pheader->off_erase_map * sizeof(tp_ondisk_unit) <
        sizeof(sddmap_disk_hder_t)) {
        LOGERR("pheader %p off_erase_map = %d < %lu\n", pheader,
               pheader->off_erase_map,
               sizeof(sddmap_disk_hder_t) / sizeof(tp_ondisk_unit));
        return NULL;
    }
    pret = (erase_head_t *) ((int8_t *) pheader +
                             (pheader->off_erase_map * sizeof(tp_ondisk_unit)));

    return &pret[idx];
}

static enum edev_state ddmap_get_dev_state(sddmap_disk_hder_t * pheader,
                                           int32_t idx)
{
    enum edev_state *pstate;

    if (pheader->off_dev_state * sizeof(tp_ondisk_unit) <
        sizeof(sddmap_disk_hder_t)) {
        LOGERR("pheader %p off_dev_state = %d < %lu\n", pheader,
               pheader->off_dev_state,
               sizeof(sddmap_disk_hder_t) / sizeof(tp_ondisk_unit));
        return error;
    }

    pstate = (enum edev_state *)((int8_t *) pheader +
                                 (pheader->off_dev_state *
                                  sizeof(tp_ondisk_unit)));
    return pstate[idx];
}

static void ddmap_set_dev_state(sddmap_disk_hder_t * pheader, int32_t idx,
                                enum edev_state v)
{
    enum edev_state *pstate;
    if (pheader->off_dev_state * sizeof(tp_ondisk_unit) <
        sizeof(sddmap_disk_hder_t)) {
        LOGERR("pheader %p off_dev_state = %d < %lu\n", pheader,
               pheader->off_dev_state,
               sizeof(sddmap_disk_hder_t) / sizeof(tp_ondisk_unit));
        return;
    }
    pstate = (enum edev_state *)((int8_t *) pheader +
                                 (pheader->off_dev_state *
                                  sizeof(tp_ondisk_unit)));
    pstate[idx] = v;
}

sector64 *ddmap_get_eu_mlogger(sddmap_disk_hder_t * pheader)
{
    if (pheader->off_eu_mlogger * sizeof(tp_ondisk_unit) <
        sizeof(sddmap_disk_hder_t)) {
        LOGERR("pheader %p off_eu_mlogger = %d < %lu\n", pheader,
               pheader->off_eu_mlogger,
               sizeof(sddmap_disk_hder_t) / sizeof(tp_ondisk_unit));
        return NULL;
    }
    return (sector64 *) ((int8_t *) pheader +
                         (pheader->off_eu_mlogger * sizeof(tp_ondisk_unit)));
}

void dedicated_map_set_dev_state(sddmap_disk_hder_t * prec, diskgrp_t * pgrp)
{
    int32_t i;
    for (i = 0; i < pgrp->cn_max_drives; i++) {
        //td->subdev[i].load_from_prev =rec->dev_state[i];
        if (pgrp->ar_subdev[i].load_from_prev == non_ready) {
            LOGINFO("skip-set subdev[%d] sign %x\n", i,
                    pgrp->ar_subdev[i].load_from_prev);
            continue;
        }
        pgrp->ar_subdev[i].load_from_prev = ddmap_get_dev_state(prec, i);
    }
    LOGINFO("dev state: ");
    for (i = 0; i < pgrp->cn_max_drives; i++) {
        printk("[%d]=0x%x ", i, pgrp->ar_subdev[i].load_from_prev);
    }
    printk("\n");
}

static void dedicated_map_get_dev_state(sddmap_disk_hder_t * prec,
                                        diskgrp_t * pgrp)
{
    int32_t i;
    for (i = 0; i < pgrp->cn_max_drives; i++) {
        ddmap_set_dev_state(prec, i, pgrp->ar_subdev[i].load_from_prev);
    }
}

void ddmap_header_dump(sddmap_disk_hder_t * prec)
{
    LOGINFO("getoff %d %d %d %d\n",
            prec->off_dev_state, prec->off_BMT_map, prec->off_erase_map,
            prec->off_ext_ecc);
}

static void ddmap_header_init(sddmap_disk_hder_t * prec, int32_t cn_disks,
                              int32_t cn_BMT_eu, int32_t cn_erase_eu)
{
    prec->off_dev_state =
        LFSM_CEIL_DIV(sizeof(sddmap_disk_hder_t), sizeof(tp_ondisk_unit));
    prec->off_BMT_map =
        prec->off_dev_state +
        LFSM_CEIL_DIV((sizeof(enum edev_state) * cn_disks),
                      sizeof(tp_ondisk_unit));
    prec->off_erase_map = prec->off_BMT_map + LFSM_CEIL_DIV(sizeof(sector64) * cn_BMT_eu * 2, sizeof(tp_ondisk_unit));  // include redundant
    prec->off_ext_ecc = prec->off_erase_map +
        LFSM_CEIL_DIV(sizeof(erase_head_t) * cn_erase_eu,
                      sizeof(tp_ondisk_unit));
    prec->off_eu_mlogger =
        prec->off_ext_ecc +
        LFSM_CEIL_DIV(sizeof(struct external_ecc_head) * cn_disks,
                      sizeof(tp_ondisk_unit));

//    LOGINFO("setoff dev_state %d bmt_map %d erase_map %d\n",
//        prec->off_dev_state, prec->off_BMT_map,prec->off_erase_map);
    return;
}

static int32_t ddmap_write2buffer(int8_t * pbuf, lfsm_dev_t * td,
                                  enum elfsm_state signature,
                                  struct EUProperty *peu)
{
    sddmap_disk_hder_t *prec;
    erase_head_t *perase_map;
    struct external_ecc_head *ext_ecc;
    struct HListGC *h;
    int32_t i;

    prec = (sddmap_disk_hder_t *) pbuf;

    /* initial ddmap on disk header */
    ddmap_header_init(prec, td->cn_ssds, MAX_BMT_EUS, MAX_ERASE_EUS);
    prec->page_number = peu->log_frontier >> SECTORS_PER_HARD_PAGE_ORDER;
    prec->signature = signature;
    prec->update_log_head = td->UpdateLog.eu_main->start_pos;
    prec->update_log_backup = td->UpdateLog.eu_backup->start_pos;
    prec->logical_capacity = td->logical_capacity;
    prec->BMT_map_gc = td->ltop_bmt->BMT_EU_gc->start_pos;
    prec->BMT_redundant_map_gc = td->ltop_bmt->BMT_EU_backup_gc->start_pos;
    HPEU_get_eupos(&td->hpeutab, &prec->HPEU_map, &prec->HPEU_redundant_map,
                   &prec->HPEU_idx_next);

    /* set state of ddmap */
    dedicated_map_get_dev_state(prec, &grp_ssds);

    /* set BMT position of ddmap */
    for (i = 0; i < td->ltop_bmt->BMT_num_EUs; i++) {
        ddmap_set_BMT_map(prec, i, td->ltop_bmt->BMT_EU[i]->start_pos);
    }

    /* set redundant BMT position of ddmap */
    for (i = 0; i < td->ltop_bmt->BMT_num_EUs; i++) {
        ddmap_set_BMT_redundant_map(prec, i,
                                    td->ltop_bmt->BMT_EU_backup[i]->start_pos);
    }

    /* set External ECC position of ddmap */
    for (i = 0; i < td->cn_ssds; i++) {
        ext_ecc = ddmap_get_ext_ecc(prec, i);
        ext_ecc->external_ecc_head = td->ExtEcc[i].eu_main->start_pos;
        ext_ecc->external_ecc_frontier = td->ExtEcc[i].eu_main->log_frontier;
    }

    /* set erase map information */
    for (i = 0; i < td->cn_ssds; i++) { /* iterate over all disks */
        h = gc[i];
        if ((h != NULL) && (h->ECT.eu_curr)) {
            perase_map = ddmap_get_erase_map(prec, i);
            memcpy(perase_map->magic, "ERSM", 4);
            perase_map->start_pos = h->ECT.eu_curr->start_pos;
            perase_map->disk_num = h->disk_num;
            perase_map->frontier = h->ECT.eu_curr->log_frontier;
        } else {
            LOGERR("disk[%d] h is NULLor eu_curr is NULL when save\n", i);
        }
    }

    /* set mlogger information */
    metalog_eulog_get_all(td->mlogger.par_eulog, ddmap_get_eu_mlogger(prec),
                          td->cn_ssds);

    return 0;
}

static bool dedicated_map_logging_A(lfsm_dev_t * td, enum elfsm_state signature,
                                    struct EUProperty *eu)
{
    SDedicatedMap_t *pDeMap;
    int8_t *buffer;
    int32_t i, cn_hpage;

    cn_hpage =
        LFSM_CEIL_DIV(ddmap_get_ondisk_size(td->cn_ssds), HARD_PAGE_SIZE);
    pDeMap = &td->DeMap;

    if (NULL == (buffer =
                 track_kmalloc(ddmap_get_ondisk_size(td->cn_ssds),
                               GFP_KERNEL | __GFP_ZERO, M_UNKNOWN))) {
        LOGERR("Memory allocation failed for dedicated_map_logging_A\n");
        return false;
    }

    pDeMap->bio->bi_iter.bi_size = PAGE_SIZE * MEM_PER_HARD_PAGE * cn_hpage;
    pDeMap->bio->bi_next = NULL;
    pDeMap->bio->bi_iter.bi_idx = 0;
    pDeMap->bio->bi_opf = 1;
//    pDeMap->bio->bi_flags = pDeMap->org_bi_flag;
    pDeMap->bio->bi_disk = NULL;

    ddmap_write2buffer(buffer, td, signature, eu);

    for (i = 0; i < cn_hpage; i++) {
        bio_vec_hpage_rw(&(pDeMap->bio->bi_io_vec[i * MEM_PER_HARD_PAGE]),
                         buffer + (i * HARD_PAGE_SIZE), 0);
    }

    pDeMap->bio->bi_iter.bi_sector = eu->start_pos + eu->log_frontier;
    pDeMap->bio->bi_seg_front_size = 0;
    pDeMap->bio->bi_seg_back_size = 0;
    pDeMap->bio->bi_flags = (pDeMap->bio->bi_flags >> BVEC_POOL_OFFSET) << BVEC_POOL_OFFSET;    // clear all bit except 4 POOL bit
    pDeMap->bio->bi_status = BLK_STS_OK;
    pDeMap->bio->bi_flags &= ~(1 << BIO_SEG_VALID);
    pDeMap->fg_io_result = 0;

    if (my_make_request(pDeMap->bio) == 0) {
        lfsm_wait_event_interruptible(pDeMap->wq, pDeMap->fg_io_result != 0);
    }

    if (pDeMap->fg_io_result <= 0) {
        LOGERR("logging dedicate map %llu frontier %d failure\n", eu->start_pos,
               eu->log_frontier);
    }
    //tf todo: if dedicate map write fail, get a new one and re-write
    eu->log_frontier += SECTORS_PER_HARD_PAGE * cn_hpage;
    track_kfree(buffer, ddmap_get_ondisk_size(td->cn_ssds), M_UNKNOWN);

    return true;
}

/*
 * Description: write ddmp with status (signature) to disk
 *
 * todo: data might lost when crash during logging state
 */
int32_t dedicated_map_logging_state(lfsm_dev_t * td, enum elfsm_state signature)
{
    SDedicatedMap_t *pDeMap;
    struct EUProperty *old_main, *old_backup;
    int32_t cn_hpage;
    bool ret1, ret2;

    if (lfsm_readonly == 1) {
        return 0;
    }

    pDeMap = &td->DeMap;
    ret1 = ret2 = true;
    cn_hpage =
        LFSM_CEIL_DIV(ddmap_get_ondisk_size(td->cn_ssds), HARD_PAGE_SIZE);
    LFSM_MUTEX_LOCK(&pDeMap->sdd_lock);
    old_main = pDeMap->eu_main;
    old_backup = pDeMap->eu_backup;
    /* no enough remaing space for ddmap in old EU, get new one */
    if (old_main->log_frontier >
        (SECTORS_PER_SEU - (cn_hpage << SECTORS_PER_HARD_PAGE_ORDER))) {
        LOGINFO("Wrapping around ddmap..Frontier %d\n", old_main->log_frontier);
        pDeMap->cn_cycle++;
        SNextPrepare_get_new(&pDeMap->next, &pDeMap->eu_main,
                             &pDeMap->eu_backup, EU_DEDICATEMAP);
    }

    ret1 = dedicated_map_logging_A(td, signature, pDeMap->eu_main);
    ret2 = dedicated_map_logging_A(td, signature, pDeMap->eu_backup);
//    LOGINFO("ddmap eu %llu frontier %d\n",pDeMap->eu_main->start_pos, pDeMap->eu_main->log_frontier);
    if (old_main->start_pos != pDeMap->eu_main->start_pos) {
        super_map_logging_all(td, &grp_ssds);
        LOGINFO("new ddmap %llu %llu logged to supermap\n",
                pDeMap->eu_main->start_pos, pDeMap->eu_backup->start_pos);
        SNextPrepare_put_old(&pDeMap->next, old_main, old_backup);
    }
    LFSM_MUTEX_UNLOCK(&pDeMap->sdd_lock);

    if (ret1 == false || ret2 == false) {
        return -1;
    }
    //LFSM_ASSERT(pDeMap->eu_main->log_frontier == pDeMap->eu_backup->log_frontier);
    return 0;
}

/*
 * Description: API to write ddmp with status (signature) to disk
 * return value: 0 : log ok
 *               non 0 : log fail
 *
 */
int32_t dedicated_map_logging(lfsm_dev_t * td)
{
    return dedicated_map_logging_state(td, sys_loaded);
}

/*
 * Description: free the old next of dedicated map
 *              allocate new one for next of dedicated map
 */
void dedicated_map_change_next(SDedicatedMap_t * pDeMap)
{
    SNextPrepare_renew(&pDeMap->next, EU_DEDICATEMAP);
}

static bool fpage_is_unwritten(int8_t * pbuff)
{
    if ((*(sector64 *) pbuff) == TAG_UNWRITE_HPAGE) {
        return true;
    }

    return false;
}

/*
 * Description:
 * Parameter  :
 *     ar_buf:   buffer for storing main dedicate map and backup dedicate map
 *     cn_fpage: size of dedicated map in page unit
 *     log_frontier: offset in EU in sector unit
 * return     : -1 : fail
 *               0 : main eu
 *               1 : backup eu
 */
//returen -1: fail, 0: main_eu, 1:backup_eu
static int32_t dedicated_map_select_eu(SDedicatedMap_t * pDeMap,
                                       lfsm_dev_t * td, int8_t ** ar_buf,
                                       int32_t cn_fpage, sector64 log_frontier)
{
    int32_t ret1, ret2;
    ret1 =
        devio_read_hpages(pDeMap->eu_main->start_pos + log_frontier, ar_buf[0],
                          cn_fpage);
    ret2 =
        devio_read_hpages(pDeMap->eu_backup->start_pos + log_frontier,
                          ar_buf[1], cn_fpage);

    // Weafon: The following logic should be reviewed in the future. 2012/10/02
    if ((!DEV_SYS_READY(grp_ssds, pDeMap->eu_main->disk_num) &&
         !DEV_SYS_READY(grp_ssds, pDeMap->eu_backup->disk_num))
        || (ret1 < 0 && ret2 < 0)) {
        LOGERR(" ddmap failure, can't get recovery information\n");
        return -1;
    }

    if ((DEV_SYS_READY(grp_ssds, pDeMap->eu_main->disk_num)) && (ret1 >= 0) &&
        (fpage_is_unwritten(ar_buf[0]) == false)) {
        return 0;
    } else if ((DEV_SYS_READY(grp_ssds, pDeMap->eu_backup->disk_num)) &&
               (ret2 >= 0) && (fpage_is_unwritten(ar_buf[1]) == false)) {
        return 1;
    }

    if ((DEV_SYS_READY(grp_ssds, pDeMap->eu_main->disk_num)) && (ret1 >= 0)) {
        return 0;
    } else if ((DEV_SYS_READY(grp_ssds, pDeMap->eu_backup->disk_num))
               && (ret2 >= 0)) {
        return 1;
    }

    return -1;
}

/*
 * Description: Get last validate entry
 * return value: true : get last entry fail
 *               false: get last entry ok
 */
bool dedicated_map_get_last_entry(SDedicatedMap_t * pDeMap,
                                  sddmap_disk_hder_t * old_rec, lfsm_dev_t * td)
{
    erase_head_t *perase_map;
    sddmap_disk_hder_t *rec;
    sector64 log_frontier;
    int8_t *ar_buf[2] = { NULL };
    int32_t i, cn_eu, idx, idx_st_hpage, cn_fpage;
    bool ret;

    log_frontier = 0;
    cn_eu = 2;
    idx = -1;
    ret = true;

    cn_fpage =
        LFSM_CEIL_DIV(ddmap_get_ondisk_size(td->cn_ssds), HARD_PAGE_SIZE);
    LOGINFO("Begin to search lastest record in ddmap..................\n");
    for (i = 0; i < cn_eu; i++) {
        if (IS_ERR_OR_NULL(ar_buf[i] =
                           track_kmalloc(cn_fpage * HARD_PAGE_SIZE, GFP_KERNEL,
                                         M_UNKNOWN))) {
            goto l_end;
        }
    }

    old_rec->page_number = -1;

    idx_st_hpage = 0;

    for (i = idx_st_hpage; i < HPAGE_PER_SEU; i += cn_fpage) {
        log_frontier = i * SECTORS_PER_HARD_PAGE;
        if ((idx =
             dedicated_map_select_eu(pDeMap, td, ar_buf, cn_fpage,
                                     log_frontier)) < 0) {
            goto l_end;
        }

        rec = (sddmap_disk_hder_t *) ar_buf[idx];

        if (rec->page_number > old_rec->page_number && rec->page_number == i) {
            memcpy(old_rec, ar_buf[idx], cn_fpage * HARD_PAGE_SIZE);
//            LOGINFO("ddmap: id_page %d frontier %llu\n", rec->page_number, log_frontier);
            perase_map = ddmap_get_erase_map(old_rec, 0);
//            LOGINFO("ddmap: addr %llus frontier %llu old_rec: {%llu,%llu, erasemap: (%llu,%llu), %d, sign%x}\n",
//                (idx?pDeMap->eu_backup->start_pos:pDeMap->eu_main->start_pos), log_frontier,
//                old_rec->update_log_head,ddmap_get_BMT_map(old_rec,0),(perase_map)?perase_map->start_pos:~0,
//                (perase_map)?perase_map->frontier:~0,old_rec->page_number,old_rec->signature);
        } else {
            break;
        }
    }

    /* next start position to write dedicated map */
    pDeMap->eu_main->log_frontier = log_frontier;
    pDeMap->eu_backup->log_frontier = log_frontier;
    ret = false;

  l_end:
    for (i = 0; i < cn_eu; i++) {
        if (ar_buf[i]) {
            track_kfree(ar_buf[i], cn_fpage * HARD_PAGE_SIZE, M_UNKNOWN);
        }
    }

    return ret;
}

/*
 * Description: rescue dedicated map
 * return: true : ok
 *         false: fail
 */
bool dedicated_map_rescue(lfsm_dev_t * td, SDedicatedMap_t * pDeMap)
{
    return sysEU_rescue(td, &pDeMap->eu_main, &pDeMap->eu_backup, &pDeMap->next,
                        &pDeMap->sdd_lock);
}

/*
 * Description: read dedicated map from disk
 * return: true : fail
 *         false: ok
 */
bool dedicated_map_load(SDedicatedMap_t * pDeMap, int32_t cn_item_to_load,
                        int32_t lfsm_cn_ssds)
{
    sddmap_disk_hder_t *pheader;
    sector64 log_frontier;
    int8_t *ar_buf[2] = { NULL };
    int32_t i, j, cn_eu, idx, cn_fpage;
    bool ret1, ret2, ret;

    cn_eu = 2;
    idx = -1;
    cn_fpage =
        LFSM_CEIL_DIV(ddmap_get_ondisk_size(lfsm_cn_ssds), HARD_PAGE_SIZE);

    ret = true;
    for (i = 0; i < cn_eu; i++) {
        if (NULL ==
            (ar_buf[i] =
             track_kmalloc(cn_fpage * HARD_PAGE_SIZE, GFP_KERNEL, M_UNKNOWN))) {
            LOGERR("fail alloc mem when load ddmap\n");
            goto l_end;
        }
    }

    for (i = 0; i < cn_item_to_load; i += cn_fpage) {
        log_frontier = i * SECTORS_PER_HARD_PAGE;

        ret1 =
            devio_read_hpages_batch(pDeMap->eu_main->start_pos + log_frontier,
                                    ar_buf[0], cn_fpage);
        ret2 =
            devio_read_hpages_batch(pDeMap->eu_backup->start_pos + log_frontier,
                                    ar_buf[1], cn_fpage);

        for (j = 0; j < cn_eu; j++) {
            pheader = (sddmap_disk_hder_t *) ar_buf[j];
            LOGINFO("ddmap: page %d frontier %llu\n", pheader->page_number,
                    log_frontier);
            LOGINFO
                ("in dedicated %s at addr %llu frontier %llu {ud:%llu %llu,BMT:%llu %llu , erase_map:%llu,%llu, %d,%x}\n",
                 (idx ? "Backup" : "Main"),
                 (idx ? pDeMap->eu_backup->start_pos : pDeMap->eu_main->
                  start_pos), log_frontier, pheader->update_log_head,
                 pheader->update_log_backup, ddmap_get_BMT_map(pheader, 0),
                 ddmap_get_BMT_redundant_map(pheader, 0, 1),
                 ddmap_get_erase_map(pheader, 0)->start_pos,
                 ddmap_get_erase_map(pheader, 0)->frontier,
                 pheader->page_number, pheader->signature);
        }
    }

    for (i = 0; i < cn_eu; i++) {
        if (ar_buf[i]) {
            track_kfree(ar_buf[i], cn_fpage * HARD_PAGE_SIZE, M_UNKNOWN);
        }
    }

    ret = false;

  l_end:
    return ret;
}

static void comp_and_log(int8_t * format, int8_t * name, sector64 num_a,
                         sector64 num_b)
{
    if (num_a == num_b) {
        return;
    }

    printk(format, name, num_a, num_b);
}

bool ddmap_dump(lfsm_dev_t * td, int8_t * data_main, int8_t * data_backup)
{
    sddmap_disk_hder_t *rec_main, *rec_backup;
    erase_head_t *perasemap_main, *perasemap_backup;
    sector64 *ar_eu_mlogger_main, *ar_eu_mlogger_backup;
    struct EUProperty *eu_main, *eu_backup;
    int32_t i, j;

    rec_main = (sddmap_disk_hder_t *) data_main;
    rec_backup = (sddmap_disk_hder_t *) data_backup;

    // mian field
    comp_and_log("%24s %d %d \n", "page_number", rec_main->page_number,
                 rec_backup->page_number);

    comp_and_log("%24s %x %x \n", "signature", rec_main->signature,
                 rec_backup->signature);

    comp_and_log("%24s %llu %llu \n", "update_log_head",
                 rec_main->update_log_head, rec_backup->update_log_head);

    comp_and_log("%24s %llu %llu \n", "update_log_backup",
                 rec_main->update_log_backup, rec_backup->update_log_backup);

    comp_and_log("%24s %llu %llu \n", "BMT_map_gc", rec_main->BMT_map_gc,
                 rec_backup->BMT_map_gc);

    comp_and_log("%24s %llu %llu \n", "BMT_redundant_map_gc",
                 rec_main->BMT_redundant_map_gc,
                 rec_backup->BMT_redundant_map_gc);

    comp_and_log("%24s %llu %llu \n", "logical_capacity",
                 rec_main->logical_capacity, rec_backup->logical_capacity);

    comp_and_log("%24s %llu %llu \n", "HPEU_map", rec_main->HPEU_map,
                 rec_backup->HPEU_map);

    comp_and_log("%24s %llu %llu \n", "HPEU_redundant_map",
                 rec_main->HPEU_redundant_map, rec_backup->HPEU_redundant_map);

    comp_and_log("%24s %d %d \n", "HPEU_idx_next",
                 rec_main->HPEU_idx_next, rec_backup->HPEU_idx_next);

    LOGINFO(" BMT redundent EUs\n");
    for (i = 0; i < td->ltop_bmt->BMT_num_EUs; i++) {
        comp_and_log(" %llu %llu \n", "BMT main",
                     ddmap_get_BMT_redundant_map(rec_main, i, 1),
                     ddmap_get_BMT_redundant_map(rec_backup, i, 1));
    }

    LOGINFO(" BMT map EUs\n");
    for (i = 0; i < td->ltop_bmt->BMT_num_EUs; i++) {
        comp_and_log(" %llu %llu \n", "BMT_backup",
                     ddmap_get_BMT_map(rec_main, i),
                     ddmap_get_BMT_map(rec_backup, i));
    }

    for (i = 0; i < td->cn_ssds; i++) {
        LOGINFO("Disk %d \n", i);
        perasemap_main = ddmap_get_erase_map(rec_main, i);
        perasemap_backup = ddmap_get_erase_map(rec_backup, i);
        comp_and_log("disk_id %d %d\n", "disk_id",
                     perasemap_main->disk_num, perasemap_backup->disk_num);
        comp_and_log("star_pos %llu %llu\n", "start_pos",
                     perasemap_main->start_pos, perasemap_backup->start_pos);
        comp_and_log("frotier %llu %llu \n", "frotier",
                     perasemap_main->frontier, perasemap_backup->frontier);

        comp_and_log("dev_stat %d %d \n", "dev_start",
                     ddmap_get_dev_state(rec_main, i),
                     ddmap_get_dev_state(rec_backup, i));
        ar_eu_mlogger_main = ddmap_get_eu_mlogger(rec_main);
        ar_eu_mlogger_backup = ddmap_get_eu_mlogger(rec_backup);
        for (j = 0; j < MLOG_EU_NUM; j++) {
            eu_main = eu_find(ARI(ar_eu_mlogger_main, i, j, MLOG_EU_NUM));
            eu_backup = eu_find(ARI(ar_eu_mlogger_backup, i, j, MLOG_EU_NUM));
            if (eu_main->start_pos != eu_backup->start_pos) {
                LOGINFO("MLOG %d %d %llu %llu\n", i, j, eu_main->start_pos,
                        eu_backup->start_pos);
            }
        }
    }

    return 0;
}

/*
 * Description: free dedicated map in memory structure
 */
void dedicated_map_destroy(lfsm_dev_t * td)
{
    int32_t cn_fpage;

    cn_fpage =
        LFSM_CEIL_DIV(ddmap_get_ondisk_size(td->cn_ssds), HARD_PAGE_SIZE);
    if (td->DeMap.bio) {
        free_composed_bio(td->DeMap.bio, cn_fpage * MEM_PER_HARD_PAGE);
    }

    td->DeMap.bio = NULL;
}

/*
 * Description: initialize dedicated map in memory data structure
 * return value: true : init fail
 *               false: init ok
 */
bool dedicated_map_init(lfsm_dev_t * td)
{
    SDedicatedMap_t *pDeMap;
    int32_t cn_hpage;

    cn_hpage =
        LFSM_CEIL_DIV(ddmap_get_ondisk_size(td->cn_ssds), HARD_PAGE_SIZE);
    pDeMap = &td->DeMap;
    if (compose_bio(&pDeMap->bio, NULL, end_bio_dedicated_page, td,
                    cn_hpage * HARD_PAGE_SIZE,
                    cn_hpage * MEM_PER_HARD_PAGE) < 0) {
        return true;
    }
//    pDeMap->org_bi_flag = pDeMap->bio->bi_flags;
    init_waitqueue_head(&pDeMap->wq);
    pDeMap->fg_io_result = 0;
    mutex_init(&pDeMap->sdd_lock);
    pDeMap->cn_cycle = 0;

    return false;
}
