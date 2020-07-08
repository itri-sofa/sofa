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
#include <linux/spinlock.h>
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
#include "lfsm_core.h"          //no need
#include "conflict.h"
#include "io_manager/io_common.h"

/*
 * DESCRIPTION: set value (0 or 1) to lpn correspond bit in a byte vector
 * INPUT:
 *     PARAMETERS:
 *         int8_t *ar_v: byte vector to set
 *         int32_t idx : lpn location in a ppq bucket : lpn % 8
 *         int32_t v   : value to set: 0 or 1
 * NOTES: idx >> 3 = idx / 8  idx & 0x07 => idx % 8
 */
static void bvector_set(int8_t * ar_v, int32_t idx, int32_t v)
{
    if (v > 0) {
        ar_v[idx >> 3] |= ((uint8_t) 1) << (idx & 0x07);
    } else {
        ar_v[idx >> 3] &= ~(((uint8_t) 1) << (idx & 0x07));
    }
}

/*
 * DESCRIPTION: get value (0 or 1) to lpn correspond bit in a byte vector
 * INPUT:
 *     PARAMETERS:
 *         int8_t *ar_v: byte vector to set
 *         int32_t idx : lpn location in a ppq bucket : lpn % 8
 * OUTPUTS:
 *     RETURN :
 *         Value: 0 or 1 to indicate whether conflict access
 * NOTES: idx >> 3 = idx / 8  idx & 0x07 => idx % 8
 */
static int32_t bvector_get(int8_t * ar_v, int32_t idx)
{
    return (ar_v[idx >> 3] & ((uint8_t) 1) << (idx & 0x07)) > 0 ? 1 : 0;
}

/*
 * DESCRIPTION: check whether bioc in conflict list is waiting for free seu
 * INPUT:
 *     PARAMETERS:
 *         sector64 lpn: current working lpn
 *         MD_L2P_item_t *ppq_head: ppq list head
 *         int32_t id_ppq: index of ppq
 * OUTPUTS:
 *     RETURN :
 *         Value: the giveup bioc
 */
static bioconter_t *conflict_search_waitSEU(sector64 lpn,
                                            MD_L2P_item_t * ppq_head,
                                            int32_t id_ppq)
{
    bioconter_t *bioc_curr, *bioc_next, *bioc_ret;

    bioc_ret = NULL;
    list_for_each_entry_safe(bioc_curr, bioc_next,
                             &ppq_head[id_ppq].conflict.confl_list,
                             arhook_conflict[id_ppq % MAX_CONFLICT_HOOK]) {
        if ((lpn >= bioc_curr->start_lbno) && (lpn <= bioc_curr->end_lbno)) {
            if (bioc_curr->fg_pending == FG_IO_WAIT_FREESEU) {
                bioc_ret = bioc_curr;
            }

            break;
        }
    }

    return bioc_ret;
}

/*
 * DESCRIPTION: set all conflict bioc as waiting SEU
 * INPUT:
 *     PARAMETERS:
 *         struct bio_container *pbioc
 *         MD_L2P_item_t *ppq_head: ppq list head
 *         io_pending_t new_fg_pend: new pending state
 *         int32_t level: debug purpose. record stack level
 * TODO: should be called under lock: if not at the same ppq bucket, won't hold lock
 * TODO: try to avoid recursive call....!!!!
 */
static void conflict_set_waitSEU(struct bio_container *pbioc,
                                 MD_L2P_item_t * ppq_head,
                                 io_pending_t new_fg_pend, int32_t level)
{
    struct bio_container *pbioc_curr, *pbioc_next;
    sector64 lpn;
    int32_t i, id_ppq, cn;

    cn = bioc_cn_fpage(pbioc);

    //LOGINFO("[%d] lpn %lld cn %d set giveup %d %p\n",level, pbioc->start_lbno,cn,new_fg_giveup,pbioc);
    for (i = 0; i < cn; i++) {
        lpn = pbioc->start_lbno + i;
        id_ppq = lpn / DBMTE_NUM;
        list_for_each_entry_safe(pbioc_curr, pbioc_next,
                                 &ppq_head[id_ppq].conflict.confl_list,
                                 arhook_conflict[id_ppq % MAX_CONFLICT_HOOK]) {
            if ((lpn < pbioc_curr->start_lbno) || (lpn > pbioc_curr->end_lbno)) {
                continue;
            }
            // the following situation happens when conflict_set_waitSEU is called recursively
            // pbioc_curr is used as the parameter pbioc of conflict_set_waitSEU() under recursive case
            if (pbioc == pbioc_curr) {
                continue;
            }
            //printk("logout: bioc(%p) lpn %llu id_ppq %d cf:%d\n",pbioc,lpn, id_ppq, atomic_read(&pbioc_curr->conflict_pages));
            if (pbioc_curr->fg_pending == (FG_IO_WAIT_FREESEU - new_fg_pend)) {
                pbioc_curr->fg_pending = new_fg_pend;
                //LOGINFO("[%d] --- set %p give lpn %lld %d\n",level, pbioc_curr,pbioc_curr->start_lbno,new_fg_pend);
                conflict_set_waitSEU(pbioc_curr, ppq_head, new_fg_pend,
                                     level + 1);
                break;
            }
        }
    }
    return;
}

/*
 * DESCRIPTION: dump all vector of lfsm to syslog
 * INPUT:
 *     PARAMETERS:
 *         lfsm_dev_t* td: lfsm device
 *         int32_t cn: how many ppq bucket to show
 */
void conflict_vec_dumpall(lfsm_dev_t * td, int32_t cn)
{
    struct bio_container *pbioc_curr, *pbioc_next;
    MD_L2P_item_t *ppq_head;
    int32_t i, j, v;

    ppq_head = td->ltop_bmt->per_page_queue;
    LOGINFO("\n***** dump conflict vector start *****\n");
    for (i = 0; i < cn; i++) {
        for (j = 0; j < SZ_BVEC; j++) {
            if ((v = bvector_get(ppq_head[i].conflict.bvec, j)) != 0) {
                printk("%d,%d:%x\n", i, j, v);
            }
        }

        list_for_each_entry_safe(pbioc_curr, pbioc_next,
                                 &ppq_head[i].conflict.confl_list,
                                 arhook_conflict[i % MAX_CONFLICT_HOOK]) {
            printk("%lld %lld @%p\n", pbioc_curr->start_lbno,
                   pbioc_curr->end_lbno - pbioc_curr->start_lbno + 1,
                   pbioc_curr);
        }
    }
    LOGINFO("***** dump conflict vector end *****\n\n");
    return;
}

/*
 * DESCRIPTION: login 1 lpn of GC bioc
 * INPUTS :
 *     PARAMETERS:
 *         sector64 lpn: target LPN GC IO want to handle
 *         MD_L2P_item_t *ppq_head: ppq head
 *         int32_t id_ppq: index of ppq bucket which lpn belong to
 * OUTPUTS:
 *     RETURN :
 *         Value: 0: no conflict
 *                1: conflict
 *                -ENOENT: if conflict and is GC
 */
static int32_t _conf_gcbioc_login(sector64 lpn, MD_L2P_item_t * ppq_head,
                                  int32_t id_ppq)
{
    struct bio_container *pbioc_giveup;
    static sector64 lpn_abort = 0;
    static int32_t cn_abort_print = 0;
    int32_t ret;

    ret = 0;

    if (NULL == (pbioc_giveup = conflict_search_waitSEU(lpn, ppq_head, id_ppq))) {
        if (lpn_abort == lpn) {
            cn_abort_print++;
        } else {
            lpn_abort = lpn;
            cn_abort_print = 0;
        }

        if (cn_abort_print < 10) {
            LOGINFO("gc bioc abort since %lld is not giveup\n", lpn);
        }

        ret = -ENOENT;
    } else {
        CONFLICT_COUNTER_INC_RETURN(pbioc_giveup, id_ppq);
    }

    return ret;
}

static void _conf_bioc_login(struct bio_container *pbioc, sector64 lpn,
                             MD_L2P_item_t * ppq_head, int32_t id_ppq)
{
    int32_t index;

    CONFLICT_COUNTER_INC_RETURN(pbioc, id_ppq);
    //printk("login: bioc(%p) lpn %llu id_ppq %d cf: %d\n",pbioc,lpn, id_ppq, atomic_read(&pbioc->conflict_pages));

    index = id_ppq % MAX_CONFLICT_HOOK;
    if (list_empty(&pbioc->arhook_conflict[index])) {
        pbioc->fg_pending =
            (conflict_search_waitSEU(lpn, ppq_head, id_ppq) == NULL) ?
            FG_IO_NO_PEND : FG_IO_WAIT_FREESEU;
        list_add_tail(&pbioc->arhook_conflict[index],
                      &ppq_head[id_ppq].conflict.confl_list);
    } else {
        LOGWARN(" arhook_conflict of bio container not empty\n");
    }
}

/*
 * DESCRIPTION: check whether current IO access conflict lpn with previous IO
 *              if no, set conflict bit as 1, if yes, add to ppq bucket's confl_list
 * INPUTS :
 *     PARAMETERS:
 *         struct bio_container *pbioc: the new IO comer
 *         lfsm_dev_t *td: lfsm device
 *         isGCIO: non-zero means the pbioc is not comes from GC module
 *                 zero     means the pbioc is aim for user IO
 * OUTPUTS:
 *     RETURN :
 *         Value: 0: no conflict
 *                1: conflict
 *                -ENOENT: if conflict and is GC
 * PROCESS:
 *     [1] for each lpn in bioc
 *     [2]     check whether conflict with someone, if no, set conflict bit as 1
 *     [3]     if yes, add to ppq bucket's confl_list
 */
int32_t conflict_bioc_login(struct bio_container *pbioc, lfsm_dev_t * td,
                            int32_t isGCIO)
{
    MD_L2P_item_t *ppq_head;
    sconflict_t *myconfl;
    sector64 lpn;
    unsigned long flag;
    int32_t ret, i, id_ppq, idx, len;

    ret = 0;
    lpn = pbioc->start_lbno;
    len = bioc_cn_fpage(pbioc);
    id_ppq = lpn / DBMTE_NUM;
    idx = lpn % DBMTE_NUM;

    ppq_head = td->ltop_bmt->per_page_queue;
    myconfl = &ppq_head[id_ppq].conflict;

    spin_lock_irqsave(&myconfl->confl_lock, flag);
    for (i = 0; i < len; i++) {
        if (bvector_get(myconfl->bvec, idx) == 0) {
            bvector_set(myconfl->bvec, idx, 1);
        } else {
            if (isGCIO == 1) {  // for gc bioc, then we return -ENOENT if it conflicts with previous biocs.
                ret = _conf_gcbioc_login(lpn + i, ppq_head, id_ppq);
                if (ret == -ENOENT) {
                    goto l_unlock;
                }
            } else {            // for non-gc bioc
                _conf_bioc_login(pbioc, lpn + i, ppq_head, id_ppq);
                ret = 1;
            }
        }

        //TODO: if len is 1, should we do the same thing to speed up perf?? no for loop in binary code
        idx++;
        if (idx == DBMTE_NUM) {
            spin_unlock_irqrestore(&myconfl->confl_lock, flag);
            idx = 0;
            id_ppq++;
            myconfl = &ppq_head[id_ppq].conflict;
            spin_lock_irqsave(&myconfl->confl_lock, flag);
        }
    }

  l_unlock:
    spin_unlock_irqrestore(&myconfl->confl_lock, flag);

    return ret;
}

/*
 * DESCRIPTION: mark all bioc with waiting free SEU as no pending
 * INPUTS :
 *     PARAMETERS:
 *         struct bio_container *pbioc: current IO that waiting for free SEU
 *         lfsm_dev_t *td: lfsm device
 * OUTPUTS:
 *     RETURN :
 *         Value: 0: make all bioc as non-pending done
 */
int32_t conflict_bioc_mark_nopending(struct bio_container *pbioc,
                                     lfsm_dev_t * td)
{
    MD_L2P_item_t *ppq_head;
    sconflict_t *myconf;
    sector64 lpn;
    unsigned long flag;
    int32_t i, id_ppq, len, index;

    ppq_head = td->ltop_bmt->per_page_queue;
    lpn = pbioc->start_lbno;
    len = bioc_cn_fpage(pbioc);

    for (i = 0; i < len; i++) {
        id_ppq = (lpn + i) / DBMTE_NUM;
        index = id_ppq % MAX_CONFLICT_HOOK;

        myconf = &ppq_head[id_ppq].conflict;
        spin_lock_irqsave(&myconf->confl_lock, flag);
        if (!list_empty(&pbioc->arhook_conflict[index])) {
            list_del_init(&pbioc->arhook_conflict[index]);
            CONFLICT_COUNTER_DEC_RETURN(pbioc, id_ppq);
            conflict_set_waitSEU(pbioc, ppq_head, 0, 0);
            pbioc->fg_pending = FG_IO_NO_PEND;
        }
        spin_unlock_irqrestore(&myconf->confl_lock, flag);
    }

    return 0;
}

/*
 * DESCRIPTION: when a bioc encounter a run-out of free eu, the bioc will call the following function
 *              to hang itself into the conflict list and set fg_pending = FG_IO_WAIT_FREESEU
 *              and recursively set the fg_pending of all bioc conflicted with itself to FG_IO_WAIT_FREESEU
 * INPUTS :
 *     PARAMETERS:
 *         struct bio_container *pbioc: current IO that waiting for free SEU
 *         lfsm_dev_t *td: lfsm device
 * OUTPUTS:
 *     RETURN :
 *         Value: 0: make all bioc as non-pending done
 */
// when a bioc encounter a run-out of free eu, the bioc will call the following function
// to hang itself into the conflict list and set giveup = 1
// and recursively set the giveup of all bioc conflicted with itself to 1
int32_t conflict_bioc_mark_waitSEU(struct bio_container *pbioc, lfsm_dev_t * td)
{
    MD_L2P_item_t *ppq_head;
    sconflict_t *myconfl;
    sector64 lpn;
    unsigned long flag;
    int32_t i, id_ppq, len, index;

    ppq_head = td->ltop_bmt->per_page_queue;
    lpn = pbioc->start_lbno;
    len = bioc_cn_fpage(pbioc);

    for (i = 0; i < len; i++) {
        id_ppq = (lpn + i) / DBMTE_NUM;
        index = id_ppq % MAX_CONFLICT_HOOK;

        myconfl = &ppq_head[id_ppq].conflict;
        spin_lock_irqsave(&myconfl->confl_lock, flag);

        if (list_empty(&pbioc->arhook_conflict[index])) {
            conflict_set_waitSEU(pbioc, ppq_head, 1, 0);
            list_add(&pbioc->arhook_conflict[index], &myconfl->confl_list);
            pbioc->fg_pending = FG_IO_WAIT_FREESEU;
            CONFLICT_COUNTER_INC_RETURN(pbioc, id_ppq);
        }

        spin_unlock_irqrestore(&myconfl->confl_lock, flag);
    }

    return 0;
}

/*
 * DESCRIPTION: ongoing IO finish IO process, set bit as 0
 * INPUTS :
 *     PARAMETERS:
 *         struct bio_container *pbioc: ongoing bioc that already finish it's IO
 *         lfsm_dev_t *td: lfsm device
 *         int32_t len_specific: non-zero means the pbioc is not comes from GC module
 *                 zero     means the pbioc is aim for user IO
 * OUTPUTS:
 *     RETURN :
 *         Value: 0: no conflict
 *                1: conflict
 *                -ENOENT: if conflict and is GC
 * PROCESS:
 *     [1] for each lpn in bioc
 *     [2]     check whether conflict with someon, if no, set conflict bit as 1
 *     [3]     if yes, add to ppq bucket's confl_list
 */
int32_t conflict_bioc_logout(struct bio_container *pbioc, lfsm_dev_t * td)
{
    struct bio_container *pbioc_curr, *pbioc_next;
    MD_L2P_item_t *ppq_head;
    sconflict_t *myconfl;
    sector64 lpn;
    unsigned long flag;
    int32_t i, id_ppq, idx, cn, len, fg_exist, index;

    ppq_head = td->ltop_bmt->per_page_queue;
    lpn = pbioc->start_lbno;
    len = bioc_cn_fpage(pbioc);
    fg_exist = 0;

    id_ppq = lpn / DBMTE_NUM;
    idx = lpn % DBMTE_NUM;

    for (i = 0; i < len; i++) {
        index = id_ppq % MAX_CONFLICT_HOOK;
        myconfl = &ppq_head[id_ppq].conflict;

        spin_lock_irqsave(&myconfl->confl_lock, flag);
        if (pbioc->fg_pending) {
            LOGWARN("pbioc %p tp %d lpn %lld sz %d fg_pending stick to 1\n",
                    pbioc, pbioc->write_type, pbioc->start_lbno,
                    bioc_cn_fpage(pbioc));
            //spin_unlock_irqrestore(&ar_ppq[id_ppq].conflict.lock, flag);
            //LFSM_ASSERT(0);
        }

        list_for_each_entry_safe(pbioc_curr, pbioc_next,
                                 &myconfl->confl_list, arhook_conflict[index]) {
            if (((lpn + i) >= pbioc_curr->start_lbno)
                && ((lpn + i) <= pbioc_curr->end_lbno)) {
                //printk("logout: bioc(%p) lpn %llu id_ppq %d cf:%d\n",pbioc,lpn, id_ppq, atomic_read(&pbioc_curr->conflict_pages));
                cn = CONFLICT_COUNTER_DEC_RETURN(pbioc_curr, id_ppq);
                if (CONFLICT_COUNTER_ZERO(cn, id_ppq)) {
                    //NOTE: we may not skip this, since we use list_empty to check
                    list_del_init(&pbioc_curr->arhook_conflict[index]);
                }

                //printk("wake up bioc(%p) lpn %llu len %d id_ppq %d \n",pbioc,lpn, len, id_ppq);
                if (CONFLICT_COUNTER_ALLZERO(cn)) {
                    wake_up_interruptible(&pbioc_curr->io_queue);
                }

                fg_exist = 1;
                break;
            }
        }

        if (fg_exist == 0) {
            bvector_set(myconfl->bvec, idx, 0);
        }
        spin_unlock_irqrestore(&myconfl->confl_lock, flag);

        fg_exist = 0;
        // use add and if to prevent from wasting cpu resources on div and mod
        idx++;
        if (idx == DBMTE_NUM) {
            idx = 0;
            id_ppq++;
        }
    }

    return 0;
}

int32_t conflict_gc_abort_helper(struct bio_container *bioc, lfsm_dev_t * td)
{
    struct bio_container *tmp_bio_c, *tmp_bio_c_next;
    int32_t cn_drop;

    cn_drop = 0;

    //TODO: using another lock to protect gc_bioc_list, avoiding lock long
    spin_lock(&td->datalog_list_spinlock[bioc->idx_group]);
    list_for_each_entry_safe(tmp_bio_c, tmp_bio_c_next, &td->gc_bioc_list, list) {
        if (tmp_bio_c->write_type == TP_GC_IO) {
            conflict_bioc_logout(tmp_bio_c, td);
            atomic_set(&tmp_bio_c->active_flag, ACF_IDLE);
            free_all_per_page_list_items(td, tmp_bio_c);
            put_bio_container_nolock(td, tmp_bio_c, tmp_bio_c->start_lbno);
            cn_drop++;
        }
    }
    spin_unlock(&td->datalog_list_spinlock[bioc->idx_group]);
    return cn_drop;
}
