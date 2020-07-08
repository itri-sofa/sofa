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
#include "metalog.h"
#include "EU_access.h"
#include "io_manager/io_common.h"
#include "mdnode.h"
#include "spare_bmt_ul.h"
#include "EU_create.h"
#ifdef THD_EXEC_STEP_MON
#include "lfsm_thread_monitor.h"
#endif

///////////////////////////// //////////////////////////////////////////////////////////
// Comment from Weafon, 20131023
// 0. to support metalog, smetalog_host is add to eu_property dynamically
// 1. fg IO/GC thread call metalog_packet_register() to append lpn-ppn into the metalog packet
// 2. the low-level function of metalog_async_write() may call metalog_packet_active_eus()
//    to force log the pending lpn-ppn of all active eus once if the last eu of metalog is used
//
//                    wakeup
//        |        <-----    ---------        trigger_action() <-----    async_write_bh()         <----       |
//        +                        packet                    done                                       |
//[process_deuqeue()@mgrfg] ------------->[ eu_log ]----------> [metalog_process_bh()@mlmgr_bh] ---|
//        +                    async_write()                                                           |
//        |        <-----    ---------                        <-----    async_write_event_bh()     <----       |
//[process_deuqeue()@mgrfg].....
//[process_deuqeue()@mgrfg].....
////////////////////////////////////////////////////////////////////////////////////////////

static int32_t metalog_create_thread(struct task_struct **ppthread,
                                     int (*threadfunc)(void *arg), void *arg,
                                     int8_t * name)
{
    *ppthread = kthread_run(threadfunc, arg, name);
    if (IS_ERR_OR_NULL(*ppthread)) {
        return -ENOMEM;
    }
#ifdef THD_EXEC_STEP_MON
    reg_thread_monitor((*ppthread)->pid, name);
#endif
    return 0;
}

static void metalog_destroy_thread(struct task_struct **ppthread)
{
    if (IS_ERR_OR_NULL(*ppthread)) {
        return;
    }
    if (kthread_stop(*ppthread) < 0) {
        LOGERR("can't stop mlmgr pthread\n");
    }

    *ppthread = 0;
    return;
}

static int32_t metalog_mgrfg_init(smetalog_mgr_fg_t * pmgr_fg,
                                  int (*threadfunc)(void *arg), void *arg,
                                  int8_t * name)
{
    spin_lock_init(&pmgr_fg->slock);
    atomic_set(&pmgr_fg->cn_full, 0);
    init_waitqueue_head(&pmgr_fg->wq_enqueue);
    init_waitqueue_head(&pmgr_fg->wq_dequeue);
    INIT_LIST_HEAD(&pmgr_fg->list_full);
    pmgr_fg->ppacket_act = NULL;
    atomic_set(&pmgr_fg->state, 1);
    pmgr_fg->sz_submit = 0;
    pmgr_fg->cn_submit = 0;
    atomic_set(&pmgr_fg->cn_pkt_bf_swap, 0);
    if (metalog_create_thread(&pmgr_fg->pthread, threadfunc, arg, name) < 0) {
        return -ENOMEM;
    }

    return 0;
}

static int32_t metalog_mgr_init(smetalog_mgr_t * pmgr,
                                int (*threadfunc)(void *arg), void *arg,
                                int8_t * name)
{
    spin_lock_init(&pmgr->slock);
    atomic_set(&pmgr->cn_act, 0);
    init_waitqueue_head(&pmgr->wq);
    INIT_LIST_HEAD(&pmgr->list_act);
    if (metalog_create_thread(&pmgr->pthread, threadfunc, arg, name) < 0) {
        return -ENOMEM;
    }
    return 0;
}

void metalog_eulog_initA(smetalog_eulog_t * peulog, int32_t id_disk,
                         bool fg_insert_old)
{
    int32_t i;

    peulog->seq = 1;
    peulog->idx = 0;
    LOGINFO("metalog[%d]: ", id_disk);
    LFSM_RMUTEX_LOCK(&gc[id_disk]->whole_lock);
    for (i = 0; i < MLOG_EU_NUM; i++) {
        if (fg_insert_old) {
            FreeList_insert(gc[id_disk], peulog->eu[i]);
        }
        peulog->eu[i] = FreeList_delete(gc[id_disk], EU_MLOG, true);
        printk("eu(%llu), ", peulog->eu[i]->start_pos);
    }
    LFSM_RMUTEX_UNLOCK(&gc[id_disk]->whole_lock);
    printk("\n");
    return;
}

static void metalog_eulog_destroy(smetalogger_t * pmlogger)
{
    if (pmlogger->par_eulog) {
        track_kfree(pmlogger->par_eulog,
                    sizeof(smetalog_eulog_t) * pmlogger->td->cn_ssds,
                    M_METALOG);
    }
}

int32_t metalog_eulog_reinit(smetalogger_t * pmlogger, int32_t cn_ssd_orig)
{
    smetalog_eulog_t *par_eulog_new, *par_eulog_orig;

    par_eulog_new = (smetalog_eulog_t *)
        track_kmalloc(sizeof(smetalog_eulog_t) * pmlogger->td->cn_ssds,
                      GFP_KERNEL, M_METALOG);

    if (IS_ERR_OR_NULL(par_eulog_new)) {
        return -ENOMEM;
    }
    memset(par_eulog_new, 0, sizeof(smetalog_eulog_t) * pmlogger->td->cn_ssds);
    memcpy(par_eulog_new, pmlogger->par_eulog,
           sizeof(smetalog_eulog_t) * cn_ssd_orig);

    par_eulog_orig = pmlogger->par_eulog;
    pmlogger->par_eulog = par_eulog_new;

    track_kfree(par_eulog_orig, sizeof(smetalog_eulog_t) * cn_ssd_orig,
                M_METALOG);

    return 0;

}

static int32_t metalog_eulog_init(smetalogger_t * pmlogger)
{
    pmlogger->par_eulog = (smetalog_eulog_t *)
        track_kmalloc(sizeof(smetalog_eulog_t) * pmlogger->td->cn_ssds,
                      GFP_KERNEL, M_METALOG);

    if (IS_ERR_OR_NULL(pmlogger->par_eulog)) {
        return -ENOMEM;
    }
    memset(pmlogger->par_eulog, 0,
           sizeof(smetalog_eulog_t) * pmlogger->td->cn_ssds);
    return 0;
}

int32_t metalog_eulog_reset(smetalogger_t * pmlogger, struct HListGC **gc,
                            bool fg_insert_old)
{
    int32_t i;

    if (lfsm_readonly == 1) {
        return 0;
    }
    for (i = 0; i < pmlogger->td->cn_ssds; i++) {
        metalog_eulog_initA(&pmlogger->par_eulog[i], i, fg_insert_old);
    }

    return 0;
}

static int32_t metalog_packet_has_metadata(smetalog_mgr_fg_t * pmgr_fg)
{
    return (!IS_ERR_OR_NULL(pmgr_fg->ppacket_act) &&
            (atomic_read(&pmgr_fg->ppacket_act->cn_pending) > 0));
}

static int32_t metalog_dumpstat(smetalog_mgr_fg_t * pmgr_fg, int32_t cn_pending,
                                int32_t id_disk)
{
    pmgr_fg->sz_submit += cn_pending;
    pmgr_fg->cn_submit++;
    if (pmgr_fg->cn_submit % 1000 == 0) {
//        printk("[%d] sz %d in round %d\n", id_disk, mlmgr->sz_submit, mlmgr->cn_submit);
        pmgr_fg->sz_submit = 0;
        pmgr_fg->cn_submit = 0;
    }

    return 0;
}

static int32_t metalog_eulog_change(smetalog_eulog_t * peulog,
                                    smetalog_mgr_fg_t * pmgr_fg,
                                    struct HListGC *h, lfsm_dev_t * td)
{
    struct EUProperty *eu_new;
    int32_t i;

    if (lfsm_readonly == 1) {
        return 0;
    }

    if (peulog->idx != MLOG_EU_NUM - 1) {
        return 0;
    }

    if (atomic_read(&pmgr_fg->cn_pkt_bf_swap) != 0) {
        return 0;
    }
    //// get new eu
    LFSM_RMUTEX_LOCK(&h->whole_lock);
    eu_new = FreeList_delete(h, EU_MLOG, true);
    FreeList_insert(h, peulog->eu[0]);
    LFSM_RMUTEX_UNLOCK(&h->whole_lock);

    /// swap eu
    for (i = 1; i < MLOG_EU_NUM; i++) {
        peulog->eu[i - 1] = peulog->eu[i];
    }

    peulog->eu[MLOG_EU_NUM - 1] = eu_new;
    peulog->idx--;

    dedicated_map_logging(td);
    return 0;
}

static void metalog_packet_free(smetalogger_t * pmlogger,
                                smetalog_pkt_t * pmlpacket)
{
    if (pmlpacket) {
        kmem_cache_free(pmlogger->pmlpacket_pool, pmlpacket);
    }
    return;
}

struct EUProperty *pbno_return_eu_idx(sector64 pbn, int32_t * ret_idx)
{
    struct EUProperty *peu;
    peu = eu_find(pbn);
    if (NULL == peu) {
        return NULL;
    }
    *ret_idx = (pbn >> SECTORS_PER_FLASH_PAGE_ORDER) % FPAGE_PER_SEU;
    return peu;
}

static int32_t metalog_async_write_bh(smetalog_pkt_t * pmlpacket)
{
    struct EUProperty *act_eu;
    smetalog_info_t *ar_info;
    lfsm_dev_t *td;
    int32_t i, idx, ret;

    idx = 0;
    ret = 0;
    td = pmlpacket->pmlogger->td;
    ar_info = pmlpacket->ar_info;

    for (i = 0; i < atomic_read(&pmlpacket->cn_pending); i++) {
        if (NULL == pmlpacket->ar_pbioc[i]) {   // imply erase mark
            continue;
        }

        if (NULL == (act_eu = pbno_return_eu_idx(ar_info[i].x.pbn, &idx))) {
            LOGERR("packet(%p) idx %d pbioc(%p) fail {lpn,pbn} {%llu, %llu}\n",
                   pmlpacket, i, pmlpacket->ar_pbioc[i], ar_info[i].x.lpn,
                   ar_info[i].x.pbn);
            ret = -EINVAL;
            continue;
        }
    }
    atomic_set(&pmlpacket->pmlogger->par_mlmgr[pmlpacket->id_disk].state, 1);
    wake_up_interruptible(&pmlpacket->pmlogger->par_mlmgr[pmlpacket->id_disk].
                          wq_dequeue);

    metalog_packet_free(pmlpacket->pmlogger, pmlpacket);
    return ret;
}

static int32_t metalog_mgr_list_insert(smetalog_mgr_t * pmgr_bh,
                                       smetalog_pkt_t * mlpacket)
{
    unsigned long flag;
    spin_lock_irqsave(&pmgr_bh->slock, flag);
    list_add_tail(&mlpacket->hook, &pmgr_bh->list_act);
    atomic_inc(&pmgr_bh->cn_act);
    spin_unlock_irqrestore(&pmgr_bh->slock, flag);
    wake_up_interruptible(&pmgr_bh->wq);
    return 0;
}

static void metalog_end_bio(struct bio *pbio)
{
    int32_t idx;
    smetalog_pkt_t *pmlpacket;
    int32_t err;

    pmlpacket = pbio->bi_private;
    err = blk_status_to_errno(pbio->bi_status);
    if (err) {
        LOGERR("packet(%p) pbio(%p) sector %llu err %d\n", pmlpacket, pbio,
               (sector64) pbio->bi_iter.bi_sector, err);
        pmlpacket->err = err;
    }

    idx =
        (lfsm_atomic_inc_return(&pmlpacket->pmlogger->cn_bh) %
         MLOG_BH_THREAD_NUM);
//    idx =  (mlpacket->id_disk % MLOG_BH_THREAD_NUM);
    metalog_mgr_list_insert(&pmlpacket->pmlogger->mlmgr_bh[idx], pmlpacket);
    free_composed_bio(pbio, pbio->bi_vcnt);
    return;
}

static void metalog_bv_bio_data(struct bio *pBio, smetalog_pkt_t * ppacket,
                                int32_t cn_hpage, int32_t size, int32_t seq)
{
    smetalog_info_t *pinfo;
    int8_t *buf;
    int32_t i, idx_source, idx_dest, size_cur;

    idx_source = 0;
    idx_dest = 0;

    for (i = 0; i < cn_hpage * MEM_PER_HARD_PAGE; i++) {
        if (i % MEM_PER_HARD_PAGE == 0) {
            size_cur = (size < PAGE_SIZE - sizeof(smetalog_info_t)) ?
                size : PAGE_SIZE - sizeof(smetalog_info_t);
        } else {
            size_cur = (size < PAGE_SIZE) ? size : PAGE_SIZE;
        }

        buf = kmap(pBio->bi_io_vec[i].bv_page); // default page buffer is zero patten
        pinfo = (smetalog_info_t *) buf;
        if (i % MEM_PER_HARD_PAGE == 0) {
            pinfo[0].seq = seq;
            idx_dest = 1;
        } else {
            idx_dest = 0;
        }
        memcpy(&pinfo[idx_dest], &ppacket->ar_info[idx_source], size_cur);
        kunmap(pBio->bi_io_vec[i].bv_page);

        idx_source += size_cur / sizeof(smetalog_info_t);
        size -= size_cur;
    }

    return;
}

static smetalog_pkt_t *metalog_packet_alloc_init(smetalogger_t * pmlogger,
                                                 int32_t id_disk, int32_t tp)
{
    smetalog_pkt_t *packet;
    int32_t i;

    if (IS_ERR_OR_NULL
        (packet = kmem_cache_alloc(pmlogger->pmlpacket_pool, GFP_KERNEL))) {
        return NULL;
    }

    INIT_LIST_HEAD(&packet->hook);
    for (i = 0; i < MLOG_BATCH_NUM; i++) {
        packet->ar_pbioc[i] = NULL;
        packet->ar_info[i].x.pbn = 0;
        packet->ar_info[i].x.lpn = 0;
    }

    packet->pmlogger = pmlogger;
    atomic_set(&packet->cn_pending, 0);
    packet->id_disk = id_disk;
    packet->tp = tp;
    packet->err = 0;
    return packet;
}

// // metalog_packet_append(...)
// append an lpn-ppn relationship into the tail of a packet
//return the idx of used
static int32_t metalog_packet_setup(smetalog_pkt_t * ppacket,
                                    struct bio_container *pbioc, sector64 pbn,
                                    sector64 lpn)
{
    int32_t idx;

    if ((idx = atomic_read(&ppacket->cn_pending)) >= MLOG_BATCH_NUM) {
        return -EPERM;
    }

    ppacket->ar_pbioc[idx] = pbioc;
    ppacket->ar_info[idx].x.pbn = pbn;
    ppacket->ar_info[idx].x.lpn = lpn;
    atomic_inc(&ppacket->cn_pending);
    return idx;
}

static int32_t metalog_mgrfg_list_insert(smetalog_mgr_fg_t * pmgr_fg,
                                         smetalog_pkt_t * ppacket)
{
    unsigned long flag;
    spin_lock_irqsave(&pmgr_fg->slock, flag);
    list_add_tail(&ppacket->hook, &pmgr_fg->list_full);
    atomic_inc(&pmgr_fg->cn_full);
    spin_unlock_irqrestore(&pmgr_fg->slock, flag);
    wake_up_interruptible(&pmgr_fg->wq_dequeue);
    return 0;
}

//    printk("pack(%p) pbn %llu, cn %d, event %d\n",ppack, pbn, i, cn_packet);
// A disk only has one thread to do metalog_packet_eu.
// 1. mdnode_set_state_copy only can happend here
// 2. when tp is ml_event, active_eu packet should be all finish created before they are submitted
//    (ie. create packet and submit is in the same thread)
// 3. return value is cn_packet_page
static int32_t metalog_packet_eu(smetalogger_t * pmlogger,
                                 struct EUProperty *eu, int32_t idx_start,
                                 int32_t tp)
{
    smetalog_pkt_t *ppack;
    sector64 pbn;
    int32_t i, ret, cn_packet, cn_page, cn_pack_page;

    cn_packet = 0;
    cn_page = eu->log_frontier >> SECTORS_PER_FLASH_PAGE_ORDER;
    cn_pack_page = cn_page - idx_start;

    ppack = NULL;
    pbn = 0;
    ret = -ENOMEM;
    cn_packet = 0;

    if ((cn_page == 0) || (cn_pack_page == 0)) {
        return 0;
    }

    if (cn_pack_page < 0) {
        LOGERR
            ("parameter error eu %llu id_hpeu %d disk_num %d cn_page %d idx_start %d\n",
             eu->start_pos, eu->id_hpeu, eu->disk_num, cn_page, idx_start);
        return -EPERM;
    }

    if ((ret = mdnode_set_state_copy(&eu->act_metadata, &pmlogger->wq)) < 0) {  // <0: act_metadata is close, no need to packet
        LOGINFO("act_metadata is close or is copying, ret %d\n", ret);
        return 0;
    }
//    LOGINFO("parameter eu %llu cn_page %d idx_start %d\n", eu->start_pos, cn_page, idx_start);

    for (i = idx_start; i < cn_page; i++) {
        if (NULL == ppack) {
            if (NULL ==
                (ppack =
                 metalog_packet_alloc_init(pmlogger, eu->disk_num, tp))) {
                goto l_end;
            }
        }

        pbn = eu->start_pos + (i << SECTORS_PER_FLASH_PAGE_ORDER);
        if (metalog_packet_setup
            (ppack, NULL, pbn, eu->act_metadata.mdnode->lpn[i]) == -EPERM) {
            metalog_mgrfg_list_insert(&pmlogger->par_mlmgr[eu->disk_num],
                                      ppack);
            ppack = NULL;
            cn_packet++;
            i--;
        }
    }

    if (NULL != (ppack)) {
        metalog_mgrfg_list_insert(&pmlogger->par_mlmgr[eu->disk_num], ppack);
        cn_packet++;
    }

    if (tp == ml_event) {
        atomic_add(cn_packet,
                   &pmlogger->par_mlmgr[eu->disk_num].cn_pkt_bf_swap);
    }

    ret = cn_pack_page;

  l_end:
    mdnode_set_state(&eu->act_metadata, md_exist);
    if ((eu->log_frontier == DATA_SECTOR_IN_SEU) && (eu->fg_metadata_ondisk)) {
        mdnode_free(&eu->act_metadata, pmlogger->td->pMdpool);
        LOGINFO("free mdnode eu %llu ret %d\n", eu->start_pos, ret);
    }

    wake_up_interruptible(&pmlogger->wq);
    return ret;
}

// dump all pending metadata of active eus of one disk to the metalog
static int32_t metalog_packet_active_eus(smetalogger_t * pmlogger,
                                         int32_t id_disk, struct HListGC **gc)
{
    int32_t i, ret;

    if (lfsm_readonly == 1) {
        return 0;
    }

    ret = 0;
    for (i = 0; i < TEMP_LAYER_NUM; i++) {
        if ((ret =
             metalog_packet_eu(pmlogger, gc[id_disk]->active_eus[i], 0,
                               ml_event) < 0)) {
            return ret;
        }
    }
    return 0;
}

static int32_t metalog_async_write(smetalog_pkt_t * packet,
                                   smetalogger_t * pmlogger, int32_t id_disk)
{
    struct bio *pBio;
    struct EUProperty *peu_cur;
    smetalog_eulog_t *eulog;
    int32_t size, cn_hpage, ret;

    if (atomic_read(&packet->cn_pending) == 0) {
        LOGERR("WRONG structure\n");
        return 0;
    }

    eulog = &pmlogger->par_eulog[id_disk];
    ret = 0;
    size = atomic_read(&packet->cn_pending) * sizeof(smetalog_info_t);
    cn_hpage = LFSM_CEIL_DIV(size, HARD_PAGE_SIZE - sizeof(smetalog_info_t));
    if (compose_bio(&pBio, NULL, metalog_end_bio, packet,
                    cn_hpage * HARD_PAGE_SIZE,
                    cn_hpage * MEM_PER_HARD_PAGE) < 0) {
        return -ENOMEM;
    }
    peu_cur = eulog->eu[eulog->idx];
    /// init bio
    pBio->bi_opf = REQ_OP_WRITE;
    pBio->bi_next = NULL;
    pBio->bi_iter.bi_idx = 0;
    pBio->bi_iter.bi_sector = peu_cur->start_pos + peu_cur->log_frontier;

    metalog_bv_bio_data(pBio, packet, cn_hpage, size, eulog->seq);

    if ((ret = my_make_request(pBio)) < 0) {
        packet->err = -EIO;
        LOGINFO("ERR return packet(%p)\n", packet);
        metalog_async_write_bh(packet);
        free_composed_bio(pBio, cn_hpage * MEM_PER_HARD_PAGE);
        return ret;
    }
//    printk("write metalog eu[%d] %llu ft %d\n",eulog->idx, peu_cur->start_pos,peu_cur->log_frontier>>SECTORS_PER_FLASH_PAGE_ORDER);

    peu_cur->log_frontier += cn_hpage * SECTORS_PER_HARD_PAGE;
    if (peu_cur->log_frontier == SECTORS_PER_SEU) {
        if (eulog->idx == MLOG_EU_NUM - 1) {
            LOGERR("Can't round to next eu because full and not recovery\n");
        }
        eulog->idx = (eulog->idx + 1) % MLOG_EU_NUM;
        metalog_packet_active_eus(pmlogger, id_disk, gc);
        eulog->seq++;
    }

    return 0;
}

static int32_t metalog_process_dequeue(void *vmlmgr)
{
    smetalog_mgr_fg_t *pmgr_fg;
    smetalog_pkt_t *packet_cur, *packet_next;
    smetalogger_t *pmlogger;
    unsigned long flag;
    int32_t id_disk;
    bool ret;

    ret = false;
    pmgr_fg = vmlmgr;
    current->flags |= PF_MEMALLOC;
    set_current_state(TASK_INTERRUPTIBLE);
    while (true) {
        wait_event_interruptible(pmgr_fg->wq_dequeue,
                                 kthread_should_stop() ||
                                 ((ret = (atomic_read(&pmgr_fg->cn_full) > 0))
                                  || (metalog_packet_has_metadata(pmgr_fg)
                                      && atomic_read(&pmgr_fg->state) == 1)));

        if (kthread_should_stop()) {
            break;
        }

        spin_lock_irqsave(&pmgr_fg->slock, flag);
        if (list_empty(&pmgr_fg->list_full)) {
            if (metalog_packet_has_metadata(pmgr_fg)) {
                packet_cur = pmgr_fg->ppacket_act;
                pmgr_fg->ppacket_act = NULL;
                atomic_set(&pmgr_fg->state, 0);
                goto l_submit;
            }
            spin_unlock_irqrestore(&pmgr_fg->slock, flag);
            continue;
        }

        list_for_each_entry_safe(packet_cur, packet_next, &pmgr_fg->list_full,
                                 hook) {
            list_del_init(&packet_cur->hook);
            atomic_dec(&pmgr_fg->cn_full);
            break;
        }

      l_submit:
        spin_unlock_irqrestore(&pmgr_fg->slock, flag);

        if (!IS_ERR_OR_NULL(packet_cur)) {
            pmlogger = packet_cur->pmlogger;
            id_disk = packet_cur->id_disk;
            metalog_dumpstat(pmgr_fg, atomic_read(&packet_cur->cn_pending),
                             id_disk);
            metalog_async_write(packet_cur, pmlogger, id_disk);
            metalog_eulog_change(&pmlogger->par_eulog[id_disk], pmgr_fg,
                                 gc[id_disk], pmlogger->td);
        }
    }

    return 0;
}

// cn_pkt_bf_swap can only be minor to zero once
int32_t metalog_async_write_event_bh(smetalog_pkt_t * ppack)
{
    smetalogger_t *pmlogger;
    pmlogger = ppack->pmlogger;

    if (ppack->tp == ml_event) {
        atomic_dec(&pmlogger->par_mlmgr[ppack->id_disk].cn_pkt_bf_swap);
    }
//    printk("packreturn eu(%p) event %d\n",ppack, atomic_read(&pmlogger->mlmgr[ppack->id_disk].cn_pkt_bf_swap));

    atomic_set(&pmlogger->par_mlmgr[ppack->id_disk].state, 1);
    wake_up_interruptible(&pmlogger->par_mlmgr[ppack->id_disk].wq_dequeue);
    metalog_packet_free(pmlogger, ppack);
    return 0;
}

static int32_t metalog_process_bh(void *vmlmgr_bh)
{
    smetalog_mgr_t *pmgr_bh;
    smetalog_pkt_t *packet_cur, *packet_next;
    unsigned long flag;

    current->flags |= PF_MEMALLOC;
    pmgr_bh = vmlmgr_bh;

    while (true) {
        wait_event_interruptible(pmgr_bh->wq,
                                 kthread_should_stop()
                                 || atomic_read(&pmgr_bh->cn_act) > 0);

        if (kthread_should_stop()) {
            break;
        }

        spin_lock_irqsave(&pmgr_bh->slock, flag);
        if (list_empty(&pmgr_bh->list_act)) {
            spin_unlock_irqrestore(&pmgr_bh->slock, flag);
            continue;
        }

        list_for_each_entry_safe(packet_cur, packet_next, &pmgr_bh->list_act,
                                 hook) {
            list_del_init(&packet_cur->hook);
            atomic_dec(&pmgr_bh->cn_act);
            break;
        }
        spin_unlock_irqrestore(&pmgr_bh->slock, flag);

        if (packet_cur->err < 0) {
            LOGINFO("packet(%p) err retry id_disk %d\n", packet_cur,
                    packet_cur->id_disk);
            packet_cur->err = 0;
            diskgrp_logerr(&grp_ssds, packet_cur->id_disk);
            metalog_mgrfg_list_insert(&packet_cur->pmlogger->
                                      par_mlmgr[packet_cur->id_disk],
                                      packet_cur);
            continue;
        }
        ///// packet_cur is good
        diskgrp_cleanlog(&grp_ssds, packet_cur->id_disk);
        if (packet_cur->tp == ml_user) {
            metalog_async_write_bh(packet_cur);
        } else {                // tp==ml_event || tp==ml_idle
            metalog_async_write_event_bh(packet_cur);
        }
        schedule();
    }

    return 0;
}

static void metalog_kill_thread_all(smetalogger_t * pmlogger)
{
    int32_t i;
    if (pmlogger->par_mlmgr) {
        for (i = 0; i < pmlogger->td->cn_ssds; i++) {
            metalog_destroy_thread(&pmlogger->par_mlmgr[i].pthread);
        }
        track_kfree(pmlogger->par_mlmgr,
                    sizeof(smetalog_mgr_fg_t) * pmlogger->td->cn_ssds,
                    M_METALOG);
    }
    for (i = 0; i < MLOG_BH_THREAD_NUM; i++) {
        metalog_destroy_thread(&pmlogger->mlmgr_bh[i].pthread);
    }

    return;
}

/*
 * Description: create metalog threads for log the metadata changes
 *              fg thread: 1 thread per disk
 *              bh thread: 2
 */
int32_t metalog_create_thread_all(smetalogger_t * pmlogger)
{
    int8_t name[32];
    int32_t i;

    pmlogger->par_mlmgr = (smetalog_mgr_fg_t *)
        track_kmalloc(sizeof(smetalog_mgr_fg_t) * pmlogger->td->cn_ssds,
                      GFP_KERNEL, M_METALOG);

    for (i = 0; i < pmlogger->td->cn_ssds; i++) {
        sprintf(name, "lfsm_ml%d", i);
        if (metalog_mgrfg_init(&pmlogger->par_mlmgr[i], metalog_process_dequeue,
                               &pmlogger->par_mlmgr[i], name) < 0) {
            goto l_fail;
        }
    }

    atomic_set(&pmlogger->cn_bh, 0);
    for (i = 0; i < MLOG_BH_THREAD_NUM; i++) {
        sprintf(name, "lfsm_mlbh%d", i);
        if (metalog_mgr_init(&pmlogger->mlmgr_bh[i], metalog_process_bh,
                             &pmlogger->mlmgr_bh[i], name) < 0) {
            goto l_fail;
        }
    }

    return 0;
  l_fail:
    metalog_kill_thread_all(pmlogger);
    return -ENOMEM;
}

int32_t metalog_create_thread_append(smetalogger_t * pmlogger,
                                     int32_t cn_ssd_orig)
{
    int8_t name[32];
    int32_t i;
    smetalog_mgr_fg_t *par_mlmgr_new, *par_mlmgr_orig;

    int32_t cn_full[MAX_SUPPORT_SSD], sz_submit[MAX_SUPPORT_SSD],
        cn_submit[MAX_SUPPORT_SSD];
    // these 3 array is used record for new structure. (only for monitor?)
    // Cause old structure has to destroy before creating the new one

    par_mlmgr_orig = pmlogger->par_mlmgr;
    for (i = 0; i < cn_ssd_orig; i++) {
        cn_full[i] = atomic_read(&par_mlmgr_orig[i].cn_full);
        sz_submit[i] = par_mlmgr_orig[i].sz_submit;
        cn_submit[i] = par_mlmgr_orig[i].cn_submit;
    }

    par_mlmgr_new = (smetalog_mgr_fg_t *)
        track_kmalloc(sizeof(smetalog_mgr_fg_t) * pmlogger->td->cn_ssds,
                      GFP_KERNEL, M_METALOG);

    if (IS_ERR_OR_NULL(par_mlmgr_new)) {
        return -ENOMEM;
    }

    for (i = 0; i < cn_ssd_orig; i++) {
        metalog_destroy_thread(&par_mlmgr_orig[i].pthread);
    }
    track_kfree(par_mlmgr_orig, sizeof(smetalog_mgr_fg_t) * cn_ssd_orig,
                M_METALOG);

//  memcpy(par_mlmgr_new, pmlogger->par_mlmgr,sizeof(smetalog_mgr_fg_t)*cn_ssd_orig);
    for (i = 0; i < pmlogger->td->cn_ssds; i++) {
        sprintf(name, "lfsm_ml%d", i);
        if (metalog_mgrfg_init(&par_mlmgr_new[i], metalog_process_dequeue,
                               &par_mlmgr_new[i], name) < 0) {
            LFSM_ASSERT(0);
        }
    }
    for (i = 0; i < cn_ssd_orig; i++) { // Ding: is it necessary?
        atomic_set(&par_mlmgr_new[i].cn_full, cn_full[i]);
        par_mlmgr_new[i].sz_submit = sz_submit[i];
        par_mlmgr_new[i].cn_submit = cn_submit[i];
    }

//  par_mlmgr_orig = pmlogger->par_mlmgr;
    pmlogger->par_mlmgr = par_mlmgr_new;

//  track_kfree(par_mlmgr_orig, sizeof(smetalog_mgr_fg_t)*cn_ssd_orig, M_METALOG);
    return 0;
}

int32_t metalog_init(smetalogger_t * pmlogger, struct HListGC **gc,
                     lfsm_dev_t * td)
{
    pmlogger->td = td;
    if (metalog_eulog_init(pmlogger) < 0) {
        return -ENOMEM;
    }

    pmlogger->pmlpacket_pool =
        kmem_cache_create("mlpacket_pool", sizeof(smetalog_pkt_t), 0,
                          SLAB_HWCACHE_ALIGN, NULL);
    atomic_set(&pmlogger->cn_host, 0);
    init_waitqueue_head(&pmlogger->wq);
    return 0;
}

void metalog_destroy(smetalogger_t * pmlogger)
{
    if (pmlogger->pmlpacket_pool) {
        kmem_cache_destroy(pmlogger->pmlpacket_pool);
    }
    /*if (pmlogger->pmlhost_pool) {
       kmem_cache_destroy(pmlogger->pmlhost_pool);
       } */
    metalog_kill_thread_all(pmlogger);
    metalog_eulog_destroy(pmlogger);
    return;
}

// must be run under slock
static int32_t metalog_mgrfg_deactive(smetalog_mgr_fg_t * pmgr_fg)
{
    list_add_tail(&pmgr_fg->ppacket_act->hook, &pmgr_fg->list_full);
    pmgr_fg->ppacket_act = NULL;
    atomic_inc(&pmgr_fg->cn_full);
    wake_up_interruptible(&pmgr_fg->wq_dequeue);
    return 0;
}

// metalog_append(...)
int32_t metalog_packet_register(smetalogger_t * pmlogger,
                                struct bio_container *pbioc, sector64 pbn,
                                sector64 lpn)
{
    smetalog_pkt_t **pppacket;
    smetalog_mgr_fg_t *pmng_fg;
    struct EUProperty *peu;
    unsigned long flag;
    int32_t idx, id_disk;

    if (lfsm_readonly == 1) {
        return 0;
    }

    if (NULL == (peu = eu_find(pbn))) {
        return -ENOENT;
    }

    id_disk = peu->disk_num;
    pmng_fg = &pmlogger->par_mlmgr[id_disk];

  l_register:
    spin_lock_irqsave(&pmng_fg->slock, flag);
    pppacket = &pmng_fg->ppacket_act;
    if (NULL == *pppacket) {
        *pppacket = ERR_PTR(-EACCES);   // reserver the right to alloc
        spin_unlock_irqrestore(&pmng_fg->slock, flag);
        if (NULL ==
            (*pppacket =
             metalog_packet_alloc_init(pmlogger, id_disk, ml_user))) {
            *pppacket = NULL;
            LOGERR("Fail to alloc\n");
            return -ENOMEM;
        }

        wake_up_interruptible(&pmng_fg->wq_enqueue);
        goto l_register;
    } else if (*pppacket == ERR_PTR(-EACCES)) {
        spin_unlock_irqrestore(&pmng_fg->slock, flag);
        wait_event_interruptible(pmng_fg->wq_enqueue,
                                 *pppacket != ERR_PTR(-EACCES));
//        LOGINFO("Has waitted on ppbin\n");
        goto l_register;
    } else {
        if ((idx = metalog_packet_setup(*pppacket, pbioc, pbn, lpn)) == -EPERM) {   // full
            metalog_mgrfg_deactive(pmng_fg);
            spin_unlock_irqrestore(&pmng_fg->slock, flag);
//            LOGINFO("Has deactived\n");
            goto l_register;
        } else {
            if (idx == 0) {
                wake_up_interruptible(&pmng_fg->wq_dequeue);
            }
            spin_unlock_irqrestore(&pmng_fg->slock, flag);  // all setup finish
        }
    }

    return 0;
}

bool metalog_packet_flushed(smetalogger_t * pmlogger, struct HListGC **gc)
{
    int32_t i, j;

    for (i = 0; i < lfsmdev.cn_ssds; i++) {
        for (j = 0; j < TEMP_LAYER_NUM; j++) {
            if (gc[i]->active_eus[j]->
                log_frontier >> SECTORS_PER_FLASH_PAGE_ORDER !=
                gc[i]->par_cn_log[j]) {
                return false;
            }
        }
    }
    return true;
}

int32_t metalog_packet_cur_eus(smetalogger_t * pmlogger, int32_t id_disk,
                               struct HListGC **gc)
{
    int32_t i, ret;

    if (lfsm_readonly == 1) {
        return 0;
    }

    ret = 0;
    for (i = 0; i < TEMP_LAYER_NUM; i++) {
        ret = metalog_packet_eu(pmlogger, gc[id_disk]->active_eus[i],
                                gc[id_disk]->par_cn_log[i], ml_idle);
        if (ret == -ENOMEM) {
            return ret;
        } else if (ret > 0) {
            gc[id_disk]->par_cn_log[i] += ret;
        }
    }
    return 0;
}

int32_t metalog_packet_cur_eus_all(smetalogger_t * pmlogger,
                                   struct HListGC **gc)
{
    int32_t i, ret;

    if (lfsm_readonly == 1) {
        return 0;
    }

    ret = 0;
    for (i = 0; i < pmlogger->td->cn_ssds; i++) {
        LFSM_RMUTEX_LOCK(&gc[i]->whole_lock);
        ret = metalog_packet_cur_eus(pmlogger, i, gc);
        LFSM_RMUTEX_UNLOCK(&gc[i]->whole_lock);
        if (ret < 0) {
            break;
        }
    }
    return ret;
}

static int32_t metalog_async_load(smetalog_eulog_t * peulog,
                                  devio_fn_t * ar_pcb, int32_t cn_round,
                                  int32_t cn_hpage, lfsm_dev_t * td)
{
    sector64 pbn;
    int32_t i, j, ret;

    ret = 0;
    for (i = 0; i < MLOG_EU_NUM; i++) {
        for (j = 0; j < cn_round; j++) {
            pbn =
                peulog->eu[i]->start_pos +
                ((j * cn_hpage) << SECTORS_PER_HARD_PAGE_ORDER);
            ret =
                devio_async_read_hpages(pbn, cn_hpage,
                                        &ar_pcb[i * cn_round + j]);
            if (ret < 0) {
                atomic_set(&ar_pcb[i * cn_round + j].result,
                           DEVIO_RESULT_FAILURE);
            }
//            LOGINFO("pbn %llu ret %d\n",pbn,ret);
        }
        LOGINFO("eu %llu cn_round %d ret %d\n", peulog->eu[i]->start_pos,
                cn_round, ret);
    }

    return ret;
}

int32_t metalog_parse_item(smetalog_info_t * pinfo, int32_t cn_item,
                           lfsm_dev_t * td)
{
    struct EUProperty *peu;
    int32_t i, idx;

    for (i = 1; i < cn_item; i++) {
        if (pinfo[i].x.pbn == TAG_UNWRITE_HPAGE) {  // info is empty
            break;
        }

        if (pinfo[i].x.pbn == 0) {  // info is empty
            break;
        }
        ///// pbn to eu
        if (NULL == (peu = pbno_return_eu_idx(pinfo[i].x.pbn, &idx))) {
            return -EPERM;
        }
        ///// lpn insert to active_metadata
        if (peu->fg_metadata_ondisk) {  // if ondisk metadata is write done, don't insert active_metadata
            continue;
        }

        if (pinfo[i].x.lpn == MLOG_TAG_EU_ERASE) {
            LOGINFO("mlog parse: eu %llu lpn %llx\n", pinfo[i].x.pbn,
                    pinfo[i].x.lpn);
            if (NULL != (peu->act_metadata.mdnode)) {
                mdnode_free(&peu->act_metadata, td->pMdpool);
            }
            continue;
        }

        if (mdnode_alloc(&peu->act_metadata, td->pMdpool) == -ENOMEM) {
            return -ENOMEM;
        }
        peu->act_metadata.mdnode->lpn[idx] = pinfo[i].x.lpn;
    }

    return 0;
}

// return latest seq, or <0 fail
int32_t metalog_parse(int8_t * buffer, int32_t cn_hpage, lfsm_dev_t * td)
{
    smetalog_info_t *pmlinfo;
    int32_t i, seq_cur, ret, cn_item;

    cn_item = HARD_PAGE_SIZE / sizeof(smetalog_info_t);
    ret = 0;

    for (i = 0; i < cn_hpage; i++) {
        pmlinfo = (smetalog_info_t *) buffer;
        seq_cur = pmlinfo[0].seq;
        if (seq_cur == 0) {
            break;              // it is empty
        }

        if ((ret = metalog_parse_item(pmlinfo, cn_item, td)) < 0) {
            return ret;
        }
        buffer += HARD_PAGE_SIZE;
    }
    return ret;
}

int32_t metalog_async_load_bh(devio_fn_t * ar_pcb, int32_t cn_round,
                              int32_t cn_hpage, lfsm_dev_t * td)
{
    int8_t *buffer;
    int32_t i, ret;

    ret = 0;
    buffer = track_kmalloc(HARD_PAGE_SIZE * cn_hpage, GFP_KERNEL, M_OTHER);

    for (i = 0; i < cn_round; i++) {
        memset(buffer, 0, HARD_PAGE_SIZE * cn_hpage);
        if (!devio_async_read_pages_bh(cn_hpage, buffer, &ar_pcb[i])) {
            continue;
        }

        if ((ret = metalog_parse(buffer, cn_hpage, td)) < 0) {
            continue;
        }
    }

    track_kfree(buffer, HARD_PAGE_SIZE * cn_hpage, M_OTHER);
    return ret;
}

int32_t metalog_grp_load_all(smetalogger_t * pmlogger, lfsm_dev_t * td,
                             struct rddcy_fail_disk *fail_disk)
{
    devio_fn_t **ar_pcb;        //[DISK_PER_HPEU];
    int32_t i, ret, ret_final, cn_hpage, cn_round, j, count, id_grp;

    ret = 0;
    ret_final = 0;
    cn_hpage = MAX_MEM_PAGE_NUM / MEM_PER_HARD_PAGE;
    cn_round = LFSM_CEIL_DIV(HPAGE_PER_SEU, cn_hpage);
    id_grp = fail_disk->pg_id;
    count = fail_disk->count;

    LOGINFO("start: id_grp %d fail count %d\n", id_grp, count);

    if (NULL == (ar_pcb = (devio_fn_t **)
                 track_kmalloc(td->hpeutab.cn_disk * sizeof(devio_fn_t *),
                               GFP_KERNEL | __GFP_ZERO, M_OTHER))) {
        return -ENOMEM;
    }

    for (i = 0; i < td->hpeutab.cn_disk; i++) {
        if (NULL == (ar_pcb[i] = (devio_fn_t *)
                     track_kmalloc(sizeof(devio_fn_t) * cn_round * MLOG_EU_NUM,
                                   GFP_KERNEL, M_OTHER))) {
            ret_final = -ENOMEM;
            goto l_end;
        }
    }

    for (i = 0; i < td->hpeutab.cn_disk; i++) {
        for (j = 0; j < count; j++) {
            if (i == fail_disk->idx[j])
                break;
        }
        if (j < count)
            continue;

        metalog_async_load(&pmlogger->par_eulog[HPEU_EU2HPEU_OFF(i, id_grp)],
                           ar_pcb[i], cn_round, cn_hpage, td);
    }

    for (i = 0; i < td->hpeutab.cn_disk; i++) {
        for (j = 0; j < count; j++) {
            if (i == fail_disk->idx[j])
                break;
        }
        if (j < count)
            continue;

        if ((ret =
             metalog_async_load_bh(ar_pcb[i], cn_round * MLOG_EU_NUM, cn_hpage,
                                   td)) < 0) {
            ret_final = ret;
        }
    }

  l_end:
    for (i = 0; i < td->hpeutab.cn_disk; i++) {
        if (ar_pcb[i]) {
            track_kfree(ar_pcb[i], sizeof(devio_fn_t) * cn_round * MLOG_EU_NUM,
                        M_OTHER);
        }
    }

    track_kfree(ar_pcb, td->hpeutab.cn_disk * sizeof(devio_fn_t *), M_OTHER);
    return ret_final;
}

int32_t metalog_load_all(smetalogger_t * pmlogger, lfsm_dev_t * td)
{
    int ret = 0, i, fail_ids;
    struct rddcy_fail_disk fail_disk;

    for (i = 0; i < td->cn_pgroup; i++) {
        fail_ids = rddcy_search_failslot_by_pg_id(&grp_ssds, i);
        rddcy_fail_ids_to_disk(td->hpeutab.cn_disk, fail_ids, &fail_disk);
        if (fail_disk.count == 0)
            fail_disk.pg_id = i;
        if ((ret = metalog_grp_load_all(pmlogger, td, &fail_disk)) < 0) {
            LOGERR("grp[%d] fail disk(%d) can't complete metalog_load\n", i,
                   fail_ids);
            return ret;
        }
    }

    return ret;
}

void metalog_eulog_get_all(smetalog_eulog_t * ar_eulog,
                           sector64 * ar_eu_mlogger, int32_t lfsm_cn_ssds)
{
    int32_t i, j;
    for (i = 0; i < lfsm_cn_ssds; i++) {
        for (j = 0; j < MLOG_EU_NUM; j++) {
            ARI(ar_eu_mlogger, i, j, MLOG_EU_NUM) =
                ar_eulog[i].eu[j]->start_pos;
        }
    }
}

int32_t metalog_eulog_set_all(smetalog_eulog_t * ar_eulog,
                              sector64 * ar_eu_mlogger, int32_t lfsm_cn_ssds,
                              lfsm_dev_t * td)
{
    struct EUProperty *eu;
    int32_t i, j, ret;

    ret = -EPERM;

    for (i = 0; i < lfsm_cn_ssds; i++) {
        for (j = 0; j < MLOG_EU_NUM; j++) {
            eu = eu_find(ARI(ar_eu_mlogger, i, j, MLOG_EU_NUM));
            if (NULL == eu) {
                goto l_end;
            }
            ar_eulog[i].eu[j] = eu;
            if (NULL ==
                (FreeList_delete_by_pos
                 (td, ar_eulog[i].eu[j]->start_pos, EU_MLOG))) {
                goto l_end;
            }
        }
    }
    ret = 0;

  l_end:
    return ret;
}

static int32_t metalog_eulog_rescueA(smetalogger_t * pmlogger, int32_t id_disk,
                                     bool isFail)
{
    struct EUProperty *peu;
    int32_t i;

    // 102/12/18  AN: Tifa say no need to reget a metalog EU
/*
    if (isFail)
    {
        metalog_eulog_initA(&pmlogger->par_eulog[id_disk], id_disk, true);
        return 0;
            }
*///// else not fail, only erase eu but not change eu
    for (i = 0; i < MLOG_EU_NUM; i++) {
        peu = pmlogger->par_eulog[id_disk].eu[i];
        EU_erase(peu);
        peu->log_frontier = 0;
        LOGINFO("do erase and clear the eulog %llu\n", peu->start_pos);
    }
    return 0;
}

void metalog_grp_eulog_rescue(smetalogger_t * pmlogger, int32_t id_grp,
                              int32_t fail_disk)
{
    int32_t i, id_disk;
    for (i = 0; i < pmlogger->td->hpeutab.cn_disk; i++) {
        id_disk = HPEU_EU2HPEU_OFF(i, id_grp);
        metalog_eulog_rescueA(pmlogger, id_disk,
                              (id_disk == fail_disk) ? true : false);
    }
    //dedicated_map_logging(pmlogger->td); //102/12/18 no change EU, no need ddmap log
    return;
}
