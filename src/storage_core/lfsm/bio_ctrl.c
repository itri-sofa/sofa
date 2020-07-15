/*
 * SPDX-License-Identifier: GPL-2.0-or-later OR Apache-2.0
 * Copyright (C) 2015-2020 Industrial Technology Research Institute
 *
 *
 *
 *  
 */
#include <linux/types.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/wait.h>
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
#include "io_manager/io_common.h"
#include "io_manager/io_write.h"
#include "EU_create.h"
#include "EU_access.h"
#include "GC.h"
#include "spare_bmt_ul.h"
#include "bmt_ppq.h"
#include "lfsm_test.h"
#include "bmt_ppdm.h"
#include "err_manager.h"
#include "hpeu_gc.h"
#include "metabioc.h"
#include "mdnode.h"
#include "bio_ctrl.h"
#include "sysapi_linux_kernel.h"

#if LFSM_FLUSH_SUPPORT == 1
static void flushbio_end_bio(struct bio *bio)
{
    sflushbio_t *pflushbio = bio->bi_private;

    pflushbio = bio->bi_private;
    if (bio != pflushbio->user_bio) {
        free_composed_bio(bio, 0);
    }

    pflushbio->result_flushbio = bio->bi_status;

    //printk("callback  %p bio\n",bio);
    if (atomic_dec_return(&pflushbio->cn_pending_flush) == 0) {
        wake_up_interruptible(&pflushbio->wq);
    }
}

/*
 * Description: when user application issue flush IO, we will issue flush to ssds
 *              not work since compile flag LFSM_FLUSH_SUPPORT = 0
 */
sflushbio_t *flushbio_alloc(struct bio *pBio)
{
    sflushbio_t *pflushbio;

    pflushbio =
        track_kmalloc(sizeof(sflushbio_t), GFP_KERNEL | __GFP_ZERO, M_UNKNOWN);

    if (IS_ERR_OR_NULL(pflushbio)) {
        LOGERR("Cannot alloc memory to handle flush bio %p\n", pBio);
        pBio->bi_status = BLK_STS_RESOURCE;
        bio_endio(pBio);
        return NULL;
    }

    pflushbio->user_bio = pBio;
    pflushbio->user_bi_end_io = pBio->bi_end_io;
    pflushbio->user_bi_private = pBio->bi_private;

    pflushbio->result_flushbio = BLK_STS_OK;

    pBio->bi_end_io = flushbio_end_bio;
    pBio->bi_private = pflushbio;

    atomic_set(&pflushbio->cn_pending_tag, lfsmCfg.cnt_io_threads);
    atomic_set(&pflushbio->cn_pending_flush, grp_ssds.cn_drives + 1);   // one for the original bio
    init_waitqueue_head(&pflushbio->wq);

    return pflushbio;
}

static void flushbio_send_to_all_dev(sflushbio_t * pflushbio)
{
    struct block_device *bdev;
    struct bio *pBio;
    int32_t i;

    for (i = 0; i < grp_ssds.cn_drives; i++) {
        if (compose_bio_no_page(&pBio, grp_ssds.ar_drives[i].bdev,
                                flushbio_end_bio, pflushbio, 0, 0) < 0) {
            // decrease all value to make user thread don'wait
            atomic_sub(grp_ssds.cn_drives - i, &pflushbio->cn_pending_flush);
            pflushbio->result_flushbio = BLK_STS_IOERR;
            wake_up_interruptible(&pflushbio->wq);
            return;
        }

        if (IS_ERR_OR_NULL(bdev = diskgrp_get_drive(&grp_ssds, i))) {   //pgrp->ar_drives[dev_id].bdev;
            continue;
        }

        pBio->bi_opf = FLUSH_BIO_BIRW_FLAG;
        submit_bio(pBio);
        diskgrp_put_drive(&grp_ssds, i);
    }

    return;
}

static bool flushbio_filter(struct bio *pBio)
{
    sflushbio_t *pflushbio;

    if (pBio->bi_end_io != NULL)
        ) {
        return false;
        }

    pflushbio = pBio->bi_private;

    free_composed_bio(pBio, 0);

    if (atomic_dec_return(&pflushbio->cn_pending_tag) == 0) {
        flushbio_send_to_all_dev(pflushbio);    // the err case is set in pfslushbio's result_flush_bio
    }
    return true;
}
#endif

#if LFSM_FLUSH_SUPPORT == 1
//(1) send out the "marker" bio in every queue
//(2) wait for the marker bio is processed(pflushbio->cn_pending_tag==0)  and 
//                 flush bio send to every device is done ( pflushbio->cn_pending_flush==0)
//(3) free the allocated pflushbio and recovery user bio's callback and call user bio's end_io
bool flushbio_handler(sflushbio_t * pflushbio, struct bio *pBio,
                      bioboxpool_t * pBBP) {
    struct bio *bio;
    int32_t id_queue;
    bool ret;

     ret = true;
     id_queue = 0;

    for (id_queue = 0; id_queue < pBBP->cnt_io_workqueues; id_queue++) {
        if (compose_bio_no_page(&bio, NULL, NULL, (void *)pflushbio, 0, 0) < 0) {
            LOGWARN("Cannot compose bio, flushbio %p giveup\n", pBio);
            ret = false;
            break;
        }

        if (bioboxpool_free2used(pBBP, bio, id_queue)) {    // insert to every queue
            LOGWARN("Cannot insert bio, flushbio %p giveup \n", pBio);
            ret = false;
            break;
        }
        //LOGINFO("generate %d \n",id_queue);
    }

    wait_event_interruptible(pflushbio->wq,
                             atomic_read(&pflushbio->cn_pending_tag) == 0 &&
                             atomic_read(&pflushbio->cn_pending_flush) == 0);

    // recovery user bio's CB and private data
    pBio->bi_end_io = pflushbio->user_bi_end_io;
    pBio->bi_private = pflushbio->user_bi_private;
    // call userbio's endio
    pBio->bi_opf &= ~FLUSH_BIO_BIRW_FLAG;
    pBio->bi_status = pflushbio->result_flushbio;
    bio_endio(pBio);
    track_kfree(pflushbio, sizeof(sflushbio_t), M_UNKNOWN);

    return ret;
}
#endif

static void trim_bio_reg_time(SUpdateLog_t * pUL) {
    struct timeval trimbio_arrive;

    do_gettimeofday(&trimbio_arrive);
    LFSM_MUTEX_LOCK(&pUL->lock);
    if (pUL->last_trimbio_arrive < trimbio_arrive.tv_sec) {
        pUL->last_trimbio_arrive = trimbio_arrive.tv_sec;
    }

    LFSM_MUTEX_UNLOCK(&pUL->lock);
}

static int32_t trim_bio_handler(struct bio *bio, lfsm_dev_t * td) {
    int32_t ret;

    sector64 lpn_start = bio->bi_iter.bi_sector >> SECTORS_PER_FLASH_PAGE_ORDER;
    sector64 cn_fpages = BIO_SIZE_ALIGN_TO_FLASH_PAGE(bio) >> FLASH_PAGE_ORDER;
    //printk("receive trim bio %p \n",bio);
     ret = trim_bmt_table(td, lpn_start, cn_fpages);    // ret is used to bio_endio
    //ToDOupdatelog ??
    if (ret) {
        bio->bi_status = BLK_STS_IOERR;
    } else {
        bio->bi_status = BLK_STS_OK;
    }
    bio_endio(bio);
    // register the latest trim bio    finish time
    trim_bio_reg_time(&td->UpdateLog);
    return 0;
}

int32_t ctrl_bio_filter(lfsm_dev_t * td, struct bio *bio) {
#if LFSM_FLUSH_SUPPORT ==1
    if (flushbio_filter(bio)) {
        return 0;
    }
#endif
    if (IS_TRIM_BIO(bio)) {
        trim_bio_handler(bio, td);  // bio_endio inside
        return 0;
    }

    return -EAGAIN;
}

bool trim_bio_shall_reset_UL(SUpdateLog_t * pUL) {
    struct timeval current_tm;
    bool ret;

    ret = false;
    do_gettimeofday(&current_tm);
    LFSM_MUTEX_LOCK(&pUL->lock);

    if (pUL->last_trimbio_arrive == 0) {
        goto l_end;
    }

    if (current_tm.tv_sec - pUL->last_trimbio_arrive > 30) {
        pUL->last_trimbio_arrive = 0;
        ret = true;
    }

  l_end:
    LFSM_MUTEX_UNLOCK(&pUL->lock);
    return ret;
}

static int32_t ppq_trim(lfsm_dev_t * td, sector64 lpn_start, sector64 cn_lpn) {
    struct D_BMT_E dbmta;
    struct bio_container *pbioc;
    sector64 lpn, pbno;
    int32_t i, ret;

    ret = 0;
    //printk(" trim from %llu len %llu \n",lpn_start, cn_lpn);
    if (cn_lpn == 0) {          // handle the special case
        cn_lpn = DBMTE_NUM;
    }

    if (IS_ERR(pbioc = bioc_lock_lpns_wait(td, lpn_start, cn_lpn))) {
        LOGERR("Cannot lock lpn %llu , len %llu\n", lpn_start, cn_lpn);
        return PTR_ERR(pbioc);
    }

    for (i = 0; i < cn_lpn; i++) {
        lpn = lpn_start + i;

        if ((ret = bmt_lookup(td, td->ltop_bmt, lpn, false, 1, &dbmta)) < 0) {
            if (ret != -EACCES) {   // return failure for any error except -EACCES
                LOGERR("Fail to bmt lookup for lpn: %lld\n", lpn);
                bioc_unlock_lpns(td, pbioc);
                break;
            }
        }

        if ((pbno = (sector64) dbmta.pbno) == PBNO_SPECIAL_NEVERWRITE) {
            ret = PPDM_BMT_cache_remove_pin(td->ltop_bmt, lpn, 1);
        } else {
            update_old_eu(NULL, 1, &pbno);
            ret = PPDM_BMT_cache_insert_one_pending(td, lpn,
                                                    PBNO_SPECIAL_NEVERWRITE, 1,
                                                    true, DIRTYBIT_CLEAR,
                                                    false);
            // remove dirtybit and pininsert one pending
        }

        if (ret < 0) {
            LOGERR("Fail to clean lpn %llu for pbno %lld, ret=%d\n", lpn, pbno,
                   ret);
            break;
        }
    }

    bioc_unlock_lpns(td, pbioc);
    return ret;
}

int32_t trim_bmt_table(lfsm_dev_t * td, sector64 lpn_start, sector64 cn_pages)  // in flash page
{
    int32_t ret, id_ppq_start, cn_ppq, idx_ppq;

    ret = 0;
    id_ppq_start = LFSM_FLOOR_DIV(lpn_start, DBMTE_NUM);
    cn_ppq = LFSM_CEIL_DIV((lpn_start + cn_pages), DBMTE_NUM) - id_ppq_start;

    for (idx_ppq = id_ppq_start; idx_ppq < id_ppq_start + cn_ppq; idx_ppq++) {
        if (idx_ppq == id_ppq_start) {
            ret = ppq_trim(td, lpn_start, DBMTE_NUM - lpn_start % DBMTE_NUM);
        } else if (idx_ppq == id_ppq_start + cn_ppq - 1) {
            ret =
                ppq_trim(td, (idx_ppq * DBMTE_NUM),
                         (lpn_start + cn_pages) % DBMTE_NUM);
        } else {
            ret = ppq_trim(td, idx_ppq * DBMTE_NUM, DBMTE_NUM);
        }

        if (ret < 0) {
            LOGERR("trim fail in handling ppq id %d\n", idx_ppq);
            break;
        }
    }

    return ret;
}
