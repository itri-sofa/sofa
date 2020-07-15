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
#include <linux/bio.h>
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
#include "io_manager/io_common.h"
#include "EU_create.h"
#include "special_cmd.h"
#include "bmt_ppq.h"
#include "bmt_ppdm.h"
#include "GC.h"
#include "spare_bmt_ul.h"
#include "bmt.h"
#include "erase_count.h"
#include "lfsm_test.h"
#include "devio.h"
#include "metabioc.h"
#include "bmt_ppdm.h"
#include "mdnode.h"
// tifa: 2^nK safety 2012/2/13

seuinfo_t *gculrec_get_euinfo(GC_UL_record_t * prec, int32_t id_disk,
                              int32_t id_temp)
{
    return &prec->ar_active + (id_disk * TEMP_LAYER_NUM) + id_temp;
}

seuinfo_t *recparam_get_euinfo(srec_param_t * prec, int32_t id_disk,
                               int32_t id_temp)
{
//    return prec->par_active +(id_disk*TEMP_LAYER_NUM) + id_temp;
    seuinfo_t *pret;
    pret = (seuinfo_t *) ((int8_t *) prec + LFSM_CEIL(sizeof(srec_param_t),
                                                      sizeof(tp_ondisk_unit)));
    return &pret[id_disk * TEMP_LAYER_NUM + id_temp];
}

void recparam_set_eu_occupied(srec_param_t * prec, int32_t id_disk,
                              int32_t id_temp, int32_t value)
{
    int32_t *pis_eu_occupied;
    pis_eu_occupied = (int32_t *) ((int8_t *) prec +
                                   LFSM_CEIL(sizeof(srec_param_t),
                                             sizeof(tp_ondisk_unit)) +
                                   LFSM_CEIL(sizeof(seuinfo_t) *
                                             lfsmdev.cn_ssds * TEMP_LAYER_NUM,
                                             sizeof(tp_ondisk_unit)));
    pis_eu_occupied[id_disk * TEMP_LAYER_NUM + id_temp] = value;
}

int32_t recparam_get_eu_occupied(srec_param_t * prec, int32_t id_disk,
                                 int32_t id_temp)
{
    int32_t *pis_eu_occupied;
    pis_eu_occupied = (int32_t *) ((int8_t *) prec +
                                   LFSM_CEIL(sizeof(srec_param_t),
                                             sizeof(tp_ondisk_unit)) +
                                   LFSM_CEIL(sizeof(seuinfo_t) *
                                             lfsmdev.cn_ssds * TEMP_LAYER_NUM,
                                             sizeof(tp_ondisk_unit)));
    return pis_eu_occupied[id_disk * TEMP_LAYER_NUM + id_temp];
}

static void UpdateLog_change_next(SUpdateLog_t * pUL)
{
    SNextPrepare_renew(&pUL->next, EU_UPDATELOG);
}

// Author: Tifa
/*
 * Descirption: post handling after update log reset
 */
bool UpdateLog_hook(void *p)
{
    lfsm_dev_t *td;
    int32_t i;
    LOGINFO("UpdateLog hook called\n");

    td = (lfsm_dev_t *) p;
    PPDM_PAGEBMT_GCEU_disk_adjust(td->ltop_bmt);
    UpdateLog_change_next(&td->UpdateLog);
    dedicated_map_change_next(&td->DeMap);
    for (i = 0; i < td->cn_ssds; i++) {
        ECT_change_next(&gc[i]->ECT);
    }

    // weafon: hpeutab change next
    HPEU_change_next(&td->hpeutab);
    return true;
}

/*
 * Description: fill the active eu information in gc to update record
 */
static void _UL_GCentry_set_active(lfsm_dev_t * td, BMT_update_record_t * rec)
{
    seuinfo_t *peuinfo;
    int32_t i, j;

    for (i = 0; i < td->cn_ssds; i++) {
        for (j = 0; j < TEMP_LAYER_NUM; j++) {
            peuinfo = gculrec_get_euinfo(&rec->gc_record, i, j);
            peuinfo->eu_start = gc[i]->active_eus[j]->start_pos;
            peuinfo->frontier = gc[i]->active_eus[j]->log_frontier;
            peuinfo->id_hpeu = gc[i]->active_eus[j]->id_hpeu;
            peuinfo->birthday =
                HPEU_get_birthday(&td->hpeutab, gc[i]->active_eus[j]);
        }
    }
}

/* This function should be called holding the update_log_mutex, except during the initial call. */
/*
 * Description: Write Update log record to disk
 * Parameters :
 *     isPipeline : true : async write record to disk
 *                  flase: sync write record to disk
 * Return
 */
static int32_t UpdateLog_diskwrite(lfsm_dev_t * td, struct EUProperty *log_eu,
                                   int32_t temperature, int32_t log_marker,
                                   sector64 update_log_pbn,
                                   struct bio_container *bio_c, bool isPipeline)
{
    BMT_update_record_t *rec;
    void *out_sec;
    int32_t ret, wait_ret, cnt;

    ret = 0;
    out_sec = kmap(bio_c->bio->bi_io_vec[0].bv_page);

    memset(out_sec, 0, PAGE_SIZE);  // clear page content

    LFSM_ASSERT(out_sec);

    memset(out_sec, 0, PAGE_SIZE);  // clear page content

    rec = (BMT_update_record_t *) (out_sec);
    rec->signature = log_marker;
    switch (log_marker) {
    case UL_GC_MARK:
        rec->gc_record.total_disks = td->cn_ssds;
        _UL_GCentry_set_active(td, rec);
        break;
    case UL_HPEU_ERASE_MARK:
        rec->id_hpeu = log_eu->id_hpeu;
        break;
    case UL_NORMAL_MARK:
        rec->n_record.birthday = HPEU_get_birthday(&td->hpeutab, log_eu);
        tf_printk("DD: EU log %x {%llu, %d, %d, %llx}\n",
                  rec->signature, log_eu->start_pos, temperature,
                  log_eu->id_hpeu, rec->n_record.birthday);
    case UL_ERASE_MARK:
        rec->n_record.eu_start = log_eu->start_pos;
        rec->n_record.temperature = temperature;
        rec->n_record.disk_num = log_eu->disk_num;
        rec->n_record.id_hpeu = log_eu->id_hpeu;
        break;
    default:
        ret = -EINVAL;
        break;
    }
    //dprintk("BMT_update_log[%llu]: eu_start %llu active %d temp %d log_marker %d\n",
    //             bio_c->bio->bi_iter.bi_sector,rec->n_record,rec->eu_active, rec->temperature, log_marker);

    kunmap(bio_c->bio->bi_io_vec[0].bv_page);
    bio_c->io_queue_fin = 0;

    LFSM_ASSERT(bio_c);
    LFSM_ASSERT(bio_c->bio);

    //LOGINFO("log write lpn %lld cn_fpage %d\n",
    //        (sector64)bio_c->bio->bi_iter.bi_sector/SECTORS_PER_FLASH_PAGE, bio_c->bio->bi_iter.bi_size/FLASH_PAGE_SIZE);
    if ((ret = my_make_request(bio_c->bio)) != 0) {
        bio_c->io_queue_fin = -ENOEXEC;
        return ret;
    }

    if (isPipeline) {
        return 0;
    }
    //lfsm_wait_event_interruptible(bio_c->io_queue, bio_c->io_queue_fin != 0);
    cnt = 0;
    while (true) {
        cnt++;
        wait_ret = wait_event_interruptible_timeout(bio_c->io_queue,
                                                    bio_c->io_queue_fin != 0,
                                                    LFSM_WAIT_EVEN_TIMEOUT);
        if (wait_ret <= 0) {
            LOGWARN("UpdateLog write IO no response %llu after seconds %d\n",
                    update_log_pbn, LFSM_WAIT_EVEN_TIMEOUT_SEC * cnt);
        } else {
            break;
        }
    }

    if (bio_c->io_queue_fin <= 0) {
        LOGERR("in Update logging at PBN %llu\n", update_log_pbn);
        ret = -EIO;
    }

    bio_c->io_queue_fin = 0;

    return ret;
}

/* BMT_update_end_bio  HWINT simply set the bio container to idle state to free it
 * @[in]bio: bio
 * @[in]err:
 * @[return] void
 */
static void end_bio_updatelog(struct bio *bio)
{
    struct bio_container *bioc;
    lfsm_dev_t *td;

    bioc = bio->bi_private;
    bioc->status = bio->bi_status;
    if (bio->bi_status) {
        LOGERR("Error %d in bmt_update_io", bio->bi_status);
    }

    td = bioc->lfsm_dev;
    LFSM_ASSERT(td);

    LFSM_ASSERT2(bioc->org_bio == NULL,
                 "updatelog bioc(%p) should not have org_bio\n", bioc);
    // It can be removed if stable. Tifa 2014/1/15

    if (bio->bi_status != BLK_STS_OK) {
        bioc->io_queue_fin = blk_status_to_errno(bio->bi_status);
    } else {
        bioc->io_queue_fin = 1;
    }

    wake_up_interruptible(&bioc->io_queue);
    /* cannot aquire lock in the INT handler here or deadlock */
    /* entry will be move the datalog_free_list in move_from_active_to_free */

    return;
}

static void end_bio_async_updatelog(struct bio *bio)
{
    struct bio_container *bioc;
    struct EUProperty *eu;

    bioc = bio->bi_private;
    if (IS_ERR_OR_NULL(eu = eu_find(bioc->source_pbn))) {   // eu start_pos access by UpdateLog_logging_direct
        bioc->io_queue_fin = -EINVAL;
        lfsm_write_pause();
        goto l_end;
    }
    bioc->status = bio->bi_status;

    atomic_dec(&eu->cn_pinned_pages);
    if (bio->bi_status) {
        bioc->io_queue_fin = blk_status_to_errno(bio->bi_status);
        if (atomic_read(&eu->cn_pinned_pages) == 0
            && atomic_read(&eu->erase_state) != LOG_OK) {
            atomic_set(&eu->erase_state, DIRTY);
            lfsm_write_pause();
        }
    } else {
        bioc->io_queue_fin = 1;
        atomic_set(&eu->erase_state, LOG_OK);
    }

  l_end:
    wake_up_interruptible(&bioc->io_queue);
    wake_up_interruptible(&lfsmdev.ersmgr.wq_item);
}

/*
 * Description: initial a bioc and call
 */
static int32_t UpdateLog_logging_direct(lfsm_dev_t * td,
                                        struct EUProperty *ul_head,
                                        struct EUProperty *log_eu,
                                        int32_t temperature, int32_t log_marker,
                                        struct bio_container *pret_bioc)
{
    struct EUProperty *cur_eu;
    struct bio_container *update_log_bio_c;
    sector64 ul_pos;
    int32_t ret;
    bool isPipeline;

    isPipeline = true;
    update_log_bio_c = NULL;
    cur_eu = ul_head;
    ret = 0;
    LFSM_ASSERT(ul_head);

    ul_pos = cur_eu->start_pos + cur_eu->log_frontier;
    if (NULL == (pret_bioc)) {
        isPipeline = false;
        update_log_bio_c =
            bioc_get(td, NULL, 0, 0, 0, DONTCARE_TYPE, REQ_OP_WRITE, 1);
        LFSM_ASSERT(update_log_bio_c);
        init_bio_container(update_log_bio_c, NULL, NULL, end_bio_updatelog,
                           REQ_OP_WRITE, ul_pos, MEM_PER_HARD_PAGE);
    } else {
        update_log_bio_c = pret_bioc;
        init_bio_container(update_log_bio_c, NULL, NULL,
                           end_bio_async_updatelog, REQ_OP_WRITE, ul_pos,
                           MEM_PER_HARD_PAGE);
        update_log_bio_c->source_pbn = log_eu->start_pos;   // tifa: eu start_pos
        atomic_inc(&log_eu->cn_pinned_pages);
    }

    ret = UpdateLog_diskwrite(td, log_eu, temperature, log_marker, ul_pos,
                              update_log_bio_c, isPipeline);

    if (!isPipeline) {
        put_bio_container(td, update_log_bio_c);
    } else {
        if (ret != 0) {
            atomic_dec(&log_eu->cn_pinned_pages);
        }
    }
    cur_eu->log_frontier += SECTORS_PER_HARD_PAGE;

    return ret;
}

static int32_t UpdateLog_logging_A(lfsm_dev_t * td, struct EUProperty *log_eu,
                                   int32_t temperature, int32_t log_marker,
                                   struct bio_container **ar_pbioc)
{
    SUpdateLog_t *pUL;
    sector64 ret, ret1, ret2;

    //printk("%s UL frontier= %d\n",__FUNCTION__,subdev->cur_update_log_eu->log_frontier);
    if (lfsm_readonly == 1) {
        return 0;
    }

    pUL = &td->UpdateLog;
    ret = 0;
    ret1 = 0;
    ret2 = 0;
    LFSM_MUTEX_LOCK(&pUL->lock);

    if ((pUL->eu_main->log_frontier +
         CN_PAGES_BEFORE_UL_EU_END * SECTORS_PER_HARD_PAGE) >=
        SECTORS_PER_SEU) {
        if (UpdateLog_try_reset(pUL, td, false) >= 0) {
            rmutex_unlock_hook(&gc[log_eu->disk_num]->whole_lock,
                               UpdateLog_hook, td);
            LOGINFO("%s end of UL change EU and frontier= %d\n", __FUNCTION__,
                    pUL->eu_main->log_frontier);
        }
    }

    if ((pUL->eu_main->log_frontier + SECTORS_PER_HARD_PAGE) >= SECTORS_PER_SEU) {  // last page, force commit
        LOGERR
            ("reach the last page in EU, force bmt commit and updatelog logging");
        UpdateLog_try_reset(pUL, td, true);
        rmutex_unlock_hook(&gc[log_eu->disk_num]->whole_lock, UpdateLog_hook,
                           td);
        LOGERR("%s end of UL change EU and frontier= %d\n", __FUNCTION__,
               pUL->eu_main->log_frontier);
    }
    ret1 = UpdateLog_logging_direct(td, pUL->eu_main, log_eu, temperature,
                                    log_marker,
                                    (ar_pbioc == NULL) ? NULL : ar_pbioc[0]);
    ret2 =
        UpdateLog_logging_direct(td, pUL->eu_backup, log_eu, temperature,
                                 log_marker,
                                 (ar_pbioc == NULL) ? NULL : ar_pbioc[1]);
    LFSM_MUTEX_UNLOCK(&pUL->lock);

    if (ret1 < 0 && ret2 < 0) {
        ret = -EIO;
    } else {
        ret = 0;
    }
    return ret;
}

/*
 * Description : write update log with sync way
 */
int32_t UpdateLog_logging(lfsm_dev_t * td, struct EUProperty *log_eu,
                          int32_t temperature, int32_t log_marker)
{
    return UpdateLog_logging_A(td, log_eu, temperature, log_marker, NULL);
}

/*
 * Description: write update log async way, required caller pass bio continer so
 *              that we won't pause thread here
 */
int32_t UpdateLog_logging_async(lfsm_dev_t * td, struct EUProperty *log_eu,
                                int32_t temperature, int32_t log_marker,
                                struct bio_container **ar_pbioc)
{
    return UpdateLog_logging_A(td, log_eu, temperature, log_marker, ar_pbioc);
}

// the function is supported running in foreground
int32_t UpdateLog_logging_if_undone(lfsm_dev_t * td, struct EUProperty *eu_ret)
{
    int32_t ret;

    ret = 0;
    //if (atomic_read(&eu_ret->erase_state)==DIRTY || atomic_read(&eu_ret->erase_state)==EU_PICK)
    if (atomic_read(&eu_ret->erase_state) < LOG_SUBMIT &&
        atomic_read(&eu_ret->erase_state) >= DIRTY) {
        ret = UpdateLog_logging(td, eu_ret, eu_ret->temper, UL_ERASE_MARK);
        //printk("state is %d \n",atomic_read(&eu_ret->erase_state));
        if (ret == 0) {
            atomic_set(&eu_ret->erase_state, LOG_OK);
        } else {
            lfsm_write_pause();
            return ret;
        }
    } else {                    // wait background and check, don't change any param.
        lfsm_wait_event_interruptible(lfsmdev.ersmgr.wq_item,
                                      atomic_read(&eu_ret->erase_state) !=
                                      LOG_SUBMIT);
        if (atomic_read(&eu_ret->erase_state) == DIRTY) {   //implies log_fail
            return -EIO;
        }
    }

    return ret;
}

/*
 * Description: Write log to new eu of UpdateLog
 * NOTE: why we write record in ul_rec to new EU
 *       and then copy an empty from old to new EU
 */
static int32_t UpadteLog_new_eu_init(SUpdateLog_t * pUL,
                                     struct EUProperty *eu_old_main,
                                     lfsm_dev_t * td)
{
    int32_t cn_copied_page, ret;
    bool ret_w1, ret_w2;

    ret_w1 =
        devio_write_pages(pUL->eu_main->start_pos, (int8_t *) pUL->ul_rec, NULL,
                          1);
    ret_w2 =
        devio_write_pages(pUL->eu_backup->start_pos, (int8_t *) pUL->ul_rec,
                          NULL, 1);

    if (ret_w1 == false && ret_w2 == false) {
        LOGINFO("write_page fail, ret1 %d ret2 %d\n", ret_w1, ret_w2);
        return -EIO;
    }

    pUL->eu_main->log_frontier += SECTORS_PER_FLASH_PAGE;
    pUL->eu_backup->log_frontier += SECTORS_PER_FLASH_PAGE;

    LOGINFO
        ("copy page from pUL->ul_frontier %d to eu_old_main->log_frontier %d\n",
         pUL->ul_frontier, eu_old_main->log_frontier);

    //copy from old eu to new eu
    cn_copied_page = devio_copy_fpage(eu_old_main->start_pos + pUL->ul_frontier,
                                      pUL->eu_main->start_pos +
                                      pUL->eu_main->log_frontier,
                                      pUL->eu_backup->start_pos +
                                      pUL->eu_backup->log_frontier,
                                      (eu_old_main->log_frontier -
                                       pUL->
                                       ul_frontier) >>
                                      SECTORS_PER_FLASH_PAGE_ORDER);
    if (cn_copied_page < 0) {
        ret = -EACCES;
    }

    pUL->eu_main->log_frontier +=
        cn_copied_page << SECTORS_PER_FLASH_PAGE_ORDER;
    pUL->eu_backup->log_frontier +=
        cn_copied_page << SECTORS_PER_FLASH_PAGE_ORDER;
    return 0;
}

/*
 *  Description:
 *  return value: true : uncommitable
 *                false: commitable
 */
static bool _UL_chk_bmt_committable(BMT_update_record_t * rec, lfsm_dev_t * td)
{
    // check all update_ppq is exceed the marker
    seuinfo_t *peuinfo;
    int32_t i, j, frontier_in_hpeu, cn_total_pages, cn_unfinished_pages;

    cn_total_pages = (DATA_FPAGE_IN_SEU * td->hpeutab.cn_disk);
    for (j = 0; j < TEMP_LAYER_NUM; j++) {
        frontier_in_hpeu = 0;
        for (i = 0; i < td->cn_ssds; i++) {
            peuinfo = gculrec_get_euinfo(&rec->gc_record, i, j);
            frontier_in_hpeu += peuinfo->frontier;
//            LOGINFO("disk=%d , temp=%d , frontier=%d\n",i,j,peuinfo->frontier);
            if (i % td->hpeutab.cn_disk == (td->hpeutab.cn_disk - 1)) { // end of a temperature in a group
                cn_unfinished_pages =
                    atomic_read(&
                                (HPEU_GetTabItem
                                 (&td->hpeutab,
                                  peuinfo->id_hpeu)->cn_unfinish_fpage));

//                LOGINFO("unfinsh write %d , all_frontier_sum %d, all_pages in hpeu = %u\n",
//                    cn_unfinished_pages,frontier_in_hpeu,cn_total_pages );

                if ((cn_total_pages - cn_unfinished_pages)  // finished page(updated pages)
                    < (frontier_in_hpeu >> SECTORS_PER_FLASH_PAGE_ORDER)) { // allocated page
                    LOGINFO("return false\n");
                    return true;
                }

                frontier_in_hpeu = 0;
            }
        }
    }

    LOGINFO("return true\n");
    return false;
}

/*
 * Description: Update log full, reset update log
 *              commit bmt table to disk
 */
int32_t UpdateLog_try_reset(SUpdateLog_t * pUL, lfsm_dev_t * td,
                            bool force_reset)
{
    struct EUProperty *eu_old_main, *eu_old_bkup;
    int32_t ret;

    LOGINFO("pUL->eu_main log froint %d, fro %d\n", pUL->eu_main->log_frontier,
            pUL->ul_frontier);
    if (!force_reset) {
        if (pUL->ul_frontier < 0) {
            memset(pUL->ul_rec, 0, FLASH_PAGE_SIZE);
            pUL->ul_frontier = pUL->eu_main->log_frontier;
            pUL->ul_rec->signature = UL_GC_MARK;
            pUL->ul_rec->gc_record.total_disks = td->cn_ssds;
            _UL_GCentry_set_active(td, pUL->ul_rec);
            return -ELOOP;
        }

        if (_UL_chk_bmt_committable(pUL->ul_rec, &lfsmdev)) {
            return -EAGAIN;
        }
    }
    // sucess update_log reset

    pUL->cn_cycle++;
    update_ondisk_BMT(td, td->ltop_bmt, true);
    ECT_commit_all(td, false);
    HPEU_save(&td->hpeutab);
    pUL->threshold = UL_THRE_INIT;
    eu_old_main = pUL->eu_main;
    eu_old_bkup = pUL->eu_backup;

    SNextPrepare_get_new(&pUL->next, &pUL->eu_main, &pUL->eu_backup,
                         EU_UPDATELOG);
    ret = UpadteLog_new_eu_init(pUL, eu_old_main, td);

    dedicated_map_logging(td);
    SNextPrepare_put_old(&pUL->next, eu_old_main, eu_old_bkup);
    pUL->ul_frontier = -1;
    return ret;
}

/*
 * Description: reset log in foreground
 */
int32_t UpdateLog_reset_withlock(lfsm_dev_t * td)
{
    wait_queue_head_t wq;
    SUpdateLog_t *pUL;
    int32_t ret;

    init_waitqueue_head(&wq);
    pUL = &td->UpdateLog;

  l_again:

    LFSM_MUTEX_LOCK(&pUL->lock);

    if ((ret = UpdateLog_try_reset(pUL, td, false)) < 0) {
        LOGINFO("Wait again, ret= %d\n", ret);
        LFSM_MUTEX_UNLOCK(&pUL->lock);
        wait_event_interruptible_timeout(wq, 0, 50);
        goto l_again;
    }
    LFSM_MUTEX_UNLOCK(&pUL->lock);
    UpdateLog_hook(td);
    return ret;
}

static void _UL_threshold_next(SUpdateLog_t * pUL)
{
    pUL->threshold =
        ((pUL->eu_main->log_frontier * 100) >> SECTORS_PER_SEU_ORDER) +
        UL_THRE_INC;

    if (pUL->threshold >= 100) {
        pUL->threshold = 100;
    }
}

/*
 * Description: check the whether update log increase with 10%
 * return: 1 : increase with 10%
 *         0 : slow increase
 * NOTE: Not use now
 */
int32_t UpdateLog_check_threshold(SUpdateLog_t * pUL)
{
    int32_t commit;

    commit = 0;
    LFSM_MUTEX_LOCK(&pUL->lock);
    /* commit per 10% increase in update log */
    if (100 * (pUL->eu_main->log_frontier) >=
        (1 << (SECTORS_PER_SEU_ORDER)) * pUL->threshold) {
        commit = 1;
        _UL_threshold_next(pUL);
    }
    LFSM_MUTEX_UNLOCK(&pUL->lock);
    return commit;
}

/*
 * Description: rescue update log EU
 * return value: true : rescue OK
 *               false: rescue fail
 */
bool UpdateLog_rescue(lfsm_dev_t * td, SUpdateLog_t * pUL)
{
    return sysEU_rescue(td, &pUL->eu_main, &pUL->eu_backup, &pUL->next,
                        &pUL->lock);
}

/*
 * Description: compare gc record in main EU and backup EU
 * return value: true : same content
 *               false: different content
 */
static bool _UL_entry_cmp_GCRecord(lfsm_dev_t * td,
                                   BMT_update_record_t * rec_main,
                                   BMT_update_record_t * rec_backup)
{
    int32_t i, j;

    for (i = 0; i < td->cn_ssds; i++) {
        for (j = 0; j < TEMP_LAYER_NUM; j++) {
            if (memcmp(gculrec_get_euinfo(&rec_main->gc_record, i, j),
                       gculrec_get_euinfo(&rec_main->gc_record, i, j),
                       sizeof(seuinfo_t)) != 0) {
                return false;
            }
        }
    }

    return true;
}

/*
 * Description: compare update log record in main EU and backup EU
 * return value: true : same content
 *               false: different content
 */
static bool _UL_cmp_content(lfsm_dev_t * td, int8_t * data_main,
                            int8_t * data_backup)
{
    BMT_update_record_t *rec_main, *rec_backup;
    int32_t log_marker;
    bool ret;

    rec_main = (BMT_update_record_t *) data_main;
    rec_backup = (BMT_update_record_t *) data_backup;
    log_marker = rec_main->signature;
    ret = false;
    if (rec_main->signature != rec_backup->signature) {
        LOGINFO(" Update log signarure is different");
        return ret;
    }

    switch (log_marker) {
    case UL_GC_MARK:
        ret = _UL_entry_cmp_GCRecord(td, rec_main, rec_backup);
        break;
    case UL_HPEU_ERASE_MARK:
        if (rec_main->id_hpeu != rec_backup->id_hpeu) {
            ret = false;
        } else {
            ret = true;
        }
        break;
    case UL_NORMAL_MARK:
        if (rec_main->n_record.birthday == rec_backup->n_record.birthday) {
            ret = true;
        }
        break;
    case UL_ERASE_MARK:
        rec_main->n_record.birthday = rec_backup->n_record.birthday;
        if (memcmp
            (&rec_main->n_record, &rec_backup->n_record,
             sizeof(UL_record_t)) == 0) {
            ret = true;
        } else {
            ret = false;
        }

        //printk(" UL_ERASE_MARK eu_start %llu, temper %d, disk_num %d , id_hpeu %d\n",
        //                rec->n_record.eu_start,rec->n_record.temperature,
        //                rec->n_record.disk_num,rec->n_record.id_hpeu);
        //hexdump((char*)&rec_main->n_record,sizeof(UL_record_t));
        //hexdump((char*)&rec_backup->n_record,sizeof(UL_record_t));
        break;
    default:
        LOGWARN(" UNKNOW MARKER\n");
        ret = false;
        break;
    }

    return ret;
}

static void _UL_dump_GC_MARK_entry(lfsm_dev_t * td, BMT_update_record_t * rec)
{
    seuinfo_t *peuinfo;
    int32_t i, j;

    for (i = 0; i < td->cn_ssds; i++) {
        for (j = 0; j < TEMP_LAYER_NUM; j++) {
            peuinfo = gculrec_get_euinfo(&rec->gc_record, i, j);
            LOGINFO(" eu_start:%lld, frontier:%d id_hpeu:%d birth %llu \n",
                    peuinfo->eu_start, peuinfo->frontier, peuinfo->id_hpeu,
                    peuinfo->birthday);
        }
    }
}

static void _UL_dump_data(lfsm_dev_t * td, int8_t * data)
{
    BMT_update_record_t *rec;
    int32_t log_marker;

    rec = (BMT_update_record_t *) data;
    log_marker = rec->signature;
    switch (log_marker) {
    case UL_GC_MARK:
        LOGINFO("GC_MARKER cn_ssd=%d,\n", rec->gc_record.total_disks);
        _UL_dump_GC_MARK_entry(td, rec);
        break;
    case UL_HPEU_ERASE_MARK:
        LOGINFO(" UL_HPEU_ERASE_MARK id_hpeu=%d\n", rec->id_hpeu);
        break;
    case UL_NORMAL_MARK:
        LOGINFO(" UL_NORMAL_MARK birthday%llu \n", rec->n_record.birthday);
        //tf_printk("DD: EU log {%x, %llu,%d,%d}\n",rec->signature,log_eu->start_pos,temperature,log_eu->id_hpeu);
        //rec->n_record.birthday = HPEU_get_birthday(&td->hpeutab,log_eu);
    case UL_ERASE_MARK:
        LOGINFO
            (" UL_ERASE_MARK eu_start %llu, temper %d, disk_num %d , id_hpeu %d\n",
             rec->n_record.eu_start, rec->n_record.temperature,
             rec->n_record.disk_num, rec->n_record.id_hpeu);
        break;
    default:
        LOGWARN(" UNKNOW MARKER\n");
        break;
    }
}

/*
 * Description: compare and dumpe single record in update log
 * Parameter:   data_main  : record in main EU
 *              data_backup: record in backup EU
 * return:
 */
bool UpdateLog_dump(lfsm_dev_t * td, int8_t * data_main, int8_t * data_backup)
{
    if (_UL_cmp_content(td, data_main, data_backup)) {
        LOGINFO("content is the same\n");
        return true;
    }

    LOGINFO("UpdateLog Main Data\n");
    _UL_dump_data(td, data_main);
    LOGINFO("UpdateLog Backup Data\n");
    _UL_dump_data(td, data_backup);
    return false;
}

/*
 * Description: alloc extra EU to use when current EU full
 */
int32_t UpdateLog_eunext_init(SUpdateLog_t * pUL)
{
    return SNextPrepare_alloc(&pUL->next, EU_UPDATELOG);
}

/*
 * Description: assign EU according dedicated map
 */
int32_t UpdateLog_eu_set(SUpdateLog_t * pUL, lfsm_dev_t * td,
                         sddmap_disk_hder_t * pheader)
{
    struct EUProperty *eu;

    if (IS_ERR_OR_NULL(eu = eu_find(pheader->update_log_head))) {
        LOGERR("fail find main EU of update log in dedicated map\n");
        return -ENOEXEC;
    }
    pUL->eu_main = eu;

    if (IS_ERR_OR_NULL(eu = eu_find(pheader->update_log_backup))) {
        LOGERR("fail find backup EU of update log in dedicated map\n");
        return -ENOEXEC;
    }
    pUL->eu_backup = eu;

    // check for ulog's EU stat
    if (NULL ==
        (FreeList_delete_by_pos(td, pUL->eu_main->start_pos, EU_UPDATELOG))) {
        LOGERR("fail remove main EU of update log from freelist\n");
        return -ENOEXEC;
    }
    // check for ulog's EU stat
    if (NULL ==
        (FreeList_delete_by_pos(td, pUL->eu_backup->start_pos, EU_UPDATELOG))) {
        LOGERR("fail remove backup EU of update log from freelist\n");
        return -ENOEXEC;
    }

    return 0;
}

/*
 * Description: initial EU in update log
 * parameter: fg_insert_old: true: put eu of update log to free list in foreground
 *                           false: free list in backgroup??
 *            ulfrontier : froniter in page unit
 * return: 0 : OK
 *         non 0: init fail
 */
int32_t UpdateLog_eu_init(SUpdateLog_t * pUL, int32_t ulfrontier,
                          bool fg_insert_old)
{                               /*s_comment changed prototype to accept index */
    struct HListGC *h;
    if (lfsm_readonly == 1) {
        UpdateLog_eunext_init(pUL);
        return 0;
    }

                                                                                        /** Use a random disk to pick up the next EU for update log **/
    if (fg_insert_old) {
        FreeList_insert(gc[pUL->eu_main->disk_num], pUL->eu_main);
    }

    h = gc[get_next_disk()];
    pUL->eu_main = FreeList_delete(h, EU_UPDATELOG, false);
    if (fg_insert_old) {
        FreeList_insert(gc[pUL->eu_backup->disk_num], pUL->eu_backup);
    }
    h = gc[get_next_disk()];
    pUL->eu_backup = FreeList_delete(h, EU_UPDATELOG, false);
    pUL->threshold = UL_THRE_INIT;
    //[CY]upfrontier is page based , need to transform to sector based.....
    pUL->eu_main->log_frontier = ulfrontier << SECTORS_PER_HARD_PAGE_ORDER;
    pUL->eu_backup->log_frontier = ulfrontier << SECTORS_PER_HARD_PAGE_ORDER;
    LOGINFO("update log start: %llu, %llu\n", pUL->eu_main->start_pos,
            pUL->eu_backup->start_pos);
    if (!fg_insert_old) {
        UpdateLog_eunext_init(pUL);
    }

    LOGINFO("BMT Update log initalizing..... DONE\n");
    return 0;
}

/*
 * Description: initial update log data structure
 * return: true : fail
 *         false : OK
 */
bool UpdateLog_init(SUpdateLog_t * pUL)
{
    /*
     * check record size, size of a record cannot over 4K
     */
    if ((sizeof(BMT_update_record_t) +
         sizeof(seuinfo_t) * lfsmdev.cn_ssds * TEMP_LAYER_NUM) >
        FLASH_PAGE_SIZE) {
        LOGERR("UpdateLog init fail record size larger than 4K\n");
        return true;
    }

    mutex_init(&pUL->lock);
    pUL->cn_cycle = 0;
    pUL->ul_frontier = -1;
    pUL->ul_rec = (BMT_update_record_t *)
        track_kmalloc(FLASH_PAGE_SIZE, GFP_KERNEL, M_OTHER);
    return false;
}

void UpdateLog_destroy(SUpdateLog_t * pUL)
{
    if (&pUL->lock) {
        mutex_destroy(&pUL->lock);
    }

    if (!IS_ERR_OR_NULL(pUL->eu_main)) {
        LOGINFO("updatelog froint %d \n", pUL->eu_main->log_frontier);
    }

    if (pUL->ul_rec) {
        track_kfree(pUL->ul_rec, FLASH_PAGE_SIZE, M_OTHER);
    }
}
