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
#include <linux/bio.h>
#include <linux/list.h>
#include <linux/kthread.h>
#include "../config/lfsm_setting.h"
#include "../config/lfsm_feature_setting.h"
#include "../config/lfsm_cfg.h"
#include "../common/common.h"
#include "../common/mem_manager.h"
#include "../common/rmutex.h"
#include "mqbiobox.h"
#include "biocbh.h"
#include "../nextprepare.h"
#include "../diskgrp.h"
#include "../system_metadata/Dedicated_map.h"
#include "../lfsm_core.h"
#include "io_common.h"
#include "io_write.h"
#include "../common/tools.h"
#ifdef THD_EXEC_STEP_MON
#include "../lfsm_thread_monitor.h"
#endif
#include "../perf_monitor/thd_perf_mon.h"
#include "../perf_monitor/ioreq_pendstage_mon.h"

uint32_t get_total_waiting_io(bioboxpool_t * pBBP)
{
    uint32_t total_iors;
    int32_t i;

    total_iors = 0;
    for (i = 0; i < pBBP->cnt_io_workqueues; i++) {
        total_iors += atomic_read(&(pBBP->bioReq_queue[i].cnt_used_biobox));
    }

    return total_iors;
}

uint64_t get_total_write_io(bioboxpool_t * pBBP)
{
    usrbio_queue_t *myqueue;
    uint64_t all_wIO;
    int32_t i;

    all_wIO = 0;
    for (i = 0; i < pBBP->cnt_io_workqueues; i++) {
        myqueue = &pBBP->bioReq_queue[i];
        all_wIO += myqueue->cnt_totalwrite;
    }

    return all_wIO;
}

static void _get_total_io_cnt(bioboxpool_t * pBBP,
                              uint64_t * all_wIO, uint64_t * all_rIO,
                              uint64_t * all_pwIO, uint64_t * all_prIO)
{
    usrbio_queue_t *myqueue;
    int32_t i;

    *all_wIO = 0;
    *all_rIO = 0;
    *all_pwIO = 0;
    *all_prIO = 0;

    for (i = 0; i < pBBP->cnt_io_workqueues; i++) {
        myqueue = &pBBP->bioReq_queue[i];
        *all_wIO += myqueue->cnt_totalwrite;
        *all_rIO += myqueue->cnt_totalread;
        *all_pwIO += myqueue->cnt_total_partial_write;
        *all_prIO += myqueue->cnt_total_partial_read;
    }
}

int32_t bioboxpool_dump(bioboxpool_t * pBBP, int8_t * buffer)
{
    static int64_t cn_prev_r = 0, cn_prev_w = 0;
    uint64_t all_wIO, all_rIO, all_pwIO, all_prIO;
    int32_t len, id_sel;

    len = 0;
    id_sel = 0;

    _get_total_io_cnt(pBBP, &all_wIO, &all_rIO, &all_pwIO, &all_prIO);
    len += sprintf(buffer + len,
                   "total write bio: %llu (%llu MB) %llu mpage/perquery P:%llu\n",
                   all_wIO, all_wIO >> (20 - PAGE_ORDER), all_wIO - cn_prev_w,
                   all_pwIO);
    cn_prev_w = all_wIO;

    len += sprintf(buffer + len,
                   "total read bio: %llu (%llu MB) %llu mpage/perquery P:%llu\n",
                   all_rIO, all_rIO >> (20 - PAGE_ORDER), all_rIO - cn_prev_r,
                   all_prIO);
    cn_prev_r = all_rIO;

    len += show_IOR_pStage_mon(buffer + len);
    for (id_sel = 0; id_sel < pBBP->cnt_io_workqueues; id_sel++) {
        len += sprintf(buffer + len, "iot %d : %d\n",
                       id_sel,
                       atomic_read(&pBBP->bioReq_queue[id_sel].
                                   cnt_used_biobox));
    }

    len += sprintf(buffer + len,
                   "total copied mpage: %d total front erase: %d total back erase: %d\n",
                   atomic_read(&pBBP->cn_total_copied), cn_erase_front,
                   cn_erase_back);

    return len;
}

static void _init_bioReq_queue(usrbio_queue_t * myqueue)
{
    INIT_LIST_HEAD(&myqueue->free_biobox_pool);
    INIT_LIST_HEAD(&myqueue->used_biobox_pool);
    init_waitqueue_head(&myqueue->biobox_pool_waitq);
    atomic_set(&myqueue->cnt_used_biobox, 0);
    spin_lock_init(&myqueue->biobox_pool_lock);
    myqueue->cnt_totalwrite = 0;
    myqueue->cnt_total_partial_write = 0;
    myqueue->cnt_totalread = 0;
    myqueue->cnt_total_partial_read = 0;
}

static void _stop_io_workers(bioboxpool_t * pBBP)
{
    usrbio_handler_t *myhandler;
    int32_t i;

    for (i = 0; i < pBBP->cnt_io_workqueues; i++) {
        myhandler = &pBBP->io_worker[i];
        if (IS_ERR_OR_NULL(myhandler->pIOthread)) {
            continue;
        }

        if (kthread_stop(myhandler->pIOthread) < 0) {
            LOGERR("can't stop io_worker[%d].pIOthread\n", i);
        }

        myhandler->pIOthread = NULL;
    }
}

static int32_t _create_io_workers(bioboxpool_t * pBBP,
                                  int (*threadfunc)(void *arg), void *arg)
{
    int8_t nm_iothread[] = "lfsm_iod0000";
    usrbio_handler_t *myhandler;
    int32_t i, ret;

    ret = 0;
    for (i = 0; i < pBBP->cnt_io_workers; i++) {
        sprintf(nm_iothread, "lfsm_iod%03d", i);
        myhandler = &pBBP->io_worker[i];

        myhandler->handler_index = i;
        myhandler->usr_private_data = arg;

        if (pBBP->ioT_vcore_map[i] == -1) {
            myhandler->pIOthread =
                kthread_run(threadfunc, myhandler, nm_iothread);
        } else {
            myhandler->pIOthread =
                my_kthread_run(threadfunc, myhandler, nm_iothread,
                               pBBP->ioT_vcore_map[i]);
        }

        if (IS_ERR_OR_NULL(myhandler->pIOthread)) {
            _stop_io_workers(pBBP);
            return -ESRCH;
        }

        reg_thd_perf_monitor(myhandler->pIOthread->pid, LFSM_IO_THD,
                             nm_iothread);
#ifdef THD_EXEC_STEP_MON
        reg_thread_monitor(myhandler->pIOthread->pid, nm_iothread);
#endif
    }

    return ret;
}

/*
 * Description: initial io worker and io request queue.
 * NOTE: caller guarantee threadfunc not NULL and vcore_map not NULL
 */
int32_t bioboxpool_init(bioboxpool_t * pBBP, int32_t cnt_workers,
                        int32_t cnt_queues, int (*threadfunc)(void *arg),
                        void *arg, int32_t * vcore_map)
{
    int32_t i, j;
    biobox_t *pBioBox;

    pBBP->cnt_io_workers = cnt_workers;
    pBBP->cnt_io_workqueues = 32;   //cnt_queues;
    pBBP->ioT_vcore_map = vcore_map;
    atomic_set(&pBBP->idx_sel_used, 0);
    atomic_set(&pBBP->idx_sel_free, 0);
    atomic_set(&pBBP->cn_total_copied, 0);

#if LFSM_BIOBOX_STAT
    atomic_set(&pBBP->cn_empty, 0);
    atomic_set(&pBBP->cn_runout, 0);
    atomic64_set(&pBBP->cn_totalcmd, 0);
    atomic_set(&pBBP->stat_queuetime, 0);
    atomic_set(&pBBP->stat_rounds, 0);
#endif

    for (i = 0; i < pBBP->cnt_io_workqueues; i++) {
        _init_bioReq_queue(&pBBP->bioReq_queue[i]);
    }

    j = 0;
    for (i = 0; i < LFSM_NUM_BIOBOX; i++) {
        pBioBox = track_kmalloc(sizeof(biobox_t), GFP_KERNEL, M_BIOBOX_POOL);
        if (IS_ERR_OR_NULL(pBioBox)) {
            return -ENOMEM;
        }

        list_add_tail(&pBioBox->list_2_bioboxp,
                      &pBBP->bioReq_queue[j].free_biobox_pool);
        j = (j + 1) % pBBP->cnt_io_workqueues;
    }

    for (i = 0; i < SZ_BIOBOX_HASH; i++) {
        pBBP->bio_hash[i] = NULL;
    }
    spin_lock_init(&pBBP->bio_hash_lock);

    return _create_io_workers(pBBP, threadfunc, arg);
}

void bioboxpool_destroy(bioboxpool_t * pBBP)
{
    usrbio_queue_t *myqueue;
    biobox_t *pBioBox, *pTmpBox;
    int32_t i;

    if (IS_ERR_OR_NULL(pBBP)) {
        return;
    }

    _stop_io_workers(pBBP);

    for (i = 0; i < pBBP->cnt_io_workqueues; i++) {
        myqueue = &pBBP->bioReq_queue[i];

        if (list_empty_check(&myqueue->free_biobox_pool)) {
            continue;
        }

        list_for_each_entry_safe(pBioBox, pTmpBox, &myqueue->free_biobox_pool,
                                 list_2_bioboxp) {
            list_del(&pBioBox->list_2_bioboxp);
            track_kfree(pBioBox, sizeof(biobox_t), M_BIOBOX_POOL);
        }
    }

    for (i = 0; i < pBBP->cnt_io_workqueues; i++) {
        myqueue = &pBBP->bioReq_queue[i];

        if (list_empty_check(&myqueue->used_biobox_pool)) {
            continue;
        }

        LOGWARN("bioboxpool_free: %d list_used_biobox is not empty\n", i);
        list_for_each_entry_safe(pBioBox, pTmpBox, &myqueue->used_biobox_pool,
                                 list_2_bioboxp) {
            bioc_resp_user_bio(pBioBox->usr_bio, -EIO);
            list_del(&pBioBox->list_2_bioboxp);
            track_kfree(pBioBox, sizeof(biobox_t), M_BIOBOX_POOL);
        }
    }

    for (i = 0; i < SZ_BIOBOX_HASH; i++) {
        if (pBBP->bio_hash[i]) {
            LOGWARN(" bio_hash[%d]!=0 (%p)\n", i, pBBP->bio_hash[i]);
        }
    }
}

static void bioboxpool_hash_switch(bioboxpool_t * pBBP, struct bio *pBio,
                                   bool isSet)
{
#if MEM_PER_FLASH_PAGE == 1
    return;
#else
    unsigned long flag;
    uint32_t key;

    if ((pBio->bi_iter.bi_size == 0) || (pBio->bi_iter.bi_size > PAGE_SIZE) ||
        ((pBio->bi_iter.bi_sector % SECTORS_PER_FLASH_PAGE) != 0)) {
        return;
    }

    key =
        ((uint32_t) (pBio->bi_iter.bi_sector >> SECTORS_PER_FLASH_PAGE_ORDER) *
         BIOBOX_BIGPRIMARY) % SZ_BIOBOX_HASH;
    spin_lock_irqsave(&pBBP->bio_hash_lock, flag);
    if (isSet) {
//        printk("%s: set hash[%d] = %ld size=%u\n",__FUNCTION__,key,pBio->bi_iter.bi_sector,pBio->bi_iter.bi_size);
        if (NULL == (pBBP->bio_hash[key])) {
            pBBP->bio_hash[key] = pBio;
        }
    } else {
        // you may found some bio has size > PAGE_SIZE, although these bio should be filter at the initial of function
        // but it is normal because the filter check size before lock bio_hash_lock
//        printk("%s: unset hash[%d] from %ld size=%u\n",__FUNCTION__,key,pBio->bi_iter.bi_sector,pBio->bi_iter.bi_size);
        if (pBBP->bio_hash[key] == pBio) {
            pBBP->bio_hash[key] = NULL;
        }
    }
    spin_unlock_irqrestore(&pBBP->bio_hash_lock, flag);
    return;
#endif
}

#if MEM_PER_FLASH_PAGE > 2
#error "bioboxpool_append only support MEM_PER_FLASH_PAGE = 1 or 2"
#endif

#if MEM_PER_FLASH_PAGE == 2
bool bioboxpool_hash_append(bioboxpool_t * pBBP, struct bio *pBio)
{
    struct bio_vec tmpvec;
    struct bio *pHashedBio;
    unsigned long flag;
    uint32_t key;
    bool ret;

    ret = false;
    if ((pBio->bi_iter.bi_size == 0) || (pBio->bi_iter.bi_size > PAGE_SIZE)
        || ((pBio->bi_iter.bi_sector % SECTORS_PER_FLASH_PAGE) !=
            SECTORS_PER_PAGE)) {
        return false;
    }
    key =
        ((uint32_t) (pBio->bi_iter.bi_sector >> SECTORS_PER_FLASH_PAGE_ORDER) *
         BIOBOX_BIGPRIMARY) % SZ_BIOBOX_HASH;
    spin_lock_irqsave(&pBBP->bio_hash_lock, flag);
    if ((pHashedBio = pBBP->bio_hash[key]) == NULL) {
        goto l_nomerge;
    }

    if ((pHashedBio->bi_iter.bi_sector >> SECTORS_PER_FLASH_PAGE_ORDER) !=
        (pBio->bi_iter.bi_sector >> SECTORS_PER_FLASH_PAGE_ORDER)) {
        goto l_nomerge;
    }

    if (pHashedBio->bi_vcnt >= MEM_PER_FLASH_PAGE) {
        goto l_nomerge;
    }

    if (((pHashedBio->bi_iter.bi_sector +
          (pHashedBio->bi_iter.bi_size >> SECTOR_ORDER)) ==
         pBio->bi_iter.bi_sector)
        && (bio_data_dir(pHashedBio) = = bio_data_dir(pBio))) {
        pHashedBio->bi_next = pBio;
        tmpvec = pHashedBio->bi_io_vec[pHashedBio->bi_vcnt];
        pHashedBio->bi_io_vec[pHashedBio->bi_vcnt] = pBio->bi_io_vec[0];
        pBio->bi_io_vec[0] = tmpvec;
        pHashedBio->bi_vcnt++;
        pHashedBio->bi_iter.bi_size += pBio->bi_iter.bi_size;
//        printk("%s: append hash[%d] = %ld bi_size=%u\n",__FUNCTION__,key,pHashedBio->bi_iter.bi_sector,pHashedBio->bi_iter.bi_size);
        pBBP->bio_hash[key] = NULL;
        ret = true;
    }

  l_nomerge:
    spin_unlock_irqrestore(&pBBP->bio_hash_lock, flag);
    return ret;
}

//FIXME: LEGO: no biohandler any more, here should be queue
bool bioboxpool_append_last(bioboxpool_t * pBBP, struct bio *pBio)
{
    biobox_t *pBioBox = 0, *pTmpBox;
    struct bio *pRetBio, *pEndBio;
    unsigned long flag;
    struct bio_vec tmpvec;
    int id_prev_sel =
        atomic_read(&pBBP->idx_sel_used) % pBBP->cnt_io_workqueues;
    //||(bio_data_dir(pBio)!=WRITE)
//        ||(bio_data_dir(pBio)!=WRITE)

    if ((pBio->bi_iter.bi_size == 0) || (pBio->bi_iter.bi_size > PAGE_SIZE) ||
        ((pBio->bi_iter.bi_sector % SECTORS_PER_FLASH_PAGE) !=
         SECTORS_PER_PAGE)) {
        return false;
    }

    spin_lock_irqsave(&pBBP->io_worker[id_prev_sel].lock, flag);

    if (list_empty(&pBBP->io_worker[id_prev_sel].list_used)) {
        goto l_nomerge;
    }

    list_for_each_entry_safe_reverse(pBioBox, pTmpBox,
                                     &pBBP->io_worker[id_prev_sel].list_used,
                                     list) {
        pRetBio = pBioBox->pbio;
        if (pRetBio->bi_vcnt >= MEM_PER_FLASH_PAGE)
            goto l_nomerge;
        pEndBio = pRetBio;
        while (pEndBio->bi_next)
            pEndBio = pEndBio->bi_next;
        if (((pEndBio->bi_iter.bi_sector +
              (pEndBio->bi_iter.bi_size >> SECTOR_ORDER)) ==
             pBio->bi_iter.bi_sector)
            && (bio_data_dir(pEndBio) == bio_data_dir(pBio))) {
            pEndBio->bi_next = pBio;
            tmpvec = pRetBio->bi_io_vec[pRetBio->bi_vcnt];
            pRetBio->bi_io_vec[pRetBio->bi_vcnt] = pBio->bi_io_vec[0];
            pBio->bi_io_vec[0] = tmpvec;
            pRetBio->bi_vcnt++;
            pRetBio->bi_iter.bi_size += pBio->bi_iter.bi_size;
            wake_up_interruptible(&pBBP->io_worker[id_prev_sel].wq);
            spin_unlock_irqrestore(&pBBP->io_worker[id_prev_sel].lock, flag);
            return true;
        } else {
            bbp_printk("Fail to merge because\n");
            bbp_printk("%s orig bio=%p,%lld,%d,%d %p\n",
                       __FUNCTION__, pRetBio, pRetBio->bi_iter.bi_sector,
                       pRetBio->bi_iter.bi_size, pRetBio->bi_vcnt,
                       pRetBio->bi_io_vec);
            bbp_printk("%s new bio=%p,%lld,%d,%d %p\n", __FUNCTION__, pBio,
                       pBio->bi_iter.bi_sector, pBio->bi_iter.bi_size,
                       pBio->bi_vcnt, pBio->bi_io_vec);
        }
        goto l_nomerge;
    }
  l_nomerge:
    spin_unlock_irqrestore(&pBBP->io_worker[id_prev_sel].lock, flag);
    return false;

}
#endif

int32_t bioboxpool_free2used(bioboxpool_t * pBBP, struct bio *pBio,
                             int32_t id_sel)
{
    biobox_t *myBioBox;
    usrbio_queue_t *myqueue;
    int64_t size_bi_vnt, bio_bi_sect, size_bi;
    int32_t rw_dir, ret;
    bool is_nonalign_io;

    if (id_sel < 0) {
        id_sel =
            ((uint32_t) (atomic_inc_return(&pBBP->idx_sel_used))) %
            pBBP->cnt_io_workqueues;
    }

    ret = 0;
    rw_dir = bio_data_dir(pBio);
    size_bi_vnt = pBio->bi_vcnt;
    size_bi = pBio->bi_iter.bi_size;
    bio_bi_sect = pBio->bi_iter.bi_sector;

    if ((size_bi % FLASH_PAGE_SIZE) || (bio_bi_sect % SECTORS_PER_FLASH_PAGE)) {
        is_nonalign_io = true;
    } else {
        is_nonalign_io = false;
    }

    myqueue = &pBBP->bioReq_queue[id_sel];
    spin_lock(&myqueue->biobox_pool_lock);
    while (list_empty(&myqueue->free_biobox_pool)) {
        spin_unlock(&myqueue->biobox_pool_lock);
#if LFSM_BIOBOX_STAT
        atomic_inc(&pBBP->cn_runout);
#endif
        //TODO: schedule maybe not a good way
        schedule();
        spin_lock(&myqueue->biobox_pool_lock);
    }

    //NOTE: list_first_entry expect free_biobox_pool isn't empty
    myBioBox =
        list_first_entry(&myqueue->free_biobox_pool, biobox_t, list_2_bioboxp);
    myBioBox->usr_bio = pBio;
#if LFSM_BIOBOX_STAT
    pBioBox->tag_time = get_time_on_us();
#endif

    list_move_tail(&myBioBox->list_2_bioboxp, &myqueue->used_biobox_pool);
    bbp_printk("bioboxpool: %c enq bio(%p) %lld %u %d\n",
               pBio->bi_opf ? 'W' : 'R', pBio,
               (sector64) pBio->bi_iter.bi_sector, pBio->bi_iter.bi_size,
               pBio->bi_vcnt);
    INC_IOR_STAGE_CNT(IOR_STAGE_WAIT_IOT, id_sel);
    INC_IOR_STAGE_CNT(IOR_STAGE_LFSM_RCV, id_sel);
    bioboxpool_hash_switch(pBBP, pBio, true);

    if (rw_dir) {
        myqueue->cnt_totalwrite += size_bi_vnt;
        if (is_nonalign_io) {
            myqueue->cnt_total_partial_write++;
        }
    } else {
        myqueue->cnt_totalread += size_bi_vnt;
        if (is_nonalign_io) {
            myqueue->cnt_total_partial_read++;
        }
    }
    spin_unlock(&myqueue->biobox_pool_lock);

    atomic_inc(&myqueue->cnt_used_biobox);
    if (waitqueue_active(&myqueue->biobox_pool_waitq)) {
        wake_up_interruptible(&myqueue->biobox_pool_waitq);
    }
#if LFSM_BIOBOX_STAT
    atomic64_inc(&pBBP->cn_totalcmd);
#endif

    return ret;
}

struct bio *bioboxpool_used2free(bioboxpool_t * pBBP)
{
    biobox_t *myBioBox;
    usrbio_queue_t *myqueue;
    struct bio *pRetBio;
    uint32_t id_sel;
    int32_t ret;

    id_sel =
        (uint32_t) (atomic_inc_return(&pBBP->idx_sel_free)) %
        pBBP->cnt_io_workqueues;

    myqueue = &pBBP->bioReq_queue[id_sel];

    spin_lock(&myqueue->biobox_pool_lock);
    while (list_empty(&myqueue->used_biobox_pool)) {
        spin_unlock(&myqueue->biobox_pool_lock);

#if LFSM_BIOBOX_STAT
        atomic_inc(&pBBP->cn_empty);
#endif
        ret = wait_event_interruptible(myqueue->biobox_pool_waitq,
                                       ((atomic_read(&myqueue->cnt_used_biobox)
                                         > 0) || kthread_should_stop()));

        if (kthread_should_stop()) {
            return NULL;
        }

        spin_lock(&myqueue->biobox_pool_lock);
    }

    //NOTE: list_first_entry expect free_biobox_pool isn't empty
    myBioBox =
        list_first_entry(&myqueue->used_biobox_pool, biobox_t, list_2_bioboxp);
    pRetBio = myBioBox->usr_bio;
    myBioBox->usr_bio = NULL;

#if LFSM_BIOBOX_STAT
    atomic_add((uint64_t) (get_time_on_us() - myBioBox->tag_time),
               &pBBP->stat_queuetime);
    atomic_inc(&pBBP->stat_rounds);
#endif

    list_move_tail(&myBioBox->list_2_bioboxp, &myqueue->free_biobox_pool);
    bioboxpool_hash_switch(pBBP, pRetBio, false);
    DEC_IOR_STAGE_CNT(IOR_STAGE_WAIT_IOT, id_sel);
    spin_unlock(&myqueue->biobox_pool_lock);

    atomic_dec(&myqueue->cnt_used_biobox);
    //TODO: Lego 20161128 weird code here, who need to be woke up??
    //      normal design should be user thread waiting free biobox and io thread wake up them
    if (waitqueue_active(&myqueue->biobox_pool_waitq)) {
        wake_up_interruptible(&myqueue->biobox_pool_waitq);
    }

    return pRetBio;
}
