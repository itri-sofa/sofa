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
/* 
* This file is used for on-cache BMT with direct mapping in page-size record.
* The pbno is the only information which is remained in this table, and it could be commit immediately
* without merging and splitting comparing with old function in bmt_ppq.c
* Each 4K(one page size) recorder array should be allocate dynamically, so the mechanism of allocating and 
* free memory rules need to setup and maintain.
* 
* P.S. These function are tided with "dbmtapool."
*
* Author:    Maggie Lin
* Date:        2011.07.25
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
#include "bmt_ppdm.h"
#include "bmt.h"
#include "io_manager/io_common.h"
#include "dbmtapool.h"
#include "spare_bmt_ul.h"
#include "autoflush.h"
#include "EU_create.h"
#include "devio.h"
#include "sflush.h"
#include "bmt_commit.h"

/*========= Private Function ================
* Which is self-contained function for the element of on-cache bmt
*/
/* used for  PPQ_BMT_cache_inser_nonpending and PPQ_BMT_cache_insert_one_pending in public function
*  [in]td: the globe lfsm structure for adding or modifying the record array in page size
*  [in]ullDbmt_idx: the index of each Direct BMT array
*  [in]pbno: physical block number
*  [in]pending_flag: pending_flag=1 means the update data is pending one; pending_flag=0 means  non-pending 
*  [return]: the static non-pending number.
*/
static int32_t PPDM_BMT_update(MD_L2P_item_t * ppq,
                               uint64_t idx_dbmta, sector64 pbno,
                               uint16_t fg_pending,
                               enum EDBMTA_OP_DIRTYBIT op_dirty,
                               bool is_fake_pbn)
{
    dbmtE_bucket_t *pDbmta_unit_local;
    int32_t fg_pending_old, num_nonpending_adjust;  // the initial value to differ from pending and nonpending case

    fg_pending_old = DBMTE_EMPTY;
    num_nonpending_adjust = 0;
    LFSM_ASSERT(idx_dbmta < (FLASH_PAGE_SIZE / sizeof(struct D_BMT_E)));
    LFSM_ASSERT(fg_pending == DBMTE_PENDING || fg_pending == DBMTE_NONPENDING);

    pDbmta_unit_local = ppq->pDbmtaUnit;
    LFSM_ASSERT2(pDbmta_unit_local != NULL, "%s : ppq = %p\n", __FUNCTION__,
                 ppq);
    fg_pending_old = pDbmta_unit_local->BK_dbmtE[idx_dbmta].pending_state;
    if (!is_fake_pbn) {
        if (pbno == PBNO_SPECIAL_NEVERWRITE) {
            pDbmta_unit_local->BK_dbmtE[idx_dbmta].pbno =
                PBNO_SPECIAL_NEVERWRITE;
            if (fg_pending == DBMTE_NONPENDING) {
                pDbmta_unit_local->BK_dbmtE[idx_dbmta].pending_state =
                    DBMTE_NONPENDING;
                return 1;
            } else {
                // either hybrid or trim may send a "neverwrite" pending update to here
                pDbmta_unit_local->BK_dbmtE[idx_dbmta].pending_state =
                    DBMTE_PENDING;
                return 0;
            }
        }
    }

    if (fg_pending == DBMTE_PENDING) {
        pDbmta_unit_local->BK_dbmtE[idx_dbmta].pbno = pbno;
        pDbmta_unit_local->BK_dbmtE[idx_dbmta].pending_state = DBMTE_PENDING;
        if (is_fake_pbn) {
            pDbmta_unit_local->BK_dbmtE[idx_dbmta].fake_pbno = 1;
        }

        if (fg_pending_old == DBMTE_NONPENDING) {
            num_nonpending_adjust = -1;
        }
        ppdm_printk("1:idx_dbmta=%llu num_nonpending=%d, update pbno=%llu\n",
                    idx_dbmta, num_nonpending_adjust,
                    (uint64_t) pDbmta_unit_local->BK_dbmtE[idx_dbmta].pbno);
    } else {                    // if(fg_pending==DBMTE_NONPENDING)
        if (fg_pending_old == DBMTE_EMPTY) {
            pDbmta_unit_local->BK_dbmtE[idx_dbmta].pbno = pbno;
            pDbmta_unit_local->BK_dbmtE[idx_dbmta].pending_state =
                DBMTE_NONPENDING;
            pDbmta_unit_local->BK_dbmtE[idx_dbmta].fake_pbno = is_fake_pbn;

            num_nonpending_adjust = 1;
            ppdm_printk
                ("2:idx_dbmta=%llu num_nonpending=%d, update pbno=%llu\n",
                 idx_dbmta, num_nonpending_adjust,
                 (uint64_t) pDbmta_unit_local->BK_dbmtE[idx_dbmta].pbno);
        }
    }
    return num_nonpending_adjust;
}

/* PPDM_BMT_cache_LRU_insert fuction insert the entry to the tail of BMT LRU
 * @[in]bmt: bmt 
 * @[in]entry: ppq->pDbmtaUnit
 */
// direct_bmt: should be maintained, keep it the same
static void PPDM_BMT_cache_LRU_insert(MD_L2P_table_t * bmt,
                                      MD_L2P_item_t * entry)
{
    LFSM_ASSERT(entry);
    spin_lock(&bmt->LRU_spinlock);
    list_move_tail(&entry->LRU, &bmt->cache_LRU_head);
    spin_unlock(&bmt->LRU_spinlock);
}

/* PPDM_BMT_cache_LRU_remove fuction remove the entry to the tail of BMT LRU
 * @[in]bmt: bmt 
 * @[in]entry: ppq->pDbmtaUnit
 */
// direct_bmt: should be maintained, keep it the same
static bool PPDM_BMT_cache_LRU_remove(MD_L2P_table_t * bmt,
                                      MD_L2P_item_t * entry)
{
    if (NULL == entry) {
        LOGERR("entry=NULL\n");
        return false;
    }

    spin_lock(&bmt->LRU_spinlock);
    list_del_init(&entry->LRU);
    spin_unlock(&bmt->LRU_spinlock);

    return true;
}

static bool PPDM_BMT_return_ppq(lfsm_dev_t * td, MD_L2P_table_t * bmt,
                                MD_L2P_item_t * c_ppq)
{
    LFSM_MUTEX_LOCK(&c_ppq->ppq_lock);
    //the fg_pending should be false if it is need to be return
    if (c_ppq->fg_pending) {
        LFSM_MUTEX_UNLOCK(&c_ppq->ppq_lock);
        return false;
    }

    if (c_ppq->cn_bioc_pins < 0) {
        LFSM_MUTEX_UNLOCK(&c_ppq->ppq_lock);
        LOGERR("err: c_ppq->cn_bioc_pins=%d\n", c_ppq->cn_bioc_pins);
        return false;
    }

    if (c_ppq->cn_bioc_pins > 0) {
        LFSM_MUTEX_UNLOCK(&c_ppq->ppq_lock);
        return false;
    }

    dbmtaE_BK_free(&td->dbmtapool, c_ppq->pDbmtaUnit);
    c_ppq->non_pending_num = 0;
    c_ppq->pDbmtaUnit = NULL;
    LFSM_MUTEX_UNLOCK(&c_ppq->ppq_lock);

    if (!PPDM_BMT_cache_LRU_remove(bmt, c_ppq)) {
        LOGERR("err: PPDM_BMT_cache_LRU_remove fail\n");
        return false;
    }

    return true;
}

 // call from autoflush
bool PPDM_BMT_ppq_flush_return(lfsm_dev_t * td, MD_L2P_item_t * c_ppq)
{
    ppdm_printk(" try flush return %dthppq\n",
                c_ppq - td->ltop_bmt->per_page_queue);
    LFSM_MUTEX_LOCK(&td->ltop_bmt->cmt_lock);
    if (PPDM_PAGEBMT_isFULL(td->ltop_bmt)) {
        if (!PPDM_PAGEBMT_CheckAndRunGC(td, td->ltop_bmt)) {    // always true
            LFSM_MUTEX_UNLOCK(&td->ltop_bmt->cmt_lock);
            return false;
        }
    }

    if (PPDM_PAGEBMT_Flush
        (td, td->ltop_bmt, c_ppq - td->ltop_bmt->per_page_queue, NULL, 0) < 0) {
        LFSM_ASSERT(0);
    }

    LFSM_MUTEX_UNLOCK(&td->ltop_bmt->cmt_lock);
    return (PPDM_BMT_return_ppq(td, td->ltop_bmt, c_ppq));
}

void PPDM_BMT_dbmta_dump(MD_L2P_item_t * ppq)
{
    int32_t i, idx;

    idx = ppq - &lfsmdev.ltop_bmt->per_page_queue[0];
    if (ppq->pDbmtaUnit) {
        LOGINFO("ppq %d ", idx);
        for (i = 0; i < DBMTE_NUM; i++) {
            printk("%llx ", (sector64) ppq->pDbmtaUnit->BK_dbmtE[i].pbno);
        }
        printk("\n");
    } else {
        LOGINFO("ppq %d empty\n", idx);
    }

    return;
}

/* PPDM_BMT_dbmta_return fuction remove the dbmta_unit entry that all bmt elements are nonpending 
 * @[in]bmt: bmt 
 * @[in]cnt: number of the entries to be removed. Internally will remove USED_CACHE_ARRAY_T_DIFF more to make sure total # of non-pending A_BMT_E will less than USED_CACHE_ARRAY_T_LOW
 * @[return]: number of entries removed
 */
int32_t PPDM_BMT_dbmta_return(lfsm_dev_t * td, MD_L2P_table_t * bmt,
                              int32_t cnt_toBeRemove)
{
    MD_L2P_item_t *c_ppq, *n_ppq;
    int32_t cnt_remove, cnt_remove_array;   // count non_pending number

    cnt_remove = 0;
    cnt_remove_array = 0;
    list_for_each_entry_safe(c_ppq, n_ppq, &bmt->cache_LRU_head, LRU) {
        if (cnt_remove_array >= cnt_toBeRemove) {
            return 0;
        }

        if (c_ppq->non_pending_num == 0) {  //kick it out from the LRU list
            continue;
        }

        if (!c_ppq->fg_pending) {   //no pending -> all non-pending
            if (!PPDM_BMT_return_ppq(td, bmt, c_ppq)) { // a dbmta-pinned ppq can be returned
                continue;
            }
            cnt_remove += c_ppq->non_pending_num;
            cnt_remove_array++;
            ppdm_printk("return non pending_items num[%p]=%d (%d)\n",
                        c_ppq, cnt_remove, cnt_remove_array);
        }
    }

    list_for_each_entry_safe(c_ppq, n_ppq, &bmt->cache_LRU_head, LRU) {
        if (cnt_remove_array >= cnt_toBeRemove) {
            return 0;
        }
// GC for PageBMT log
        if (!PPDM_BMT_ppq_flush_return(td, c_ppq)) {
            continue;
        }
        cnt_remove += c_ppq->non_pending_num;
        cnt_remove_array++;
        ppdm_printk("return pending_items num[%p]=%d (%d) \n", c_ppq,
                    cnt_remove, cnt_remove_array);
    }

    return cnt_remove;
}

/*========= Pubic Function =================
* Deal with the committing situation
*/

/* PPDM_BMT_cache_init fuction initialize the bmt cache
 * @[in]bmt: bmt 
 * @[return]: void
 */
bool PPDM_BMT_cache_init(MD_L2P_table_t * bmt)
{
    int32_t i;
#if DYNAMIC_PPQ_LOCK_KEY==1
    bmt->ar_key = (struct lock_class_key *)
        track_kmalloc(bmt->cn_ppq * sizeof(struct lock_class_key), GFP_KERNEL,
                      M_UNKNOWN);
    if (IS_ERR_OR_NULL(bmt->ar_key)) {
        return false;
    }
#else
    static struct lock_class_key __key[PPDM_STATIC_KEY_NUM];
#endif

    //printk("Initializing Cache structures..... \n");
    spin_lock_init(&bmt->LRU_spinlock);
    INIT_LIST_HEAD(&bmt->cache_LRU_head);
    atomic64_set(&bmt->cn_valid_page_ssd, 0);
    atomic64_set(&bmt->cn_dirty_page_ssd, 0);

#if DYNAMIC_PPQ_LOCK_KEY==0
    if (bmt->cn_ppq > PPDM_STATIC_KEY_NUM) {
        LOGERR("bmt size %d out of bound\n", bmt->cn_ppq);
        return false;
    }
#endif

    for (i = 0; i < bmt->cn_ppq; i++) {
#if DYNAMIC_PPQ_LOCK_KEY == 1
        __mutex_init(&(bmt->per_page_queue[i].ppq_lock),
                     "&((*bmt)->per_page_queue[i].ppq_lock)", &bmt->ar_key[i]);
#else
        __mutex_init(&(bmt->per_page_queue[i].ppq_lock),
                     "&((*bmt)->per_page_queue[i].ppq_lock)", &__key[i]);
#endif
        bmt->per_page_queue[i].pDbmtaUnit = NULL;
        bmt->per_page_queue[i].fg_pending = false;
        bmt->per_page_queue[i].non_pending_num = 0;
        INIT_LIST_HEAD(&bmt->per_page_queue[i].LRU);
        bmt->per_page_queue[i].addr_ondisk = PAGEBMT_NON_ONDISK;
        bmt->per_page_queue[i].ondisk_logi_page = PAGEBMT_NON_ONDISK;
        bmt->per_page_queue[i].ondisk_logi_eu = PAGEBMT_NON_ONDISK;
        bmt->per_page_queue[i].cn_bioc_pins = 0;

    }
    LOGINFO("DONE\n");

    return true;
}

/* PPDM_BMT_cache_lookup fuction query the bmt for lbno
 * @[in]bmt: bmt 
 * @[in]lbno: LBN 
 * @[return]: true means got it (in_mem) false mean neverwrite or not in cache 
 */
bool PPDM_BMT_cache_lookup(MD_L2P_table_t * bmt, sector64 lbno,
                           bool incr_acc_count, struct D_BMT_E *pret_dbmta)
{
    dbmtE_bucket_t *pDbmta_unit_local;
    uint64_t record_num_per_page, record_num_per_sector, _rem, _q_no, _lbno;
    uint64_t idx_dbmta;

    pret_dbmta->pbno = PBNO_SPECIAL_NOT_IN_CACHE;
    pret_dbmta->dirty = false;
    pret_dbmta->pending_state = DBMTE_EMPTY;

    record_num_per_page = FLASH_PAGE_SIZE;
    record_num_per_sector = SECTOR_SIZE;
    do_div(record_num_per_page, sizeof(struct D_BMT_E));
    do_div(record_num_per_sector, sizeof(struct D_BMT_E));

    _lbno = (uint64_t) lbno;
    _rem = do_div(_lbno, record_num_per_sector);
    _q_no = _lbno;
    do_div(_q_no, SECTORS_PER_FLASH_PAGE);
    idx_dbmta = lbno - _q_no * record_num_per_page;

    dprintk("%s : idx=%llu lbn=%lld qno=%d\n", __FUNCTION__, idx_dbmta, lbno,
            (int32_t) _q_no);
    if (NULL == bmt->per_page_queue[_q_no].pDbmtaUnit) {
        if (bmt->per_page_queue[_q_no].addr_ondisk == PAGEBMT_NON_ONDISK) {
            pret_dbmta->pbno = PBNO_SPECIAL_NEVERWRITE;
            return false;       //PBNO_SPECIAL_NEVERWRITE;
        }
//        printk("pDbmtaUnit(%llu) == NULL  _q_no=%llu ondisk=%lld\n", lbno, _q_no,bmt->per_page_queue[_q_no].addr_ondisk);
        bmt->per_page_queue[_q_no].fg_pending = false;  // reset the initail value if the pDbmtaUint is null
        bmt->per_page_queue[_q_no].non_pending_num = 0;
        pret_dbmta->pbno = PBNO_SPECIAL_NOT_IN_CACHE;
        return false;           //PBNO_SPECIAL_NOT_IN_CACHE;
    } else {                    //if (bmt->per_page_queue[_q_no].pDbmtaUnit != NULL)
        pDbmta_unit_local = bmt->per_page_queue[_q_no].pDbmtaUnit;

        if (pDbmta_unit_local->BK_dbmtE[idx_dbmta].pending_state == DBMTE_EMPTY) {
            ppdm_printk("pending_state(%llu) == EMPTY\n", lbno);
            pret_dbmta->pbno = PBNO_SPECIAL_NEVERWRITE;
            return false;       //PBNO_SPECIAL_NEVERWRITE;
        } else {
            ppdm_printk("pending_state(%llu) != EMPTY\n", lbno);
            *pret_dbmta = pDbmta_unit_local->BK_dbmtE[idx_dbmta];
        }
    }

    if ((pret_dbmta->pbno != PBNO_SPECIAL_NOT_IN_CACHE) &&
        (bmt->per_page_queue[_q_no].non_pending_num != 0)) {
        PPDM_BMT_cache_LRU_insert(bmt, &bmt->per_page_queue[_q_no]);
    }

    return true;
}

/* PPDM_BMT_cache_insert_nonpending fuction insert the bmt_in on-disk bmt buffer to the PPQ->BMT_cache
 * @[in]bmt: bmt 
 * @[in]bmt_in: on-disk BMT_buffer$$$$$bmt_in must contains BMT records in the same PPQ!!!
 * @[in]cnt: number of D_BMT_E in the bmt_in
 * @[in]start_lbno: start lbno of bmt_in
 * @[return]: always 1
 */
void PPDM_BMT_cache_insert_nonpending(lfsm_dev_t * td,
                                      MD_L2P_table_t * bmt,
                                      struct D_BMT_E *bmt_in, int32_t cnt,
                                      sector64 start_lbno)
{
    MD_L2P_item_t *ppq;
    struct D_BMT_E *pBmt_in;
    uint64_t record_num_per_page, record_num_per_sector, _q_no, _rem;
    uint64_t idx_dbmta, _start_lbno;
    int32_t i, num_nonpending;

    ppq = NULL;
    pBmt_in = NULL;
    record_num_per_page = FLASH_PAGE_SIZE;
    record_num_per_sector = SECTOR_SIZE;
    num_nonpending = 0;
    do_div(record_num_per_page, sizeof(struct D_BMT_E));
    do_div(record_num_per_sector, sizeof(struct D_BMT_E));

    _start_lbno = (uint64_t) start_lbno;
    _rem = do_div(_start_lbno, record_num_per_sector);
    _q_no = _start_lbno;
    do_div(_q_no, (FLASH_PAGE_SIZE / SECTOR_SIZE));
    idx_dbmta = start_lbno - _q_no * record_num_per_page;

// get the per_page_queue entry
    ppq = &bmt->per_page_queue[_q_no];
    ppdm_printk("record_num_per_page=%llu, record_num_per_sector=%llu, "
                "_start_lbno=%llu, _rem=%llu \n",
                record_num_per_page, record_num_per_sector, _start_lbno, _rem);
    ppdm_printk("idx_dbmta=%llu, start_lbno=%llu, _q_no=%llu \n", idx_dbmta,
                start_lbno, _q_no);
// if the pDbmtaUnit does not have any array in it, just alloc a new one and hook it on the bmt_cache

    LFSM_ASSERT(ppq->pDbmtaUnit);
    ppdm_printk("pointer of pDbmtaUnit= %p \n", ppq->pDbmtaUnit);
    for (i = 0; i < cnt; i++) { //cnt=record_num_per_page
        pBmt_in = &(bmt_in[i]); // bmt_in include one sector size info
        ppdm_printk("DBMTE_NONPENDING: lbno=%llu, pbno=%llu \n", lbno,
                    pBmt_in->pbno);
        num_nonpending += PPDM_BMT_update(ppq, idx_dbmta + i, pBmt_in->pbno, DBMTE_NONPENDING, pBmt_in->dirty, pBmt_in->fake_pbno); // 0 for non-pending
    }
    ppq->non_pending_num += num_nonpending;
    PPDM_BMT_cache_LRU_insert(bmt, ppq);
}

/* PPDM_BMT_cache_insert_one_pending fuction insert one A_BMT_E to the bmt_cache
 * @[in]bmt: bmt 
 * @[in]abmte_in: A_BMT_E to insert
 * @[return]: always 1
 */
static MD_L2P_item_t *PPDM_BMT_lpn2ppq2(MD_L2P_table_t * bmt, sector64 lbno,
                                        int32_t * ret_idx_dbmta)
{
    *ret_idx_dbmta = lbno & ((1 << (FLASH_PAGE_ORDER - 3)) - 1);
    return &bmt->per_page_queue[lbno >> (FLASH_PAGE_ORDER - 3)];
}

static MD_L2P_item_t *PPDM_BMT_lpn2ppq(MD_L2P_table_t * bmt, sector64 lbno,
                                       int32_t * ret_idx_dbmta)
{
    uint64_t record_num_per_sector, record_num_per_page, _lbno, _q_no, _rem;

    record_num_per_page = FLASH_PAGE_SIZE;
    record_num_per_sector = SECTOR_SIZE;
    do_div(record_num_per_page, sizeof(struct D_BMT_E));
    do_div(record_num_per_sector, sizeof(struct D_BMT_E));

    _lbno = (uint64_t) lbno;
    _rem = do_div(_lbno, record_num_per_sector);
    _q_no = _lbno;
    do_div(_q_no, (FLASH_PAGE_SIZE / SECTOR_SIZE));

    if (_q_no > bmt->cn_ppq) {
        return 0;
    }

    (*ret_idx_dbmta) = lbno - _q_no * record_num_per_page;

    return &bmt->per_page_queue[_q_no];
}

// the function may block the code...
static int32_t PPDM_BMT_CheckAndAllocDBMTA(lfsm_dev_t * td, MD_L2P_item_t * ppq)    // supposed that ppq's ppq_lock has been locked
{
    LFSM_ASSERT2(ppq->cn_bioc_pins >= 0, "ppq->cn_bioc_pins=%d\n",
                 ppq->cn_bioc_pins);
    while (NULL == ppq->pDbmtaUnit) {
        LFSM_ASSERT(IS_GC_THREAD(current->pid) == false);
        if (NULL != (ppq->pDbmtaUnit = dbmtaE_BK_alloc(&td->dbmtapool))) {
            break;
        }
        LFSM_MUTEX_UNLOCK(&ppq->ppq_lock);
        wake_up_interruptible(&td->worker_queue);
        lfsm_wait_event_interruptible(td->gc_conflict_queue,
                                      atomic_read(&td->dbmtapool.
                                                  dbmtE_bk_used) <
                                      td->dbmtapool.max_bk_size);
        LFSM_MUTEX_LOCK(&ppq->ppq_lock);
    }

    return 0;
}

int32_t PPDM_BMT_cache_remove_pin(MD_L2P_table_t * pbmt, sector64 lpn,
                                  int32_t cn_pins)
{
    MD_L2P_item_t *ppq;
    int32_t ret_idx_dbmta;

    ppq = NULL;
    if ((ppq = PPDM_BMT_lpn2ppq(pbmt, lpn, &ret_idx_dbmta)) == 0) {
        return -EINVAL;
    }

    LFSM_MUTEX_LOCK(&ppq->ppq_lock);
    ppq->cn_bioc_pins -= cn_pins;
    LFSM_MUTEX_UNLOCK(&ppq->ppq_lock);
    return 0;
}

int32_t PPDM_BMT_cache_insert_one_pending(lfsm_dev_t * td,
                                          sector64 lbno, sector64 pbno,
                                          int32_t rec_size, bool to_remove_pin,
                                          enum EDBMTA_OP_DIRTYBIT op_dirty,
                                          bool is_fake_pbn)
{
    MD_L2P_table_t *bmt;
    MD_L2P_item_t *ppq;
    uint64_t dpmta_lbno;
    sector64 dpmta_pbno;
    int32_t ret, num_nonpending, ret_idx_dbmta, i;

    bmt = td->ltop_bmt;
    ppq = NULL;
    ret = 0;
    num_nonpending = 0;

    if (!is_fake_pbn) {
        LFSM_ASSERT2(!(pbno % SECTORS_PER_FLASH_PAGE),
                     "ERROR: not aligned for lbn %llu ret_pbn = %llu\n", lbno,
                     pbno);
    }
    if (!(ppq = PPDM_BMT_lpn2ppq2(bmt, lbno, &ret_idx_dbmta))) {
        LFSM_ASSERT(0);
        return 0;
    }

    LFSM_MUTEX_LOCK(&ppq->ppq_lock);

    if (to_remove_pin) {
        ppq->cn_bioc_pins -= rec_size;
    }

    if ((ret = PPDM_BMT_CheckAndAllocDBMTA(td, ppq)) < 0) {
        goto l_end;
    }

    ppq->fg_pending = true;
    for (i = 0; i < rec_size; i++) {
        dpmta_lbno = ret_idx_dbmta + i;
        if (is_fake_pbn) {
            dpmta_pbno = pbno + (i << SECTORS_PER_FLASH_PAGE_ORDER);
        } else {
            dpmta_pbno = pbno + i;
        }

        ppq->pDbmtaUnit->BK_dbmtE[dpmta_lbno].pbno = dpmta_pbno;
        ppq->pDbmtaUnit->BK_dbmtE[dpmta_lbno].pending_state = DBMTE_PENDING;
        ppq->pDbmtaUnit->BK_dbmtE[dpmta_lbno].fake_pbno = is_fake_pbn;
        //if(is_fake_pbn)
        //printk("Ding: lbno %llu, fake_pbno %llu, is_fake_pbn %d\n",lbno,ppq->pDbmtaUnit->BK_dbmtE[dpmta_lbno].pbno,ppq->pDbmtaUnit->BK_dbmtE[dpmta_lbno].fake_pbno);
    }

    ppq->non_pending_num += num_nonpending;

  l_end:
    LFSM_MUTEX_UNLOCK(&ppq->ppq_lock);
    return ret;
}

/* PPDM_BMT_commit_build_page_buffer fuction fill a D_BMT_E buffer "without reading from on-disk bmt first"
 * @[in]bmt_out: bmt_out , NOTE this buffer must be allocated with GFP_ZERO
 * @[in]ppq: corresponding per page queue
 * @[in]recovery_flag: reuse should be "0" here, since it is not for recovery
 * @[return]: always 0
 */
static int32_t PPDM_BMT_commit_build_page_buffer(MD_L2P_table_t * bmt,
                                                 int8_t * bmt_out,
                                                 MD_L2P_item_t * ppq,
                                                 int32_t recovery_flag)
{
    dbmtE_bucket_t *pDbmta_unit_local;
    struct D_BMT_E *pUpdateBmtRecord;
    uint64_t record_num_per_page;
    int32_t num_nonpending, i;

    num_nonpending = 0;
    record_num_per_page = DBMTE_NUM;
    pDbmta_unit_local = ppq->pDbmtaUnit;
    for (i = 0; i < record_num_per_page; i++) {
        if (pDbmta_unit_local->BK_dbmtE[i].pending_state == DBMTE_PENDING) {
            pDbmta_unit_local->BK_dbmtE[i].pending_state = DBMTE_NONPENDING;
            num_nonpending++;
            ppdm_printk("pending to non-pending [%d]@normal commit mod!\n",
                        pDbmta_unit_local->BK_dbmtE[i].pending_state,
                        pDbmta_unit_local->BK_dbmtE[i].pbno);
        }
        pUpdateBmtRecord =
            (struct D_BMT_E *)&(bmt_out[i * sizeof(struct D_BMT_E)]);
        *pUpdateBmtRecord = pDbmta_unit_local->BK_dbmtE[i];

        ppdm_printk("on-cache pbno=%llu, on-disk pbno=%llu, num_nonpending=%d, "
                    "pending_state=%d \n",
                    pDbmta_unit_local->BK_dbmtE[i].pbno,
                    pUpdateBmtRecord->pbno, num_nonpending,
                    pDbmta_unit_local->BK_dbmtE[i].pending_state);
    }

    if (ppq->non_pending_num == 0 && num_nonpending != 0) { // all pending until this time we commit it, and it will become nonpending
        PPDM_BMT_cache_LRU_insert(bmt, ppq);
    }

    ppq->non_pending_num += num_nonpending;
    ppq->fg_pending = false;

    return 0;
}

/* PPDM_BMT_dbmta_return_threshold fuction check if the total non-pending count is larger than the USED_CACHE_ARRAY_T_HIGH, if yes, eliminate entries from the LRU BMT_cache
 * @[in]bmt: in-memory bmt
 * @[return]: always 0
 */
int32_t PPDM_BMT_dbmta_return_threshold(lfsm_dev_t * td, MD_L2P_table_t * bmt)
{
    int32_t cnt_remove_item;

    if (atomic_read(&td->dbmtapool.dbmtE_bk_used) <
        (td->dbmtapool.max_bk_size - DBMTAPOOL_ALLOC_LOWNUM)) {
        return 0;
    }

    cnt_remove_item = 0;
    ppdm_printk("[%s] available cnt in pool = %d\n", __FUNCTION__,
                atomic_read(&td->dbmtapool.dbmtapool_count));

    cnt_remove_item = PPDM_BMT_dbmta_return(td, bmt,
                                            (atomic_read
                                             (&td->dbmtapool.dbmtE_bk_used) +
                                             DBMTAPOOL_ALLOC_HIGHNUM -
                                             td->dbmtapool.max_bk_size));
    return 0;
}

static bool PPDM_BMT_need_flush(MD_L2P_item_t * ppq)
{
    return ((ppq->pDbmtaUnit != NULL) && (ppq->fg_pending));
}

// if ppq is read from backup BMT, reload it or set fg_pending as true; re-commit later
int32_t PPDM_PAGEBMT_reload_defect(lfsm_dev_t * td, MD_L2P_table_t * bmt)
{
    MD_L2P_item_t *ppq;
    struct EUProperty *peu;
    int32_t i, ret;

    ret = 0;

    for (i = 0; i < bmt->cn_ppq; i++) {
        ppq = &bmt->per_page_queue[i];

        if (ppq->addr_ondisk == PAGEBMT_NON_ONDISK) {
            continue;
        }

        if (NULL == (peu = eu_find(ppq->addr_ondisk))) {
            return -EPERM;
        }
        ///// chk1. if read from main eu
        if (peu == bmt->BMT_EU[ppq->ondisk_logi_eu]) {
            continue;
        }

        LFSM_MUTEX_LOCK(&(ppq->ppq_lock));
        ///// chk2. ppq is empty, load back (pDbmtaUnit=true || fg_pending=false)
        if (PPDM_BMT_need_flush(ppq)) {
            LFSM_MUTEX_UNLOCK(&(ppq->ppq_lock));
            continue;
        }

        if (ppq_alloc_reload(ppq, td, bmt, i) < 0) {
            LOGERR("ppq %d reload fail\n", i);
            LFSM_MUTEX_UNLOCK(&(ppq->ppq_lock));
            ret = -ENXIO;
            continue;
        }
        LFSM_MUTEX_UNLOCK(&(ppq->ppq_lock));
        ppq->fg_pending = true;
    }

    return ret;
}

static bool PPDM_PAGEBMT_AppendSegment(lfsm_dev_t * td,
                                       MD_L2P_table_t * bmt, int8_t * pBuf,
                                       int32_t PPDM_idx, devio_fn_t ** pCb,
                                       int cmt_idx)
{
    MD_L2P_item_t *ppq;
    int32_t id_eu, id_page;
    bool ret1, ret2;

    if (PPDM_PAGEBMT_isFULL(bmt)) {
        return false;
    }

    ret1 = ret2 = true;

    ppq = &bmt->per_page_queue[PPDM_idx];
    LFSM_ASSERT2(bmt->pagebmt_cn_used < (bmt->BMT_num_EUs * FPAGE_PER_SEU) &&
                 bmt->pagebmt_cn_used >= 0, "pagebmt %d over/lower bound\n",
                 bmt->pagebmt_cn_used);

    id_eu = PPDM_idx / FPAGE_PER_SEU;
    id_page = PPDM_idx % FPAGE_PER_SEU;
    //printk("%s : write seg to eu %lld pg %d with spare %d\n", __FUNCTION__,
    //        bmt->BMT_EU[id_eu]->start_pos, id_page, PPDM_idx);
    ret1 = devio_write_page(bmt->BMT_EU[id_eu]->start_pos +
                            (id_page << SECTORS_PER_FLASH_PAGE_ORDER), pBuf,
                            (sector64) PPDM_idx,
                            (pCb) ? (&pCb[0][cmt_idx]) : 0);
    //PPDM_PAGEBMT_printk("%s : write seg to eu %lld pg %d with spare %d\n", __FUNCTION__,
    //        bmt->BMT_EU_backup[id_eu]->start_pos, id_page, PPDM_idx);
    ret2 = devio_write_page(bmt->BMT_EU_backup[id_eu]->start_pos +
                            (id_page << SECTORS_PER_FLASH_PAGE_ORDER), pBuf,
                            (sector64) PPDM_idx,
                            (pCb) ? (&pCb[1][cmt_idx]) : 0);

    if (ppq->ondisk_logi_page == PAGEBMT_NON_ONDISK) {
        bmt->pagebmt_cn_used++;
    }

    if (ppq->ondisk_logi_page != PAGEBMT_NON_ONDISK) {
        lfsm_clear_bit((ppq->ondisk_logi_eu * FPAGE_PER_SEU + ppq->ondisk_logi_page), bmt->AR_PAGEBMT_isValidSegment);  // clear origial location
    }

    ppq->addr_ondisk =
        bmt->BMT_EU[id_eu]->start_pos +
        (id_page << SECTORS_PER_FLASH_PAGE_ORDER);
    ppq->ondisk_logi_eu = id_eu;
    ppq->ondisk_logi_page = id_page;
    lfsm_set_bit((id_eu * FPAGE_PER_SEU + id_page),
                 bmt->AR_PAGEBMT_isValidSegment);

    if (ret1 == false || ret2 == false) {
        return false;
    }

    return true;
}

int32_t PPDM_PAGEBMT_Flush(lfsm_dev_t * td, MD_L2P_table_t * bmt,
                           int32_t PPDM_idx, devio_fn_t ** pCb, int cmt_idx)
{
    int8_t *pBuf;
    int32_t ret;

    ret = false;
    LFSM_MUTEX_LOCK(&(bmt->per_page_queue[PPDM_idx].ppq_lock));
    if (!PPDM_BMT_need_flush(&bmt->per_page_queue[PPDM_idx])) {
        LFSM_MUTEX_UNLOCK(&(bmt->per_page_queue[PPDM_idx].ppq_lock));
        return 0;
    }

    pBuf = track_kmalloc(FLASH_PAGE_SIZE, __GFP_ZERO, M_BMT_CACHE);
    if (NULL == pBuf) {
        LOGERR("%s alloc page fail\n", __FUNCTION__);
        LFSM_MUTEX_UNLOCK(&(bmt->per_page_queue[PPDM_idx].ppq_lock));
        goto l_end;
    }

    PPDM_BMT_commit_build_page_buffer(bmt, pBuf, &bmt->per_page_queue[PPDM_idx],
                                      0);
    ret = PPDM_PAGEBMT_AppendSegment(td, bmt, pBuf, PPDM_idx, pCb, cmt_idx);
    LFSM_MUTEX_UNLOCK(&(bmt->per_page_queue[PPDM_idx].ppq_lock));

    track_kfree(pBuf, FLASH_PAGE_SIZE, M_BMT_CACHE);

  l_end:
    if (!ret) {
        return -EIO;
    }

    return 1;
}

bool PPDM_PAGEBMT_isFULL(MD_L2P_table_t * bmt)
{
    if (bmt->pagebmt_cn_used >= (bmt->BMT_num_EUs * FPAGE_PER_SEU)) {
        return true;
    } else {
        return false;
    }
}

/*
 return -1:false copy, >=0:cn_used_pages 
*/
static int32_t copy_valid_pagebmt(lfsm_dev_t * td, MD_L2P_table_t * bmt,
                                  int32_t * ar_mapping)
{
    struct EUProperty *pNewEU, *pNewEU_backup;
    sector64 *ar_lpn;
    sector64 addr_main, addr_bkup, source, offset;
    int32_t i, ret_source_idx, cn_used_pages, idx_spare;

    if (NULL ==
        (ar_lpn = (sector64 *) track_kmalloc(PAGE_SIZE, GFP_KERNEL, M_OTHER))) {
        return -ENOMEM;
    }

    source = 0;
    ret_source_idx = 0;
    cn_used_pages = 0;
    idx_spare = -1;
    pNewEU = bmt->BMT_EU_gc;
    pNewEU_backup = bmt->BMT_EU_backup_gc;

    LOGINFO("disk id {old, new}={%d, %d}\n", bmt->BMT_EU[0]->disk_num,
            pNewEU->disk_num);

    sflush_open(&td->crosscopy);
    for (i = 0; i < FPAGE_PER_SEU; i++) {
        if (lfsm_test_bit(i, bmt->AR_PAGEBMT_isValidSegment) == 0) {
            continue;
        }

        if ((i / MAX_LPN_ONE_SHOT) != idx_spare) {
            idx_spare = i / MAX_LPN_ONE_SHOT;
            offset =
                ((idx_spare *
                  MAX_LPN_ONE_SHOT) << SECTORS_PER_FLASH_PAGE_ORDER);
            addr_main = bmt->BMT_EU[0]->start_pos + offset;
            addr_bkup = bmt->BMT_EU_backup[0]->start_pos + offset;
            //printk("Read lpn spare %llu len %d\n",addr,cn_page_readspare);
            if ((ret_source_idx =
                 EU_spare_read_unit(td, (int8_t *) ar_lpn, addr_main, addr_bkup,
                                    MAX_LPN_ONE_SHOT << SZ_SPAREAREA_ORDER)) <
                0) {
                goto l_fail;
            }
        }

        if (ret_source_idx == 1) {  // spare read from main_eu
            source =
                bmt->BMT_EU_backup[0]->start_pos +
                (i << SECTORS_PER_FLASH_PAGE_ORDER);
        } else {                // spare read from bkup_eu
            source =
                bmt->BMT_EU[0]->start_pos + (i << SECTORS_PER_FLASH_PAGE_ORDER);
        }

        if (sflush_copy_submit(td, source,
                               pNewEU->start_pos +
                               (cn_used_pages << SECTORS_PER_FLASH_PAGE_ORDER),
                               ar_lpn[i % MAX_LPN_ONE_SHOT], 1) != 0) {
            goto l_fail;
        }

        if (sflush_copy_submit(td, source,
                               pNewEU_backup->start_pos +
                               (cn_used_pages << SECTORS_PER_FLASH_PAGE_ORDER),
                               ar_lpn[i % MAX_LPN_ONE_SHOT], 1) != 0) {
            goto l_fail;
        }

        ar_mapping[i] = cn_used_pages;
        lfsm_clear_bit(i, bmt->AR_PAGEBMT_isValidSegment);
        cn_used_pages++;
    }

    goto l_succ;
  l_fail:
    cn_used_pages = -1;
  l_succ:
    LOGINFO("cn_copy_pages %d submit\n", cn_used_pages);
    sflush_waitdone(&td->crosscopy);
    track_kfree(ar_lpn, PAGE_SIZE, M_OTHER);
    return cn_used_pages;
}

static void update_valid_pagebmt_and_ppq(MD_L2P_table_t * bmt,
                                         int32_t * ar_mapping,
                                         int32_t cn_used_pages)
{
    struct EUProperty *pOldEU, *pNewEU, *pOldEU_backup, *pNewEU_backup;
    MD_L2P_item_t *ppq;
    int32_t i;

    pNewEU = bmt->BMT_EU_gc;
    pNewEU_backup = bmt->BMT_EU_backup_gc;
    pOldEU = bmt->BMT_EU[0];
    pOldEU_backup = bmt->BMT_EU_backup[0];

    for (i = 1; i < bmt->BMT_num_EUs; i++) {
        bmt->BMT_EU[i - 1] = bmt->BMT_EU[i];
        bmt->BMT_EU_backup[i - 1] = bmt->BMT_EU_backup[i];
        memcpy(&bmt->AR_PAGEBMT_isValidSegment[((i - 1) * FPAGE_PER_SEU) / 8],
               &bmt->AR_PAGEBMT_isValidSegment[(i * FPAGE_PER_SEU) / 8],
               (FPAGE_PER_SEU) / 8);
    }

    bmt->BMT_EU[bmt->BMT_num_EUs - 1] = pNewEU;
    bmt->BMT_EU_backup[bmt->BMT_num_EUs - 1] = pNewEU_backup;
    bmt->BMT_EU_gc = pOldEU;
    bmt->BMT_EU_backup_gc = pOldEU_backup;
    for (i = 0; i < cn_used_pages; i++) {
        lfsm_set_bit((bmt->BMT_num_EUs - 1) * FPAGE_PER_SEU + i,
                     &bmt->AR_PAGEBMT_isValidSegment);
    }

    for (i = cn_used_pages; i < FPAGE_PER_SEU; i++) {
        lfsm_clear_bit((bmt->BMT_num_EUs - 1) * FPAGE_PER_SEU + i,
                       &bmt->AR_PAGEBMT_isValidSegment);
    }

    //update ppq address and logi info
    for (i = 0; i < bmt->cn_ppq; i++) {
        ppq = &(bmt->per_page_queue[i]);
        LFSM_MUTEX_LOCK(&(ppq->ppq_lock));
        if (ppq->addr_ondisk == PAGEBMT_NON_ONDISK) {
            LFSM_MUTEX_UNLOCK(&(ppq->ppq_lock));
            continue;
        }

        ppq->ondisk_logi_eu--;
        if (ppq->ondisk_logi_eu != -1) {
            LFSM_MUTEX_UNLOCK(&(ppq->ppq_lock));
            continue;
        }

        ppq->ondisk_logi_eu = bmt->BMT_num_EUs - 1;
        LFSM_ASSERT(ar_mapping[ppq->ondisk_logi_page] != PAGEBMT_NON_ONDISK);
        ppq->ondisk_logi_page = ar_mapping[ppq->ondisk_logi_page];
        //tf: BMT_GC could be fail, so addr_ondisk should determine access eu_main or eu_bkup. (this need fixing 2013/02/01)
        ppq->addr_ondisk = pNewEU->start_pos +
            (ppq->ondisk_logi_page << SECTORS_PER_FLASH_PAGE_ORDER);
        LFSM_MUTEX_UNLOCK(&(ppq->ppq_lock));
    }
    bmt->pagebmt_cn_used = bmt->pagebmt_cn_used - FPAGE_PER_SEU + cn_used_pages;
}

static bool RunGC_pagebmt(lfsm_dev_t * td, MD_L2P_table_t * bmt)
{
    int32_t *ar_mapping;
    int32_t cn_used_pages;

    ar_mapping =
        track_kmalloc(sizeof(int32_t) * FPAGE_PER_SEU, GFP_KERNEL, M_UNKNOWN);
    if (NULL == ar_mapping) {
        LOGERR("Allocating ar_mapping fail\n");
        return false;
    }
    memset(ar_mapping, 0xFF, sizeof(int32_t) * FPAGE_PER_SEU);

    cn_used_pages = 0;
    if ((cn_used_pages = copy_valid_pagebmt(td, bmt, ar_mapping)) < 0) {
        track_kfree(ar_mapping, sizeof(int32_t) * FPAGE_PER_SEU, M_UNKNOWN);
        return false;
    }

    update_valid_pagebmt_and_ppq(bmt, ar_mapping, cn_used_pages);
    LOGINFO("%s cn_used_pages=%d done\n", __FUNCTION__, cn_used_pages); // [CY]for autotest
    track_kfree(ar_mapping, sizeof(int32_t) * FPAGE_PER_SEU, M_UNKNOWN);

    return true;
}

// support always return true, mean eu is changed
static struct EUProperty *PPDM_PAGEBMT_change_EU(struct EUProperty *old_eu,
                                                 int32_t id_disk_new)
{
    struct HListGC *h;
    h = gc[old_eu->disk_num];
    LFSM_RMUTEX_LOCK(&h->whole_lock);
    if (FreeList_insert(h, old_eu) < 0) {
        LFSM_ASSERT(0);
    }

    LFSM_RMUTEX_UNLOCK(&h->whole_lock);

    //get the new_bmt_gc from freelist
    h = gc[id_disk_new];
    LFSM_RMUTEX_LOCK(&h->whole_lock);
    old_eu = FreeList_delete_with_log(&lfsmdev, h, EU_BMT, true);
    LFSM_RMUTEX_UNLOCK(&h->whole_lock);

    return old_eu;
}

//return true: need change, false: no change
bool PPDM_PAGEBMT_GCEU_disk_adjust(MD_L2P_table_t * bmt)
{
    bool ret;

    ret = false;
    LFSM_MUTEX_LOCK(&bmt->cmt_lock);
// tf: this function will only be called by updatelog logging unlock hook, which update_ondisk_bmt is reuse type.
//     those bmt eu will be changed only when bad eu happend, so that there is no eu wear-leveling in reuse type.

    if (bmt->fg_gceu_dirty) {
        bmt->BMT_EU_gc =
            PPDM_PAGEBMT_change_EU(bmt->BMT_EU_gc, bmt->BMT_EU[0]->disk_num);
        bmt->BMT_EU_backup_gc =
            PPDM_PAGEBMT_change_EU(bmt->BMT_EU_backup_gc,
                                   bmt->BMT_EU_backup[0]->disk_num);
        ret = true;
        bmt->fg_gceu_dirty = false;
    }
    LFSM_MUTEX_UNLOCK(&bmt->cmt_lock);
    return ret;
}

/* Weafon, Tifa
** This is a function for gc the pagebmt.
** there is BMT_EU_gc and BMT_EU_backup_gc logged in the dedicated map, used when gc a pagebmt eu.
** when reuse happened, old_bmt_eu put back to BMT_EU_gc; otherwise, no reuse, get new eu as BMT_EU_gc.
** 
** Returns bool: true is finish gc pagebmt, false is quit gc pagebmt
*/

// tf: only can be called by updatelog commit, make sure bmt_gc EU has been erased in unlock hook function
bool PPDM_PAGEBMT_CheckAndRunGC_Reuse(lfsm_dev_t * td, MD_L2P_table_t * bmt)
{
    if (bmt->fg_gceu_dirty) {
        if (EU_erase(bmt->BMT_EU_gc) < 0) {
            return false;
        }

        if (EU_erase(bmt->BMT_EU_backup_gc) < 0) {
            return false;
        }
    }

    if (!RunGC_pagebmt(td, bmt)) {
        LOGERR("RunGC_pagebmt fail\n");
        return false;
    }

    bmt->fg_gceu_dirty = true;
    dedicated_map_logging(td);
    return true;

}

bool PPDM_PAGEBMT_CheckAndRunGC(lfsm_dev_t * td, MD_L2P_table_t * bmt)
{
    LOGINFO("bmt_gc start\n");

    //WARNING: support BMT_EU and backup in the same disk, and BMT_EU_gc in the same disk
    if (!RunGC_pagebmt(td, bmt)) {
        LOGERR("RunGC_pagebmt fail\n");
        return false;
    }
    //put the old_bmt to freelist, get the new_bmt_gc from freelist
    bmt->BMT_EU_gc =
        PPDM_PAGEBMT_change_EU(bmt->BMT_EU_gc, bmt->BMT_EU[0]->disk_num);
    bmt->BMT_EU_backup_gc =
        PPDM_PAGEBMT_change_EU(bmt->BMT_EU_backup_gc,
                               bmt->BMT_EU_backup[0]->disk_num);
    dedicated_map_logging(td);
    return true;
}

static bool PPDM_PAGEBMT_check_allzero(sector64 * data)
{
    int32_t i;

    for (i = 0; i < FLASH_PAGE_SIZE / sizeof(sector64); i++) {
        if ((data[i]) != TAG_UNWRITE_HPAGE) {
            return false;
        }
    }
    return true;
}

static int32_t PPDM_PAGEBMT_get_bitmap_by_scan_fast(MD_L2P_table_t * bmt,
                                                    int32_t idx_fpage_bitmap)
{
    sector64 *buffer;
    sector64 addr_ondisk;
    int32_t i, j, id_eu, id_page, idx_cur_page, cn_batch, idx_page_start;
    bool data_readed;

    cn_batch = FLASH_PAGE_SIZE * BITS_PER_BYTE / READ_PER_BATCH;

    idx_page_start = idx_fpage_bitmap * FLASH_PAGE_SIZE * BITS_PER_BYTE;    // to page id
    data_readed = false;
    LOGINFO("scan to get  bitmap %d, idx_page_start=%d, cn_batch=%d\n",
            idx_fpage_bitmap, idx_page_start, cn_batch);

    if (NULL == (buffer = (sector64 *)
                 track_vmalloc(FLASH_PAGE_SIZE * READ_PER_BATCH, GFP_KERNEL,
                               M_UNKNOWN))) {
        LOGERR("Cannot alloc memory\n");
        return -ENOSPC;
    }

    for (i = 0; i < cn_batch; i++) {    //512 if 4K page
        data_readed = true;
        idx_cur_page = (idx_page_start + i * READ_PER_BATCH);

        if (idx_cur_page >= bmt->cn_ppq) {
            break;
        }

        id_eu = idx_cur_page / FPAGE_PER_SEU;
        id_page = idx_cur_page % FPAGE_PER_SEU;
        addr_ondisk =
            bmt->BMT_EU[id_eu]->start_pos + id_page * SECTORS_PER_FLASH_PAGE;

        if (!devio_read_pages
            (addr_ondisk, (int8_t *) buffer, READ_PER_BATCH, NULL)) {
            addr_ondisk =
                bmt->BMT_EU_backup[id_eu]->start_pos +
                id_page * SECTORS_PER_FLASH_PAGE;
            if (!devio_read_pages
                (addr_ondisk, (int8_t *) buffer, READ_PER_BATCH, NULL)) {
                data_readed = false;
            }
        }

        for (j = 0; j < READ_PER_BATCH; j++) {  //64
            idx_cur_page = (idx_page_start + i * READ_PER_BATCH + j);
            if (idx_cur_page >= bmt->cn_ppq) {
                break;
            }

            if (!data_readed) {
                continue;
            }

            if (PPDM_PAGEBMT_check_allzero((sector64 *)
                                           (((int8_t *) buffer) +
                                            j * FLASH_PAGE_SIZE))) {
                continue;
            }

            lfsm_set_bit(idx_cur_page, bmt->AR_PAGEBMT_isValidSegment);
        }
    }

    track_vfree(buffer, FLASH_PAGE_SIZE * READ_PER_BATCH, M_UNKNOWN);

    return 0;
}

static int32_t PPDM_PAGEBMT_get_bitmap_by_load(MD_L2P_table_t * bmt,
                                               int32_t idx_fpage_bitmap)
{
    int8_t *pbitmap;
    int32_t id_eu, id_page;

    pbitmap =
        bmt->AR_PAGEBMT_isValidSegment + idx_fpage_bitmap * FLASH_PAGE_SIZE;

    id_eu = (bmt->cn_ppq + idx_fpage_bitmap) / FPAGE_PER_SEU;   // the bitmap is following in bmt data
    id_page = (bmt->cn_ppq + idx_fpage_bitmap) % FPAGE_PER_SEU;

    //LOGINFO(" %d \n",DEV_SYS_READY(grp_ssds,bmt->BMT_EU[id_eu]->disk_num));

    //LOGINFO(" %d \n",DEV_SYS_READY(grp_ssds,bmt->BMT_EU_backup[id_eu]->disk_num));

    if (DEV_SYS_READY(grp_ssds, bmt->BMT_EU[id_eu]->disk_num) &&
        devio_read_pages(bmt->BMT_EU[id_eu]->start_pos +
                         (id_page << SECTORS_PER_FLASH_PAGE_ORDER), pbitmap, 1,
                         NULL)) {
        goto l_success;
    }

    if (DEV_SYS_READY(grp_ssds, bmt->BMT_EU_backup[id_eu]->disk_num) &&
        devio_read_pages(bmt->BMT_EU_backup[id_eu]->start_pos +
                         (id_page << SECTORS_PER_FLASH_PAGE_ORDER), pbitmap, 1,
                         NULL)) {
        goto l_success;
    }

    LOGERR("Cannot read idx_fapge_bitmap %d , id_eu %d , id_page %d\n",
           idx_fpage_bitmap, id_eu, id_page);
    return -EIO;

  l_success:
//    LOGINFO("read BMT validbitmap %llu, id %d, id_eu %d , id_page %d done \n",
//            bmt->BMT_EU[id_eu]->start_pos + (id_page<<SECTORS_PER_FLASH_PAGE_ORDER),idx_fpage_bitmap,id_eu, id_page);

    return 0;
}

static int32_t PPDM_PAGEBMT_get_valid_bitmap(lfsm_dev_t * td,
                                             MD_L2P_table_t * bmt,
                                             int32_t idx_fpage_bitmap)
{
    if (td->prev_state == sys_unloaded) {   //  valid bitmap is commited in EU
//        LOGINFO("load from bitmap %d \n",idx_fpage_bitmap);
        if (PPDM_PAGEBMT_get_bitmap_by_load(bmt, idx_fpage_bitmap) == 0) {
            return 0;
        }
    }

    return (PPDM_PAGEBMT_get_bitmap_by_scan_fast(bmt, idx_fpage_bitmap));
}

static void PPDM_PAGEBMT_ppq_set_valid(MD_L2P_table_t * bmt, int32_t id_ppq)
{
    int32_t id_eu, id_page;

    id_eu = id_ppq / FPAGE_PER_SEU;
    id_page = id_ppq % FPAGE_PER_SEU;

    bmt->per_page_queue[id_ppq].addr_ondisk =
        bmt->BMT_EU[id_eu]->start_pos +
        (id_page << SECTORS_PER_FLASH_PAGE_ORDER);

    bmt->per_page_queue[id_ppq].ondisk_logi_eu = id_eu;
    bmt->per_page_queue[id_ppq].ondisk_logi_page = id_page;
    bmt->pagebmt_cn_used++;
}

bool PPDM_PAGEBMT_rebuild(lfsm_dev_t * td, MD_L2P_table_t * bmt)
{
    int32_t i, idx_fpage_bitmap, idx_ppq, cn_fpage_valid_bitmap;

    cn_fpage_valid_bitmap = bmt_cn_fpage_valid_bitmap(bmt);
    LOGINFO(" cn_ppq = %d ,cn_fpage_valid_bitmap %d \n", bmt->cn_ppq,
            cn_fpage_valid_bitmap);

    // load the valid bitmap into bmt->AR_PAGEBMT_isValidSegment
    for (idx_fpage_bitmap = 0; idx_fpage_bitmap < cn_fpage_valid_bitmap;
         idx_fpage_bitmap++) {
        if (PPDM_PAGEBMT_get_valid_bitmap(td, bmt, idx_fpage_bitmap) < 0) {
            LOGERR("cannot load %d fpage_bitmap \n", idx_fpage_bitmap);
            return false;
        }
    }

    for (idx_ppq = 0; idx_ppq < bmt->cn_ppq; idx_ppq++) {
        if (lfsm_test_bit(idx_ppq, bmt->AR_PAGEBMT_isValidSegment) == 0) {  // bitmap rsult is at bmt->AR_PAGEBMT_isValidSegment
            continue;
        }
        PPDM_PAGEBMT_ppq_set_valid(bmt, idx_ppq);
    }

    // to set the bitmap field  to make rescue copy complete
    for (i = bmt->cn_ppq; i < bmt->cn_ppq + cn_fpage_valid_bitmap; i++) {
        lfsm_set_bit(i, bmt->AR_PAGEBMT_isValidSegment);
    }

    return true;
}

static bool PPDM_BMT_read_page(sector64 pos, int8_t * bmt_rec)
{
    if (!DEV_SYS_READY(grp_ssds, eu_find(pos)->disk_num)) {
        return false;
    }

    return devio_read_pages(pos, (int8_t *) bmt_rec, 1, NULL);
}

int32_t PPDM_BMT_load(lfsm_dev_t * td, int32_t idx_ppq, int8_t * bmt_rec)
{
    MD_L2P_table_t *bmt;
    sector64 bmt_addr;

    bmt = td->ltop_bmt;
    if (!PPDM_BMT_read_page
        (bmt->per_page_queue[idx_ppq].addr_ondisk, (int8_t *) bmt_rec)) {
        //read page_bmt from backup eu
        bmt_addr =
            bmt->BMT_EU_backup[bmt->per_page_queue[idx_ppq].ondisk_logi_eu]->
            start_pos +
            (bmt->per_page_queue[idx_ppq].
             ondisk_logi_page << SECTORS_PER_FLASH_PAGE_ORDER);
        if (bmt->per_page_queue[idx_ppq].addr_ondisk == bmt_addr) { // it's allready load from backup
            return -EIO;
        }
        LOGINFO("read from backup bmt %llu\n", bmt_addr);
        if (!PPDM_BMT_read_page(bmt_addr, (int8_t *) bmt_rec)) {
            LOGERR("%s: devio_read_pages fail\n", __FUNCTION__);
            return -EIO;
        }
    }
    return 0;
}
