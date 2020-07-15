/*
 * SPDX-License-Identifier: GPL-2.0-or-later OR Apache-2.0
 * Copyright (C) 2015-2020 Industrial Technology Research Institute
 *
 *
 *
 *  
 */
/*
** This file constitues of all the functions related to the BMT Commit Manager
** Regarding bmt commit (critical & Popularity-based) and dependency lists maintenance
*/
#include <linux/types.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/bio.h>
#include "config/lfsm_setting.h"
#include "config/lfsm_feature_setting.h"
#include "config/lfsm_cfg.h"
#include "common/common.h"
#include "common/rmutex.h"
#include "common/mem_manager.h"
#include "io_manager/mqbiobox.h"
#include "io_manager/biocbh.h"
#include "nextprepare.h"
#include "diskgrp.h"
#include "system_metadata/Dedicated_map.h"
#include "lfsm_core.h"
#include "bmt.h"
#include "bmt_ppq.h"
#include "bmt_ppdm.h"
#include "io_manager/io_common.h"
//#include "New_BMT_updatelog.h"
#include "EU_create.h"
#include "special_cmd.h"
#include "bmt_commit.h"
#include "spare_bmt_ul.h"
#include "GC.h"

int32_t BMT_cache_commit(lfsm_dev_t * td, MD_L2P_table_t * bmt)
{
    PPDM_BMT_dbmta_return_threshold(td, bmt);
    wake_up_all(&td->gc_conflict_queue);
    return true;
}

/* 
** This is the main handler of the BMT commit
** Based upon the return value of the get_commit_threshold(), decide which commit algorithm needs to be deployed
** Calls PPQ_BMT_nonpending_threshold() to regulate the memory usage of BMT cache
** Checks if the update log tail could be advanced
**
** @td : lfsm_dev_t object
** @bmt : BMT object
**
** Returns 0
 */
int32_t BMT_commit_manager(lfsm_dev_t * td, MD_L2P_table_t * bmt)
{
#if 0
    /* To decide whether to not perform the BMT commit */
    if (!get_commit_threshold(td)) {
        dprintk("Not enough BMT updates. Don't commit.\n");
        return -1;
    }

    /* Critical Commit */
    else {
        printk("Critical Commit \n");
        // tf: bmt commit don't trigger update_log EU change
        update_ondisk_BMT(td, td->ltop_bmt, false); // for autotesting
    }

/* Check the number of nonpending updates in the BMT cache and free enough if necessary */
/*
   PPDM_BMT_dbmta_return_threshold(td, bmt,cnt_remove_array);
*/
#endif
    return 0;
}

// return -EAGAIN, or 0
static int32_t PPDM_PAGEBMT_Flush_bh(MD_L2P_table_t * bmt, devio_fn_t ** par_cb,
                                     int32_t count)
{
    int32_t i, j;
    bool fg_redo;

    fg_redo = false;
    for (i = 0; i < count; i++) {
        for (j = 0; j < 2; j++) {
            if (NULL == par_cb[j][i].pBio) {
                continue;
            }

            if (devio_async_write_bh(&par_cb[j][i])) {
                continue;
            }

            LOGERR("%d async_write pBio %p\n", i, par_cb[j][i].pBio);
            LFSM_MUTEX_LOCK(&(bmt->per_page_queue[i].ppq_lock));
            bmt->per_page_queue[i].fg_pending = true;
            LFSM_MUTEX_UNLOCK(&(bmt->per_page_queue[i].ppq_lock));
            fg_redo = true;
        }
    }

    if (fg_redo) {
        return -EAGAIN;
    }
    return 0;
}

static int32_t PPDM_PAGEBMT_write_valid_bitmap(lfsm_dev_t * td,
                                               MD_L2P_table_t * bmt,
                                               devio_fn_t * pCb)
{
    sector64 addr1, addr2;
    int8_t *buffer;
    int32_t i, id_eu, id_page, cn_fpage_valid_bitmap, ret, offset;

    ret = 0;
    cn_fpage_valid_bitmap = bmt_cn_fpage_valid_bitmap(bmt);

    for (i = 0; i < cn_fpage_valid_bitmap; i++) {
        id_eu = (bmt->cn_ppq + i) / FPAGE_PER_SEU;
        id_page = (bmt->cn_ppq + i) % FPAGE_PER_SEU;

        buffer = bmt->AR_PAGEBMT_isValidSegment + i * FLASH_PAGE_SIZE;
        addr1 =
            bmt->BMT_EU[id_eu]->start_pos +
            (id_page << SECTORS_PER_FLASH_PAGE_ORDER);
        offset = i << 1;
        if (atomic_read(&((pCb + offset)->result)) != DEVIO_RESULT_SUCCESS) {
            devio_write_page(addr1, buffer, -1, pCb + offset);
        }

        addr2 =
            bmt->BMT_EU_backup[id_eu]->start_pos +
            (id_page << SECTORS_PER_FLASH_PAGE_ORDER);
        if (atomic_read(&((pCb + offset + 1)->result)) != DEVIO_RESULT_SUCCESS) {
            devio_write_page(addr2, buffer, -1, pCb + offset + 1);
        }
    }

    for (i = 0; i < (cn_fpage_valid_bitmap << 1); i++) {
        if (NULL == pCb[i].pBio) {
            continue;
        }

        if (!devio_async_write_bh(&pCb[i])) {
            LOGWARN("bmt bitmap %d fpage write fail\n", i);
            ret = -EIO;
        }
    }

    return ret;
}

static int32_t PPDM_PAGEBMT_commit_valid_bitmap(lfsm_dev_t * td,
                                                MD_L2P_table_t * bmt)
{
    devio_fn_t *pCb;
    int32_t cn_fpage_valid_bitmap, ret, cn_retry;

    cn_fpage_valid_bitmap = bmt_cn_fpage_valid_bitmap(bmt);

    if (NULL == (pCb =
                 track_kmalloc(cn_fpage_valid_bitmap * 2 * sizeof(devio_fn_t),
                               GFP_KERNEL | __GFP_ZERO, M_UNKNOWN))) {
        LOGERR("Cannot alloc memory\n");
        return -ENOMEM;
    }

    ret = 0;
    cn_retry = 0;
    LOGINFO("Committing cn_ppq = %d , cn_fpage_valid_bitmap %d \n",
            bmt->cn_ppq, cn_fpage_valid_bitmap);
    for (cn_retry = 0; cn_retry < 3; cn_retry++) {
        if (PPDM_PAGEBMT_write_valid_bitmap(td, bmt, pCb) == 0) {
            break;
        }

        cn_retry++;
        LOGINFO("commit bmt bitmap %d times fail\n", cn_retry);
    }

    if (cn_retry >= 3) {
        LOGERR("commit bmt bitmap fail \n");
        ret = -EIO;
    }

    track_kfree(pCb, cn_fpage_valid_bitmap * 2 * sizeof(devio_fn_t), M_UNKNOWN);
    return ret;
}

int32_t update_ondisk_BMT(lfsm_dev_t * td, MD_L2P_table_t * bmt, bool reuse)
{
    devio_fn_t *pCallback[2];
    int32_t i, ret_cnt, cn_cmt, ret, max_round;
    bool fg_redo, fg_should_bh;

    if (lfsm_readonly == 1) {
        return 0;
    }

    max_round = 40960;

    for (i = 0; i < 2; i++) {
        if (NULL == (pCallback[i] = (devio_fn_t *)
                     kmalloc(sizeof(devio_fn_t) * max_round, GFP_KERNEL))) {
            LOGERR("Can't alloc devio_callback\n");
            for (i--; i >= 0; i--)
                kfree(pCallback[i]);
            return -ENOMEM;
        }
    }

    LFSM_MUTEX_LOCK(&bmt->cmt_lock);

    LOGINFO("bmt segments to be scanned: %lu reuse %d\n",
            bmt->BMT_num_EUs * FPAGE_PER_SEU, reuse);

    ret_cnt = 0;
    ret = 0;

  l_flush:
    fg_should_bh = false;
    fg_redo = false;
    cn_cmt = 0;
    for (i = 0; i < 2; i++)
        memset(pCallback[i], 0, sizeof(devio_fn_t) * max_round);

    for (i = 0; i < bmt->cn_ppq; i++) {
        ret_cnt = PPDM_PAGEBMT_Flush(td, bmt, i, pCallback, cn_cmt % max_round);
        if ((ret_cnt < 0) || (ret_cnt == 1)) {
            fg_should_bh = true;
            cn_cmt++;
            if (ret_cnt < 0) {
                ret = -EIO;
            }
        }

        if ((fg_should_bh) && (cn_cmt % max_round == 0)) {
            if (PPDM_PAGEBMT_Flush_bh(bmt, pCallback, max_round) == -EAGAIN) {
                fg_redo = true;
            }

            LOGINFO("bmt[%d] commited: %d pagebmt_cn_used: %d been fail: %d\n",
                    i, cn_cmt, bmt->pagebmt_cn_used, ret);
            fg_should_bh = false;
        }
    }

    if (PPDM_PAGEBMT_Flush_bh(bmt, pCallback, max_round) == -EAGAIN) {
        fg_redo = true;
    }

    LOGINFO("bmt total %d commited: %d pagebmt_cn_used: %d been fail: %d\n",
            bmt->cn_ppq, cn_cmt, bmt->pagebmt_cn_used, ret);
    if (fg_redo) {
        goto l_flush;
    }

    if ((ret = PPDM_PAGEBMT_commit_valid_bitmap(td, bmt)) < 0) {
        LOGERR("Commit BMT valid bitmap fail ret= %d \n", ret); // retry 3 times inside,nothing we can do.. just log err
    }

    goto l_fail;
  l_fail:
    for (i = 0; i < 2; i++)
        kfree(pCallback[i]);
    LFSM_MUTEX_UNLOCK(&bmt->cmt_lock);
    return ret;
}

/* 
** This function is called during the unload of the driver
** It calls the update_ondisk_BMT() to update on the on-disk BMT completely
** Also, it frees the cache entries completely
**
** @td : lfsm_dev_t object
** @bmt : BMT object
**
** Returns 0
*/
void flush_clear_bmt_cache(lfsm_dev_t * td, MD_L2P_table_t * bmt,
                           bool isNormalExit)
{
    MD_L2P_item_t *ppq;
    dbmtE_bucket_t *pItem;
    int32_t i;

    if (NULL == bmt) {
        return;
    }

    if ((isNormalExit == true) && (lfsmCfg.testCfg.crashSimu == false)) {
        update_ondisk_BMT(td, bmt, false);
        LOGINFO("Updated ondisk BMT....done\n");
    }

    ppq = NULL;
    for (i = 0; i < bmt->cn_ppq; i++) {
        ppq = &bmt->per_page_queue[i];
        if (NULL == ppq) {
            continue;
        }
        if (NULL == (pItem = ppq->pDbmtaUnit)) {
            continue;
        }

        dbmtaE_BK_free(&td->dbmtapool, pItem);
        ppq->pDbmtaUnit = NULL;
        mutex_destroy(&(bmt->per_page_queue[i].ppq_lock));
    }

/**For the unload signature before destroying data structures **/
    if ((isNormalExit == true) && (lfsmCfg.testCfg.crashSimu == false)) {
        dedicated_map_logging_state(td, sys_unloaded);  //tf: should pass parameter of DeMap structure //log_super
    }

    mutex_destroy(&(bmt->cmt_lock));
    LOGINFO("PPQ_BMT_cache_cleanup Done....\n");

    dedicated_map_destroy(td);
    LOGINFO(".. done\n");
/*
    for(i = 0; i < td->DeMap.bio->bi_vcnt; i++){
        track_free_page(td->DeMap.bio->bi_io_vec[i].bv_page);
    }
    bio_put(td->DeMap.bio);
*/
}
