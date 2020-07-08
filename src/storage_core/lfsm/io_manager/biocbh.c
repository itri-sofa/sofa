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
#include "io_read.h"
#include "../special_cmd.h"
#include "../err_manager.h"
#include "../GC.h"
#include "../devio.h"
#include "../sflush.h"
#include "../spare_bmt_ul.h"
#include "../bmt.h"

#ifdef THD_EXEC_STEP_MON
#include "../lfsm_thread_monitor.h"
#endif
#include "../perf_monitor/thd_perf_mon.h"
#include "../perf_monitor/ssd_perf_mon.h"
#include "../perf_monitor/ioreq_pendstage_mon.h"
#include "../perf_monitor/calculate_iops.h"

static int32_t biocbh_bioc_copywrite_handler(lfsm_dev_t * td,
                                             struct bio_container *pBioc)
{
    //////////////////////
    if (!rddcy_end_stripe(td, pBioc)) {
        LOGERR("rddcy_end_stripe return fail pbioc=%p\n", pBioc);
    }
    //////////////////////

    // AN: move to here add the fail handling policy inside update_ppq_and_bitmap
    update_ppq_and_bitmap(td, pBioc);
    // error path for copy pipeline
    if (pBioc->status != BLK_STS_OK) {
        LOGERR("error pbioc(%p)\n", pBioc);
        put_err_bioc_to_err_list(&td->biocbh, pBioc);
        return -EIO;
    }
    //if(atomic_read(&eu->cn_pinned_pages)%100==0)
    //printk("%s pBioc(%p) source %llu dest %llu pin %d\n",__FUNCTION__,pBioc,pBioc->source_pbn,
    //    (sector64)pBioc->bio->bi_iter.bi_sector-SECTORS_PER_FLASH_PAGE,atomic_read(&eu->cn_pinned_pages));

    HPEUgc_dec_pin_and_wakeup(pBioc->source_pbn);
    return 0;
}

static int32_t biocbh_bioc_write_handler(lfsm_dev_t * td,
                                         struct bio_container *pBioc,
                                         int32_t id_bh_thread)
{
    if (!rddcy_end_stripe(td, pBioc)) {
        LOGERR("rddcy_end_stripe return fail pbioc=%p\n", pBioc);
    }
    // AN: move to here add the fail handling policy inside update_ppq_and_bitmap

    update_ppq_and_bitmap(td, pBioc);
    //if(pBioc->io_queue_fin < 0)
    if (pBioc->status != BLK_STS_OK) {
        //printk("bi_sector: %llu , page : %llu, bi_size : %d\n",(pBioc->bio->bi_iter.bi_sector),(pBioc->bio->bi_iter.bi_sector)>> SECTORS_PER_FLASH_PAGE_ORDER,(pBioc->bio->bi_iter.bi_size));
        put_err_bioc_to_err_list(&td->biocbh, pBioc);
        return -1;
    }

    if (pBioc->org_bio == 0) {
        goto l_end;             // ECC page
    }

    LFSM_ASSERT((pBioc->write_type == TP_FLUSH_IO) || (pBioc->org_bio));
    td->biocbh.cnt_writeIO_done[id_bh_thread]++;

    post_rw_check_waitlist(td, pBioc, 0);   // tifa 0909
#if LFSM_PAGE_SWAP==1
    bioc_resp_user_bio(pBioc->org_bio, 0);
#endif

  l_end:
    put_bio_container(td, pBioc);

    return 0;
}

// an-nan after
static int32_t biocbh_bioc_read_handler(lfsm_dev_t * td,
                                        struct bio_container *pBioc,
                                        int32_t err, int32_t id_bh_thread)
{
    if (err == 0) {
        td->biocbh.cnt_readIO_done[id_bh_thread]++; //iostat
        if (bioc_update_user_bio(pBioc, pBioc->org_bio)) {
            err = -EIO;
        }
    }
    // ret!=0 info post_rw that this bio_c has failed, so do not satisfy any dependency from memory
    lfsm_resp_user_bio_free_bioc(td, pBioc, err, id_bh_thread);
    return 0;
}

static int32_t biocbh_bioc_handler(lfsm_dev_t * td, struct bio_container *pBioc,
                                   int32_t id_bh_thread)
{
    int32_t ret;

    do {
        if (pBioc->write_type == TP_GC_IO) {
            ret = biocbh_bioc_copywrite_handler(td, pBioc);
            break;
        }

        if (bio_data_dir(pBioc->bio) == WRITE) {
            ret = biocbh_bioc_write_handler(td, pBioc, id_bh_thread);
            break;
        }
        //if ((pBioc->io_queue_fin == 1) || (pBioc->io_queue_fin == -ENODEV)) {
        ret = biocbh_bioc_read_handler(td, pBioc,
                                       pBioc->io_queue_fin >=
                                       0 ? 0 : pBioc->io_queue_fin,
                                       id_bh_thread);
        break;
        //}

        ret = 0;
    } while (0);

    return ret;
}

static int biocbh_process(void *vtd)
{
    lfsm_dev_t *td;
    biocbh_t *pbiocbh;
    struct bio_container *pBioc, *pNextBioC;
    int64_t t_start;
    unsigned long flag;
    int32_t id_bh_thread, io_rw;
    bool do_dec;

    td = (lfsm_dev_t *) vtd;
    pbiocbh = &td->biocbh;
    pBioc = NULL;
    id_bh_thread = atomic_inc_return(&td->biocbh.cn_thread_biocbh) - 1;

    current->flags |= PF_MEMALLOC;

    mp_printk("biocbh_process pid is %d\n", current->pid);
    biocbh_printk("biocbh_process: lock address %p\n",
                  &pbiocbh->ar_lock_biocbh[id_bh_thread]);

    while (true) {
        lfsm_wait_event_interruptible(pbiocbh->ar_wq[id_bh_thread],
                                      kthread_should_stop() ||
                                      (atomic_read
                                       (&pbiocbh->
                                        ar_len_io_done[id_bh_thread]) > 0));
        if (kthread_should_stop()) {
            break;
        }

        t_start = jiffies_64;

        spin_lock_irqsave(&pbiocbh->ar_lock_biocbh[id_bh_thread], flag);
        if (list_empty(&pbiocbh->ar_io_done_list[id_bh_thread])) {
            spin_unlock_irqrestore(&pbiocbh->ar_lock_biocbh[id_bh_thread],
                                   flag);
            continue;
        }

        list_for_each_entry_safe(pBioc, pNextBioC,
                                 &pbiocbh->ar_io_done_list[id_bh_thread],
                                 submitted_list) {
            list_del_init(&pBioc->submitted_list);
            break;
        }
        DEC_IOR_STAGE_CNT(IOR_STAGE_WAIT_BHT, id_bh_thread);
        spin_unlock_irqrestore(&pbiocbh->ar_lock_biocbh[id_bh_thread], flag);

        atomic_dec(&pbiocbh->ar_len_io_done[id_bh_thread]);
        INC_IOR_STAGE_CNT(IOR_STAGE_PROCESS_BHT, id_bh_thread);

        //TODO: remove it at future
        //HOOK_INSERT(fn_biocbh_process, HOOK_ARG(arg_biocbh_process_usec));

        mp_printk("biocbh: got_a_bio %p %llu %llu fin : %d\n",
                  pBioc, pBioc->start_lbno, pBioc->end_lbno,
                  pBioc->io_queue_fin);
        io_rw = pBioc->io_rw;
        do_dec = pBioc->is_user_bioc ? true : false;
        biocbh_bioc_handler(td, pBioc, id_bh_thread);

        DEC_IOR_STAGE_CNT(IOR_STAGE_PROCESS_BHT, id_bh_thread);
        if (do_dec) {
            INC_IOR_STAGE_CNT(IOR_STAGE_LFSMBH_PROCESS_DONE, id_bh_thread);
        }
        LOG_THD_PERF(current->pid, io_rw, t_start, jiffies_64);
    }

    return 0;
}

//reread and if success,hook eu to the original eu
static int32_t err_bioc_handler_GC_reread(lfsm_dev_t * td,
                                          struct bio_container *pBioc)
{
    struct bad_block_info bad_blk_info;
    bio_end_io_t *ar_bi_end_io[MAX_ID_ENDBIO];
    int32_t ret;

    ar_bi_end_io[ID_ENDBIO_BRM] = sbrm_end_bio_generic_read;
    ar_bi_end_io[ID_ENDBIO_NORMAL] = end_bio_page_read;

    if (pBioc->io_queue_fin < 0) {
        if ((ret =
             special_read_enquiry(pBioc->source_pbn, 1, 0, 0,
                                  &bad_blk_info)) < 0) {
            goto l_rddcy;
        }
    }

    if ((ret =
         iread_mustread(td, pBioc->source_pbn, pBioc, 0, false,
                        ar_bi_end_io)) == 0) {
        goto l_end;
    }

  l_rddcy:
    errmgr_bad_process(pBioc->source_pbn, pBioc->start_lbno, pBioc->start_lbno, td, false, false);  // log one error
    if (pBioc->write_type == TP_COPY_IO) {
        goto l_end;
    } else {                    //TP_GC_IO
        if ((ret = rddcy_fpage_read(td, pBioc->source_pbn, pBioc, 0)) < 0) {
            goto l_end;
        }
    }

  l_end:
    //if read success,hook back to sflush read
    if (pBioc->write_type == TP_COPY_IO) {
        if (ret < 0) {
            pBioc->start_lbno = TAG_SPAREAREA_BADPAGE;
            LOGERR("bioc(%p) source %llu dest %llu ret=%d\n", pBioc,
                   pBioc->source_pbn, pBioc->dest_pbn, ret);
        }
        pBioc->io_queue_fin = 1;
        sflush_readphase_done((struct ssflush *)(td->crosscopy.variables),
                              pBioc);
    } else {                    //GC copy read
        if (ret < 0) {
            LOGINFO("biobh copyread fail ret %d, bioc %p fail source %lld\n",
                    ret, pBioc, pBioc->source_pbn);
            HPEUgc_dec_pin_and_wakeup(pBioc->source_pbn);
        } else {
            pBioc->io_queue_fin = 1;
            sflush_readphase_done((struct ssflush *)(td->gcwrite.variables),
                                  pBioc);
        }
    }
    return ret;
}

static void err_bioc_handler_read(lfsm_dev_t * td, struct bio_container *pBioc)
{

//    err_handle_printk("%s: bi_sector: %llu , page : %llu, bi_size : %d\n",__FUNCTION__,
//        (sector64)(pBioc->bio->bi_iter.bi_sector),(sector64)(pBioc->bio->bi_iter.bi_sector)>> SECTORS_PER_FLASH_PAGE_ORDER,(pBioc->bio->bi_iter.bi_size));

    iread_bio_bh(td, pBioc->org_bio, pBioc, false, -1);
}

static int32_t biocbh_ErrorSummary(struct bad_block_info *bad_blk_info)
{
    int32_t i, ret;

    ret = LFSM_ERR_WR_RETRY;
    for (i = 0; i < bad_blk_info->num_bad_items; i++) {
        //spec_read_enquiry shall return the failure in the correct range
        err_handle_printk("biobh err_bio_handle[%d]={%d}\n", i,
                          bad_blk_info->errors[i].logical_num);
        if (bad_blk_info->errors[i].failure_code != LFSM_ERR_WR_RETRY) {
            ret = LFSM_ERR_WR_ERROR;
            break;
        }
    }
    return ret;
}

static void bioc_reinit_bio(struct bio_container *pBioc,
                            sector64 bi_sector_bf_w, sector64 bi_size_bf_w)
{
    int32_t i;

    pBioc->status = BLK_STS_OK;
    pBioc->bio->bi_iter.bi_sector = bi_sector_bf_w;
    pBioc->bio->bi_iter.bi_size = bi_size_bf_w;
    pBioc->bio->bi_vcnt = (pBioc->bio->bi_iter.bi_size) / PAGE_SIZE;
    err_handle_printk
        ("bioc %p biobh retry bi_sector %llu bi_size %d vcntcount %d\n", pBioc,
         (sector64) pBioc->bio->bi_iter.bi_sector, pBioc->bio->bi_iter.bi_size,
         pBioc->bio->bi_vcnt);
    pBioc->bio->bi_status = BLK_STS_OK;
    pBioc->bio->bi_flags &= ~(1 << BIO_SEG_VALID);
    for (i = 0; i < pBioc->bio->bi_vcnt; i++) {
        pBioc->bio->bi_io_vec[i].bv_len = PAGE_SIZE;
        pBioc->bio->bi_io_vec[i].bv_offset = 0;
    }
    pBioc->bio->bi_phys_segments = 0;
    pBioc->bio->bi_seg_front_size = 0;
    pBioc->bio->bi_seg_back_size = 0;
    pBioc->bio->bi_next = NULL;
    pBioc->bio->bi_iter.bi_idx = 0;
    return;
}

static int32_t lfsm_err_write_retry(lfsm_dev_t * td,
                                    struct bio_container *pBioc,
                                    sector64 bi_sector_bf_w,
                                    int32_t bi_size_bf_w, bool is_bh)
{
    int32_t cnt, wait_ret;

    err_handle_printk("biobh LFSM_ERR_WR_RETRY again bioc %p\n", pBioc);
    // adjust the bi_sector and bi_size to original
    bioc_reinit_bio(pBioc, bi_sector_bf_w, bi_size_bf_w);
    pBioc->io_queue_fin = 0;
    if (is_bh) {
        if (my_make_request(pBioc->bio) < 0) {
            return -ENODEV;
        }
    } else {
        if (my_make_request(pBioc->bio) < 0) {
            return -ENODEV;
        }
        //lfsm_wait_event_interruptible(pBioc->io_queue, pBioc->io_queue_fin != 0);
        cnt = 0;
        while (true) {
            cnt++;
            wait_ret = wait_event_interruptible_timeout(pBioc->io_queue,
                                                        pBioc->io_queue_fin !=
                                                        0,
                                                        LFSM_WAIT_EVEN_TIMEOUT);
            if (wait_ret <= 0) {
                LOGWARN
                    ("err write retry IO no response %lld after seconds %d\n",
                     (sector64) pBioc->bio->bi_iter.bi_sector,
                     LFSM_WAIT_EVEN_TIMEOUT_SEC * cnt);
            } else {
                break;
            }
        }

        if (pBioc->io_queue_fin <= 0) {
            err_handle_printk("LFSM_retry_fail_again bioc %p %d\n", pBioc,
                              pBioc->io_queue_fin);
            return -EAGAIN;     // do again special query
        }
    }

    return 0;
}

static int32_t bioc_err_write(lfsm_dev_t * td, struct bio_container *pBioc,
                              bool is_bh)
{
    struct bad_block_info bad_blk_info;
    sector64 bi_sector_bf_w, bi_size_bf_w /* byte unit */ ;
    int32_t ret, cn_badblock, failure_code;

    ret = -1;
    cn_badblock = 0;
    failure_code = LFSM_ERR_WR_RETRY;

  l_begin:
    LOGINFO("bi_sector: %llu , page : %llu, bi_size : %d, bi_dev : %p\n",
            (sector64) (pBioc->bio->bi_iter.bi_sector),
            (sector64) (pBioc->bio->bi_iter.
                        bi_sector) >> SECTORS_PER_FLASH_PAGE_ORDER,
            (pBioc->bio->bi_iter.bi_size), pBioc->bio->bi_disk);

    if (pBioc->par_bioext) {
        pBioc->bio = pBioc->par_bioext->bio_orig;
        bioext_free(pBioc->par_bioext, bioc_cn_fpage(pBioc));
        init_bio_container(pBioc, pBioc->org_bio,
                           NULL, NULL, REQ_OP_WRITE, 0,
                           bioc_cn_fpage(pBioc) * MEM_PER_FLASH_PAGE);

        return -EAGAIN;
    }

    bi_sector_bf_w = pBioc->bio->bi_iter.bi_sector = pBioc->dest_pbn;
    bi_size_bf_w = pBioc->bio->bi_iter.bi_size = pBioc->bytes_submitted;

    pBioc->bio->bi_disk = NULL;
    if (pBioc->dest_pbn == (sector64) (-1)) {
        LOGERR("Internal- pBioc(%p)->dest shall be assigned\n", pBioc);
        goto l_fail_reget_pbno;
    }

    cn_badblock = special_read_enquiry(bi_sector_bf_w,
                                       bi_size_bf_w >> FLASH_PAGE_ORDER
                                       /*unit page */ , 0, 0,
                                       &bad_blk_info);
    if (cn_badblock < 0) {
        LOGINFO("fail to exec special query (%d)\n", cn_badblock);
        goto l_fail_reget_pbno;
    }

    if (cn_badblock != bad_blk_info.num_bad_items) {
        LOGINFO("cn_badblock %d != %d\n", cn_badblock,
                bad_blk_info.num_bad_items);
        goto l_fail_reget_pbno;
    }

    if (cn_badblock > 0) {
        failure_code = biocbh_ErrorSummary(&bad_blk_info);
    } else {                    // cn_badblock=0
        // spec read  result is no fail, set the return code =0 ( no error)
        // direct put the bioc to io_done_list
        failure_code = devio_compare(&pBioc->bio->bi_io_vec[0], bi_sector_bf_w);
        if (failure_code == LFSM_NO_FAIL) {
            pBioc->status = BLK_STS_OK;
            LOGINFO
                ("biobh ssd_check_contect_no_error bi_sector: %llu , page : %llu, bi_size : %d\n",
                 (sector64) (pBioc->bio->bi_iter.bi_sector),
                 (sector64) (pBioc->bio->bi_iter.
                             bi_sector) >> SECTORS_PER_FLASH_PAGE_ORDER,
                 (pBioc->bio->bi_iter.bi_size));
            if (is_bh) {
                biocbh_io_done(&td->biocbh, pBioc, false);
            }
            return 0;
        }
    }

    if (failure_code == LFSM_ERR_WR_RETRY) {
        ret =
            lfsm_err_write_retry(td, pBioc, bi_sector_bf_w, bi_size_bf_w,
                                 is_bh);
        // ret= -ENODEV, -EAGAIN, 0
        if (ret == -ENODEV) {
            return -EAGAIN;
        }

        if (ret == -EAGAIN) {
            goto l_begin;       // do again special query
        }
        return 0;
    }

  l_fail_reget_pbno:
    // true fail, start write fail handling
    if (pBioc->write_type == TP_GC_IO) {
        LOGINFO("biobh true fail, bioc %p source %lld fail dest %llu\n",
                pBioc, pBioc->source_pbn, bi_sector_bf_w);
    }

    errmgr_bad_process(bi_sector_bf_w, pBioc->start_lbno, pBioc->end_lbno, td,
                       false, true);
    // get new dest pdno  and write again
    init_bio_container(pBioc, pBioc->org_bio,
                       NULL, NULL, REQ_OP_WRITE, 0,
                       bioc_cn_fpage(pBioc) * MEM_PER_FLASH_PAGE);
    return -EAGAIN;
}

static void err_bioc_handler_write(lfsm_dev_t * td, struct bio_container *pBioc)
{
    int32_t ret;
    ret = bioc_err_write(td, pBioc, true);  // -EAGAIN and 0
    if (ret == 0) {
        return;
    }
    // AN:2014/4/8 the dest_pbno is finished write and failed , dec unwrite count
    // don't known how to do fail handling

    //HPEU_dec_unfinish(&td->hpeutab,pBioc->dest_pbn);
    LOGINFO("pBioc->dest_pbn %llu\n", pBioc->dest_pbn);

    if ((RDDCY_IS_ECCLPN(pBioc->start_lbno))
        || (pBioc->start_lbno == LBNO_PREPADDING_PAGE)) {
        LFSM_ASSERT2(pBioc->org_bio == NULL, "bioc(%p) is ecc or padding org_bio not null\n", pBioc);   // It can be removed if stable. Tifa 2014/1/15
        if (pBioc->write_type == TP_GC_IO) {
            LOGERR("GGG: why bioc(%p) source %llu is gc without org_bio\n",
                   pBioc, pBioc->source_pbn);
        }
        put_bio_container(td, pBioc);
        return;
    }

    if (pBioc->write_type == TP_PARTIAL_IO || pBioc->write_type == TP_NORMAL_IO) {
        iwrite_bio_bh(td, pBioc, true); //
    } else if (pBioc->write_type == TP_GC_IO) {
        LOGERR("retry gcwrite pbioc(%p) source %llu\n", pBioc,
               pBioc->source_pbn);
        sflush_gcwrite_handler(td->gcwrite.variables, pBioc, true);
    } else {
        LOGERR("unknown pBioc %p write_type %d \n", pBioc, pBioc->write_type);
    }
}

static int fail_bioc_process(void *vtd)
{
    lfsm_dev_t *td;
    biocbh_t *pbiocbh;
    bioconter_t *pBioc, *pNextBioC;
    unsigned long flag;

    td = (lfsm_dev_t *) vtd;
    pbiocbh = &td->biocbh;
    pBioc = NULL;

    current->flags |= PF_MEMALLOC;
    mp_printk("fail_bioc_process pid is %d\n", current->pid);
    biocbh_printk("fail_process: lock address %p\n", &pbiocbh->lock_fail_bh);

    while (true) {
        lfsm_wait_event_interruptible(pbiocbh->ehq,
                                      kthread_should_stop() ||
                                      (atomic_read(&pbiocbh->len_fail_bioc) >
                                       0));
        if (kthread_should_stop()) {
            break;
        }

        spin_lock_irqsave(&pbiocbh->lock_fail_bh, flag);

        if (list_empty(&pbiocbh->handle_fail_bioc_list)) {
            spin_unlock_irqrestore(&pbiocbh->lock_fail_bh, flag);
            continue;
        }

        list_for_each_entry_safe(pBioc, pNextBioC,
                                 &pbiocbh->handle_fail_bioc_list,
                                 submitted_list) {
            list_del_init(&pBioc->submitted_list);
            atomic_dec(&pbiocbh->len_fail_bioc);
            break;
        }

        spin_unlock_irqrestore(&pbiocbh->lock_fail_bh, flag);
//        err_handle_printk("fail_process: got a bio %p %llu %llu\n",pBioc,pBioc->start_lbno,pBioc->end_lbno);
        if (((pBioc->write_type == TP_GC_IO) &&
             (bio_data_dir(pBioc->bio) == READ))
            || (pBioc->write_type == TP_COPY_IO)) {
            err_bioc_handler_GC_reread(td, pBioc);
        } else if ((bio_data_dir(pBioc->bio) == WRITE) ||
                   ((pBioc->write_type == TP_GC_IO)
                    && (bio_data_dir(pBioc->bio) == WRITE))) {
            err_bioc_handler_write(td, pBioc);
        } else {
            err_bioc_handler_read(td, pBioc);
        }
    }
    return 0;
}

/*
 * Description: init back ground process threads
 *              default: 3 lfsm_bh   threads handling IO
 *                       1 lfsm_fail thread handling failing case
 */
bool biocbh_init(biocbh_t * pbiocbh, lfsm_dev_t * td)
{
    int8_t nm_bh[32];
    int32_t i;

    init_waitqueue_head(&pbiocbh->ehq);
    INIT_LIST_HEAD(&pbiocbh->handle_fail_bioc_list);
    atomic_set(&pbiocbh->len_fail_bioc, 0);

    atomic_set(&pbiocbh->cnt_readNOPPNIO_done, 0);
    atomic_set(&pbiocbh->cn_thread_biocbh, 0);

    for (i = 0; i < lfsmCfg.cnt_bh_threads; i++) {
        init_waitqueue_head(&pbiocbh->ar_wq[i]);
        spin_lock_init(&(pbiocbh->ar_lock_biocbh[i]));
        INIT_LIST_HEAD(&pbiocbh->ar_io_done_list[i]);
        pbiocbh->cnt_readIO_done[i] = 0;
        pbiocbh->cnt_writeIO_done[i] = 0;
        atomic_set(&pbiocbh->ar_len_io_done[i], 0);
    }

    for (i = 0; i < lfsmCfg.cnt_bh_threads; i++) {
        sprintf(nm_bh, "lfsm_bh%d", i);
        if (lfsmCfg.bhT_vcore_map[i] == -1) {
            pbiocbh->pthread_bh[i] = kthread_run(biocbh_process, td, nm_bh);
        } else {
            pbiocbh->pthread_bh[i] = my_kthread_run(biocbh_process, td, nm_bh,
                                                    lfsmCfg.bhT_vcore_map[i]);
        }

        if (!IS_ERR_OR_NULL(pbiocbh->pthread_bh[i])) {
            reg_thd_perf_monitor(pbiocbh->pthread_bh[i]->pid, LFSM_IO_BH_THD,
                                 nm_bh);
#ifdef THD_EXEC_STEP_MON
            reg_thread_monitor(pbiocbh->pthread_bh[i]->pid, nm_bh);
#endif
        }
    }

    for (i = 0; i < HANDLE_FAIL_PROCESS_NUM; i++) {
        sprintf(nm_bh, "lfsm_fail%d", i);
        pbiocbh->pthread_handle_fail_process[i] =
            kthread_run(fail_bioc_process, td, nm_bh);

#ifdef THD_EXEC_STEP_MON
        if (!IS_ERR_OR_NULL(pbiocbh->pthread_handle_fail_process[i])) {
            reg_thread_monitor(pbiocbh->pthread_handle_fail_process[i]->pid,
                               nm_bh);
        }
#endif
    }

    // process check
    for (i = 0; i < lfsmCfg.cnt_bh_threads; i++) {
        if (IS_ERR_OR_NULL(pbiocbh->pthread_bh[i])) {
            //kthread_stop(pbiocbh->pthread);
            LOGERR("fail run pbiocbh->pthread_bh[%d]\n", i);
            biocbh_destory(pbiocbh);
            return false;
        }
    }

    for (i = 0; i < HANDLE_FAIL_PROCESS_NUM; i++) {
        if (IS_ERR_OR_NULL(pbiocbh->pthread_handle_fail_process[i])) {
            LOGERR("fail run pbiocbh->pthread_handle_fail_process[%d]\n", i);
            biocbh_destory(pbiocbh);
            return false;
        }
    }

    pbiocbh->ready = true;

    return true;
}

void biocbh_destory(biocbh_t * pbiocbh)
{
    int32_t i;

    if (IS_ERR_OR_NULL(pbiocbh)) {
        return;
    }
//    atomic_inc(&pbiocbh->len_fail_bioc);
    for (i = 0; i < lfsmCfg.cnt_bh_threads; i++) {
//        atomic_inc(&pbiocbh->ar_len_io_done[i]);
        if (IS_ERR_OR_NULL(pbiocbh->pthread_bh[i])) {
            LOGERR("pbiocbh->pthread_bh[%d] is NULL when stop\n", i);
            continue;
        }

        if (kthread_stop(pbiocbh->pthread_bh[i]) < 0) {
            LOGERR("can't stop pbiocbh->pthread_bh[%d]\n", i);
        }

        pbiocbh->pthread_bh[i] = NULL;
    }

    for (i = 0; i < HANDLE_FAIL_PROCESS_NUM; i++) {
        if (IS_ERR_OR_NULL(pbiocbh->pthread_handle_fail_process[i])) {
            LOGERR
                ("pbiocbh->pthread_handle_fail_process[%d] is NULL when stop\n",
                 i);
            continue;
        }

        if (kthread_stop(pbiocbh->pthread_handle_fail_process[i]) < 0) {
            LOGERR("can't stop pthread_handle_fail_process[%d]\n", i);
        }

        pbiocbh->pthread_handle_fail_process[i] = NULL;
    }

    pbiocbh->ready = false;
}

/////////////////////////////////////////////////////////////////////////////////////////////////

void biocbh_io_done(biocbh_t * pbiocbh, struct bio_container *pBioc,
                    bool in_issued_list)
{
    wait_queue_head_t *my_waitq;
    spinlock_t *mylock;
    unsigned long flag;
    int32_t idx;

#if DATALOG_GROUP_NUM > 10
    idx = pBioc->idx_group % lfsmCfg.cnt_bh_threads;
#else
    idx =
        lfsm_atomic_inc_return(&pbiocbh->idx_curr_thread) %
        lfsmCfg.cnt_bh_threads;
#endif
    LOG_SSD_PERF(pBioc->selected_diskID, pBioc->io_rw, pBioc->ts_submit_ssd,
                 jiffies_64);
    INC_RESP_IOPS();

    mylock = &pbiocbh->ar_lock_biocbh[idx];
    my_waitq = &pbiocbh->ar_wq[idx];

    spin_lock_irqsave(mylock, flag);
    list_add_tail(&pBioc->submitted_list, &pbiocbh->ar_io_done_list[idx]);
    if (pBioc->is_user_bioc) {
        INC_IOR_STAGE_CNT(IOR_STAGE_SSD_RESP, idx);
    }
    INC_IOR_STAGE_CNT(IOR_STAGE_WAIT_BHT, idx);
    spin_unlock_irqrestore(mylock, flag);

    atomic_inc(&pbiocbh->ar_len_io_done[idx]);

    if (waitqueue_active(my_waitq)) {
        wake_up_interruptible(my_waitq);
    }
}

void put_err_bioc_to_err_list(biocbh_t * pbiocbh, struct bio_container *pBioc)
{
    unsigned long flag;
//    bool empty=false;
    spin_lock_irqsave(&pbiocbh->lock_fail_bh, flag);
//    empty=list_empty(&pbiocbh->io_done_list);
    LOGERR("put_err_bioc %p to_err_list: start lbno %llu\n", pBioc,
           pBioc->start_lbno);
    list_add_tail(&pBioc->submitted_list, &pbiocbh->handle_fail_bioc_list);
    atomic_inc(&pbiocbh->len_fail_bioc);

//    if (empty)
    wake_up_interruptible(&pbiocbh->ehq);
    spin_unlock_irqrestore(&pbiocbh->lock_fail_bh, flag);
}

int32_t err_write_bioc_normal_path_handle(struct lfsm_dev_struct *td,
                                          struct bio_container *pBioc,
                                          bool isMetaData)
{
    int32_t ret;
    ret = bioc_err_write(td, pBioc, false); //-EAGAIN and 0
    if ((RDDCY_IS_ECCLPN(pBioc->start_lbno))
        || (pBioc->start_lbno == LBNO_PREPADDING_PAGE)) {
        LFSM_ASSERT2(pBioc->org_bio == NULL, "bioc(%p) is ecc or padding org_bio not null\n", pBioc);   // It can be removed if stable. Tifa 2014/1/15
        return 0;
    }
    return ret;
}
