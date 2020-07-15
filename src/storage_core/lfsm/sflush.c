/*
 * SPDX-License-Identifier: GPL-2.0-or-later OR Apache-2.0
 * Copyright (C) 2015-2020 Industrial Technology Research Institute
 *
 *
 *
 *  
 */
///////////////////////////////////////////////////////////////////////////////
//    Overview:
//        You can call sflush_init() to run two kthreads to help SOFA copying data.
//        sflush_read_process() takes charges of the callback of a page read
//        while sflush_write_process() is for the callback of a data write.
//        One may submit a read bio to kernel and ask its end_bio to use
//        sflush_readphase_done() to hook the bioc
//        of the bio to the read kthread handled by sflush_read_process().
//        Then, the read kthread will check error and then re-submit a write bio to kernel
//        to finish the copy operation.
//
//        For now, sflush_init are called twice in lfsm.c
//            sflush_init(&lfsmdev,&lfsmdev.gcwrite,ID_USER_GCWRITE)
//            sflush_init(&lfsmdev,&lfsmdev.crosscopy,ID_USER_CROSSCOPY)
//        The sflush named lfsmdev.gcwrite and its 2 kthreads help GC to copy data pages on parallel.
//            SOFA uses HPEUgc_submit_copyios() to submit read bio and their end-bio will hook bioc back to gcwrite's queue
//
//        The sflush named lfsmdev.crosscopy helps SOFA for the following 2 cases:
//            - bmt garbage collection. HINT: copy_valid_pagebmt()
//            - system eu clone during SOFA rebuild period (i.e. one drive crashes). HINT: EU_copyall()
//        SOFA uses sflush_copy_submit() to submit read bio and  and their end-bio will hook bioc back to crosscopy's queue
//
//    Key Structure Name: ssflush_t
//    Key Function Name:
//        sflush_read_process() : the read process
//        sflush_write_process() : the write process
//        sflush_readphase_done() : enqueue the read process
///////////////////////////////////////////////////////////////////////////////

#include <linux/bio.h>
#include "config/lfsm_setting.h"
#include "config/lfsm_feature_setting.h"
#include "config/lfsm_cfg.h"
#include "common/common.h"
#include "common/mem_manager.h"
#include "common/rmutex.h"
#include "io_manager/mqbiobox.h"
#include "io_manager/biocbh.h"
#include "io_manager/biocbh.h"
#include "nextprepare.h"
#include "diskgrp.h"
#include "system_metadata/Dedicated_map.h"
#include "system_metadata/Dedicated_map.h"
#include "lfsm_core.h"
#include "autoflush.h"
#include "bmt.h"
#include "io_manager/io_common.h"
#include "GC.h"
#include "special_cmd.h"
#include "bmt_ppq.h"
#include "bmt_ppdm.h"
#include "io_manager/io_write.h"
#include "io_manager/io_read.h"
#include "autoflush.h"

#include "EU_access.h"
#include "sflush_private.h"
#ifdef  HLIST_GC_WHOLE_LOCK_DEBUG
#include "lfsm_thread_monitor.h"
#endif

static int32_t sflush_exec(lfsm_dev_t * td, sflushops_t * ops, sector64 s_lpno,
                           int32_t len)
{
    return 0;
}

static void sflush_end_bio_crosscopy_read_brm(struct bio *bio)
{
    struct bio_container *entry;
    lfsm_dev_t *td;

    entry = bio->bi_private;
    td = entry->lfsm_dev;
    if (bio->bi_status == BLK_STS_OK) {
        entry->io_queue_fin = 1;
    } else {
        entry->io_queue_fin = blk_status_to_errno(bio->bi_status);
    }
    sflush_readphase_done((ssflush_t *) (td->crosscopy.variables), entry);
    return;
}

static void sflush_end_bio_crosscopy_read(struct bio *bio)
{
    struct bio_container *entry;
    lfsm_dev_t *td;

    entry = bio->bi_private;
    td = entry->lfsm_dev;
    if (bio->bi_status == BLK_STS_OK) {
        entry->io_queue_fin = 1;
    } else {
        entry->io_queue_fin = blk_status_to_errno(bio->bi_status);
    }

    atomic_set(&bio->__bi_cnt, 1);
    track_bio_put(bio);         // relesae bio in read
    sflush_readphase_done((ssflush_t *) (td->crosscopy.variables), entry);
    return;
}

int32_t sflush_copy_submit(lfsm_dev_t * td, sector64 source, sector64 dest,
                           sector64 lpn, int32_t cn_page)
{
    bio_end_io_t *ar_bi_end_io[MAX_ID_ENDBIO];
    struct bio_container *bio_c;
    ssflush_t *ssf;
    int32_t i;

    ssf = (ssflush_t *) td->crosscopy.variables;

    for (i = 0; i < cn_page; i++) {
        bio_c = bioc_get(td, NULL, 0, 0, 0, TP_COPY_IO, WRITE, 1);
        LFSM_ASSERT(bio_c);
        bio_c->source_pbn = source;
        bio_c->dest_pbn = dest;
        bio_c->start_lbno = lpn;
        ar_bi_end_io[ID_ENDBIO_BRM] = sflush_end_bio_crosscopy_read_brm;
        ar_bi_end_io[ID_ENDBIO_NORMAL] = sflush_end_bio_crosscopy_read;
        read_flash_page_vec(td, source, bio_c->bio->bi_io_vec, 0, bio_c, true,
                            ar_bi_end_io);
        atomic_inc(&ssf->non_ret_bio_cnt);
        //printk("TF: %s source %llu dest %llu lpn %llu cn_bio %d\n",__FUNCTION__, source, dest, lpn, atomic_read(&ssf->non_ret_bio_cnt));
        source += SECTORS_PER_FLASH_PAGE;
        dest += SECTORS_PER_FLASH_PAGE;
    }
    return 0;
}

void sflush_open(sflushops_t * ops)
{
    ssflush_t *ssf;

    ssf = (ssflush_t *) ops->variables;
    atomic_set(&ssf->non_ret_bio_cnt, 0);
}

bool sflush_waitdone(sflushops_t * ops)
{
    ssflush_t *ssf;

    ssf = ops->variables;
    lfsm_wait_event_interruptible(ssf->wq_submitor,
                                  atomic_read(&ssf->non_ret_bio_cnt) == 0);
    return true;
}

// from call back

void sflush_readphase_done(ssflush_t * ssf, struct bio_container *pBioc)
{
    unsigned long flag;
//    bool empty=false;
    spin_lock_irqsave(&ssf->finish_r_list_lock, flag);
//    empty=list_empty(&pbiocbh->io_done_list);
//    printk("biocbh:  read io_done %llu\n",pBioc->start_lbno);
    list_add_tail(&pBioc->submitted_list, &ssf->finish_r_list);
    atomic_inc(&ssf->len_finish_read);

//    if (empty)
    wake_up_interruptible(&ssf->wq_proc_finish_r);
    spin_unlock_irqrestore(&ssf->finish_r_list_lock, flag);
}

void sflush_writephase_done(ssflush_t * ssf, struct bio_container *pBioc)
{
    unsigned long flag;
//    bool empty=false;
    spin_lock_irqsave(&ssf->finish_w_list_lock, flag);
//    empty=list_empty(&pbiocbh->io_done_list);
//    printk("biocbh: write io_done %llu\n",pBioc->start_lbno);
    list_add_tail(&pBioc->submitted_list, &ssf->finish_w_list);
    atomic_inc(&ssf->len_finish_write);

//    if (empty)
    wake_up_interruptible(&ssf->wq_proc_finish_w);
    spin_unlock_irqrestore(&ssf->finish_w_list_lock, flag);

}

static void end_bio_write_sflush_hybrid(struct bio *bio)
{
    LFSM_ASSERT2(0, "%s: should not be called when AN_HYBRID=0\n",
                 __FUNCTION__);
}

static void end_bio_write_sflush_crosscopy(struct bio *bio)
{
    struct bio_container *entry;
    lfsm_dev_t *td;

    entry = bio->bi_private;
    td = entry->lfsm_dev;
    entry->status = bio->bi_status;

    if (bio->bi_status) {
        LOGERR("Error %d in write request\n", bio->bi_status);
    }
//    LFSM_ASSERT2(err==0,
//        "(2)enter_page_write_call_back error, bio= %p, bioc = %p, active_flag %d,err=%d\n",bio,entry,atomic_read(&entry->active_flag),err);
    sflush_writephase_done((ssflush_t *) (td->crosscopy.variables), entry);
    return;
}

static int32_t sflush_copy_submitwrite(struct bio_container *pBioc)
{
    int32_t ret;

    ret = 0;
    init_bio_container(pBioc, NULL, NULL, end_bio_write_sflush_crosscopy,
                       WRITE, pBioc->dest_pbn, MEM_PER_FLASH_PAGE);

    pBioc->io_queue_fin = 0;
    if ((ret = my_make_request(pBioc->bio)) < 0) {
        return ret;
    }

    return 0;
}

int32_t sflush_gcwrite_handler(ssflush_t * ssf, struct bio_container *pBioc,
                               bool isRetry)
{
    int32_t ret;

    //printk("%s bioc(%p) type %d source %llu lpn %llu\n",__FUNCTION__,pBioc, pBioc->write_type,pBioc->source_pbn, pBioc->start_lbno);
    if (pBioc->write_type == TP_GC_IO) {
        init_bio_container(pBioc, NULL, NULL, end_bio_write, REQ_OP_WRITE, -1,
                           MEM_PER_FLASH_PAGE);
        pBioc->dest_pbn = -1;
        ret = bioc_submit_write(&lfsmdev, pBioc, isRetry);
        if (ret < 0) {          // non-pipeline's error path (before submit or submit failure)
            LOGINFO("gcwrite dec pin pbioc(%p) source %llu\n", pBioc,
                    pBioc->source_pbn);
            HPEUgc_dec_pin_and_wakeup(pBioc->source_pbn);
        }
    } else {
        LOGERR("unknown write type: %d\n", pBioc->write_type);
    }

    return 0;
}

static int32_t sflush_copywrite_handler(ssflush_t * ssf,
                                        struct bio_container *pBioc)
{
    lfsm_dev_t *td;
    //bioc_bio_free(td,pBioc);

    td = (lfsm_dev_t *) ssf->ptd;
    put_bio_container(td, pBioc);
    atomic_dec(&(ssf->non_ret_bio_cnt));
    wake_up_interruptible(&(ssf->wq_submitor));

    return 0;
}

static int32_t sflush_copyread_handler(ssflush_t * ssf,
                                       struct bio_container *pBioc)
{

    dprintk("%s bioc(%p) type %d source %llu lpn %llu\n",
            __FUNCTION__, pBioc, pBioc->write_type, pBioc->source_pbn,
            pBioc->start_lbno);

    if (pBioc->write_type == TP_COPY_IO) {
        if (sflush_copy_submitwrite(pBioc) < 0) {
            sflush_copywrite_handler(ssf, pBioc);
        }
    } else {
        LOGERR("unknown write type: %d\n", pBioc->write_type);
    }

    return 0;
}

static int32_t sflush_read_handler(struct bio_container *pBioc)
{
    int32_t i;
    pBioc->bio->bi_iter.bi_sector = (pBioc->start_lbno) << SECTORS_PER_FLASH_PAGE_ORDER;    // to sector
    pBioc->bio->bi_iter.bi_sector |= MAPPING_TO_HDD_BIT;
    pBioc->bio->bi_iter.bi_size = FLASH_PAGE_SIZE;
    pBioc->bio->bi_vcnt = MEM_PER_FLASH_PAGE;
    pBioc->bio->bi_iter.bi_idx = 0;

    for (i = 0; i < MEM_PER_FLASH_PAGE; i++) {
        pBioc->bio->bi_io_vec[i].bv_len = PAGE_SIZE;    // 4096
        pBioc->bio->bi_io_vec[i].bv_offset = 0;
    }
    pBioc->io_queue_fin = 0;
    pBioc->bio->bi_end_io = end_bio_write_sflush_hybrid;
    //printk(" send write bi_sector= %llu\n",pBioc->bio->bi_iter.bi_sector);
    if (my_make_request(pBioc->bio) < 0) {
        LFSM_ASSERT(0);
    }
    //hybrid_sys_writephase_done(&td->hybrid_sys,pBioc);
    return (0);
}

static int sflush_read_process(void *vssf)
{
    struct bio_container *pBioc, *pNextBioC;
    ssflush_t *ssf;
    unsigned long flag;

    ssf = (ssflush_t *) vssf;
    pBioc = 0;
    current->flags |= PF_MEMALLOC;

    while (true) {
        lfsm_wait_event_interruptible(ssf->wq_proc_finish_r,
                                      atomic_read(&ssf->len_finish_read) > 0
                                      || kthread_should_stop());
        if (kthread_should_stop()) {
            return 0;
        }
        spin_lock_irqsave(&ssf->finish_r_list_lock, flag);

        if (list_empty(&ssf->finish_r_list)) {
            spin_unlock_irqrestore(&ssf->finish_r_list_lock, flag);
            continue;
        }

        list_for_each_entry_safe(pBioc, pNextBioC, &ssf->finish_r_list,
                                 submitted_list) {
            list_del_init(&pBioc->submitted_list);
            atomic_dec(&ssf->len_finish_read);
            break;
        }

        spin_unlock_irqrestore(&ssf->finish_r_list_lock, flag);
        //printk("biocbh: got_a_bio %p %llu %llu fin : %d\n",pBioc,pBioc->start_lbno,pBioc->end_lbno,pBioc->io_queue_fin);

        if (pBioc->io_queue_fin != 1) { //add fail bio to fail list in biocbh path
            put_err_bioc_to_err_list(&lfsmdev.biocbh, pBioc);
            continue;
        }

        if (ssf->id_user == ID_USER_CROSSCOPY) {
            sflush_copyread_handler(ssf, pBioc);
        } else if (ssf->id_user == ID_USER_GCWRITE) {
            sflush_gcwrite_handler(ssf, pBioc, false);
        } else {                //ID_USER_AUTOFLUSH
            sflush_read_handler(pBioc);
        }
    }

    return 0;
}

static int32_t sflush_write_handler(ssflush_t * ssf,
                                    struct bio_container *pBioc)
{
    lfsm_dev_t *td;
    // set insert_one pending in previous procedure
    //PPDM_BMT_cache_insert_one_pending(td, pBioc->start_lbno,PBNO_SPECIAL_ON_HD, 1,true);
    //printk(" ppq_update_done lbno %llu\n",pBioc->start_lbno);

    td = ssf->ptd;
    bioc_bio_free(td, pBioc);
    atomic_dec(&(ssf->non_ret_bio_cnt));
    wake_up_interruptible(&(ssf->wq_submitor));
    //printk(" finish_all \n");
    return (0);
}

static int sflush_write_process(void *vssf)
{
    struct bio_container *pBioc, *pNextBioC;
    ssflush_t *ssf;
    unsigned long flag;

    ssf = (ssflush_t *) vssf;
    current->flags |= PF_MEMALLOC;

    while (true) {
        lfsm_wait_event_interruptible(ssf->wq_proc_finish_w,
                                      atomic_read(&ssf->len_finish_write) > 0
                                      || kthread_should_stop());
        if (kthread_should_stop()) {
            return 0;
        }

        spin_lock_irqsave(&ssf->finish_w_list_lock, flag);

        if (list_empty(&ssf->finish_w_list)) {
            spin_unlock_irqrestore(&ssf->finish_w_list_lock, flag);
            continue;
        }

        list_for_each_entry_safe(pBioc, pNextBioC, &ssf->finish_w_list,
                                 submitted_list) {
            list_del_init(&pBioc->submitted_list);
            atomic_dec(&ssf->len_finish_write);
            break;
        }
        spin_unlock_irqrestore(&ssf->finish_w_list_lock, flag);

        if (ssf->id_user == ID_USER_CROSSCOPY) {
            sflush_copywrite_handler(ssf, pBioc);
        } else {
            sflush_write_handler(ssf, pBioc);
        }
    }
    return 0;
}

static int32_t sflush_estimate(MD_L2P_item_t * ar_ppq, sector64 lpno_s,
                               int32_t page_num)
{
    return 0;
}

bool IS_GCWrite_THREAD(sflushops_t * ops, int32_t pid)
{
    ssflush_t *ssf;

    ssf = (ssflush_t *) (ops->variables);

    if (ssf->pthread_proc_finish_w->pid == pid) {
        return true;
    }

    return false;
}

bool sflush_init(lfsm_dev_t * td, sflushops_t * ops, int32_t id_user)
{
#ifdef  HLIST_GC_WHOLE_LOCK_DEBUG
    int8_t tName[64];
#endif
    ssflush_t *ssf;

    sf_printk("%s start\n", __FUNCTION__);
    ops->variables = (void *)
        track_kmalloc(sizeof(ssflush_t), GFP_KERNEL | __GFP_ZERO, M_IO);
    memset(ops->variables, 0, sizeof(ssflush_t));
    if (!(ops->variables)) {
        LOGERR("%s cannot allocate memory \n", __FUNCTION__);
        return false;
    }

    ssf = (ssflush_t *) (ops->variables);
    ssf->ptd = td;
    ssf->id_user = id_user;
    init_waitqueue_head(&ssf->wq_proc_finish_r);
    init_waitqueue_head(&ssf->wq_proc_finish_w);
    init_waitqueue_head(&ssf->wq_submitor);

    spin_lock_init(&(ssf->finish_r_list_lock));
    spin_lock_init(&(ssf->finish_w_list_lock));

    INIT_LIST_HEAD(&ssf->finish_r_list);
    INIT_LIST_HEAD(&ssf->finish_w_list);

    atomic_set(&ssf->len_finish_read, 0);
    atomic_set(&ssf->len_finish_write, 0);
    atomic_set(&ssf->non_ret_bio_cnt, 0);

    ssf->pthread_proc_finish_r =
        kthread_run(sflush_read_process, ssf, "lfsm_sfr%d", id_user);
    ssf->pthread_proc_finish_w =
        kthread_run(sflush_write_process, ssf, "lfsm_sfw%d", id_user);

    // process check
    if (IS_ERR_OR_NULL(&ssf->pthread_proc_finish_r)) {
        LOGERR("&ssf->pthread_proc_finish_r NULL, ssf init fail\n");
        sflush_destroy(ops);
        return false;
    }
#ifdef HLIST_GC_WHOLE_LOCK_DEBUG
    memset(tName, '\0', 64);
    sprintf(tName, "lfsm_sfr%d", id_user);
    reg_thread_monitor(ssf->pthread_proc_finish_r->pid, tName);
#endif

    if (IS_ERR_OR_NULL(&ssf->pthread_proc_finish_w)) {
        LOGERR("&ssf->pthread_proc_finish_w NULL, ssf init fail\n");
        sflush_destroy(ops);
        return false;
    }
#ifdef HLIST_GC_WHOLE_LOCK_DEBUG
    memset(tName, '\0', 64);
    sprintf(tName, "lfsm_sfw%d", id_user);
    reg_thread_monitor(ssf->pthread_proc_finish_w->pid, tName);
#endif
    return true;
}

bool sflush_destroy(sflushops_t * ops)
{
    ssflush_t *ssf;

    ssf = (ssflush_t *) (ops->variables);
    if (IS_ERR_OR_NULL(ops->variables)) {
        return 0;
    }

    if (!IS_ERR_OR_NULL(&ssf->pthread_proc_finish_r)) {
        if (kthread_stop(ssf->pthread_proc_finish_r) < 0) {
            LOGERR("can't stop ssf->pthread_proc_finish_r\n");
        }
    }

    if (!IS_ERR_OR_NULL(&ssf->pthread_proc_finish_w)) {
        if (kthread_stop(ssf->pthread_proc_finish_w) < 0) {
            LOGERR("can't stop ssf->pthread_proc_finish_w\n");
        }
    }

    if (ssf) {
        track_kfree(ssf, sizeof(ssflush_t), M_IO);
        ops->variables = 0;
    }

    return 0;
}

void sflush_end_bio_gcwrite_read_brm(struct bio *bio)
{
    struct bio_container *entry;
    lfsm_dev_t *td;

    entry = bio->bi_private;
    td = entry->lfsm_dev;
    if (bio->bi_status == BLK_STS_OK) {
        entry->io_queue_fin = 1;
    } else {
        entry->io_queue_fin = blk_status_to_errno(bio->bi_status);
    }

    sflush_readphase_done((ssflush_t *) (td->gcwrite.variables), entry);
    return;
}

void sflush_end_bio_gcwrite_read(struct bio *bio)
{
    struct bio_container *entry;
    lfsm_dev_t *td;

    entry = bio->bi_private;
    td = entry->lfsm_dev;
    if (bio->bi_status == BLK_STS_OK) {
        entry->io_queue_fin = 1;
    } else {
        entry->io_queue_fin = blk_status_to_errno(bio->bi_status);
    }
    atomic_set(&bio->__bi_cnt, 1);
    track_bio_put(bio);         // relesae bio in read
    sflush_readphase_done((ssflush_t *) (td->gcwrite.variables), entry);
    return;
}
