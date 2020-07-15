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
#include "common/mem_manager.h"
#include "common/rmutex.h"
#include "io_manager/mqbiobox.h"
#include "io_manager/biocbh.h"
#include "nextprepare.h"
#include "diskgrp.h"
#include "system_metadata/Dedicated_map.h"
#include "lfsm_core.h"
#include "err_manager.h"
#include "erase_manager.h"
#include "devio.h"
#include "spare_bmt_ul.h"
#include "erase_count.h"
#include "io_manager/io_common.h"
#include "io_manager/io_write.h"

#ifdef THD_EXEC_STEP_MON
#include "lfsm_thread_monitor.h"
#endif

int32_t gcn_free_total;

static int32_t geu_count_all_free(void)
{
    int32_t i;

    for (i = 0; i < lfsmdev.cn_ssds; i++) {
        gcn_free_total += gc[i]->free_list_number;
    }

    return 0;
}

// a function to determine whether to enable pre-erase.
static bool ersmgr_should_enable(int32_t pre_erase_threshold)
{
    int32_t id_disk;
    bool ret;

    ret = false;
    if (lfsm_readonly == 1) {
        return false;
    }

    for (id_disk = 0; id_disk < lfsmdev.cn_ssds; id_disk++) {
#if TRIM_ALL_FREE == 1
        if (atomic_read(&gc[id_disk]->cn_clean_eu) <
            (gc[id_disk]->free_list_number - 1))
#else
        if ((atomic_read(&gc[id_disk]->cn_clean_eu) <= pre_erase_threshold) &&
            (atomic_read(&gc[id_disk]->cn_clean_eu) <
             (gc[id_disk]->free_list_number - 1)))
#endif
        {
            ret = true;
            break;
        }
    }

    return ret;
}

static int32_t ersmgr_eu_insertfree(sersmgr_item_t * ar_item, int32_t cn_item)
{
    struct HListGC *h;
    int32_t i;

    for (i = 0; i < cn_item; i++) {
        h = gc[ar_item[i].peu->disk_num];
        LFSM_RMUTEX_LOCK(&h->whole_lock);
        if (FreeList_insert_nodirty(h, ar_item[i].peu) < 0) {
            LOGERR("fail insert disk %d eu %p\n", ar_item[i].peu->disk_num,
                   ar_item[i].peu);
        }
        LFSM_RMUTEX_UNLOCK(&h->whole_lock);
    }

    return 0;
}

static int32_t ersmgr_eu_pick(sersmgr_item_t * ar_item, lfsm_dev_t * td)
{
    struct HListGC *h;
    struct EUProperty *eu_cur, *eu_next;
    int32_t id_disk, cn_pick_per_disk;
    int32_t cn_total_pick;

    cn_total_pick = 0;
    //scan whole disk freelist, mark 10 dirty eu per disk as erasing_state
    for (id_disk = 0; id_disk < td->cn_ssds; id_disk++) {
        cn_pick_per_disk = 0;
        h = gc[id_disk];
        if (td->ar_pgroup[PGROUP_ID(id_disk)].state != epg_good) {
            continue;
        }
#if TRIM_ALL_FREE==0
        if (atomic_read(&h->cn_clean_eu) > ERSMGR_MAX_ERASE_NUM) {
            continue;
        }
#endif
        LFSM_RMUTEX_LOCK(&h->whole_lock);
        list_for_each_entry_safe(eu_cur, eu_next, &h->free_list, list) {
            //printk("cn=%d, erase_state=%d, ar_peu[%d]=%p\n", cn, atomic_read(&eu_cur->erase_state),
            //        cn+i*MAX_ERASE_NUM, perase_item[cn+i*LFSM_DISK_NUM].peu);
            if (atomic_read(&eu_cur->erase_state) != DIRTY) {
                continue;
            }

            if (list_is_last(&eu_cur->list, &h->free_list)) {   // eu is last in list
                break;
            }

            list_del_init(&eu_cur->list);
            h->free_list_number--;
            atomic_set(&eu_cur->erase_state, EU_PICK);
            ar_item[cn_total_pick].peu = eu_cur;
            cn_total_pick++;
            cn_pick_per_disk++;
            if (cn_pick_per_disk >= ERSMGR_MAX_ERASE_NUM) {
                break;
            }
        }
        LFSM_RMUTEX_UNLOCK(&h->whole_lock);
    }

    return cn_total_pick;
}

static int32_t _wait_UL_logging_async_done(lfsm_dev_t * td,
                                           struct bio_container *pbioc)
{
    int32_t ret;

    if (IS_ERR_OR_NULL(pbioc)) {
        return 0;
    }

    ret = 0;
    lfsm_wait_event_interruptible(pbioc->io_queue,
                                  kthread_should_stop()
                                  || (pbioc->io_queue_fin != 0));

    if (pbioc->io_queue_fin <= 0) {
        LOGERR("in Update logging at bioc %p sector %llu (%d)\n",
               pbioc, (sector64) pbioc->bio->bi_iter.bi_sector,
               pbioc->io_queue_fin);
        ret = pbioc->io_queue_fin;
    }

    pbioc->io_queue_fin = 0;
    put_bio_container(td, pbioc);

    return ret;
}

// ar_item -> array of item to indicate eus to be logging and erase later
// cn_item -> cn_item to logging
static int32_t ersmgr_updatelog_logging(lfsm_dev_t * td,
                                        sersmgr_item_t * ar_item,
                                        int32_t cn_item)
{
    struct EUProperty *peu;
    int32_t i, ret, ret1;
    bool fg_inlock;

    ret1 = 0;
    fg_inlock = false;

    //do aysnc updatelog
    for (i = 0; i < cn_item; i++) {
        peu = ar_item[i].peu;
        if (atomic_read(&peu->erase_state) == EU_PICK) {
            ar_item[i].ar_pbioc[0] =
                bioc_get(td, NULL, 0, 0, 0, DONTCARE_TYPE, WRITE, 0);
            ar_item[i].ar_pbioc[1] =
                bioc_get(td, NULL, 0, 0, 0, DONTCARE_TYPE, WRITE, 0);
        }
    }

    for (i = 0; i < cn_item; i++) {
        peu = ar_item[i].peu;
        if (!fg_inlock) {
            LFSM_RMUTEX_LOCK(&gc[peu->disk_num]->whole_lock);
        }

        if (atomic_read(&peu->erase_state) == EU_PICK) {
            atomic_set(&peu->cn_pinned_pages, 0);
            atomic_set(&peu->erase_state, LOG_SUBMIT);
            ret =
                UpdateLog_logging_async(td, peu, 0, UL_ERASE_MARK,
                                        ar_item[i].ar_pbioc);
            if (ret != 0) {
                atomic_set(&peu->erase_state, DIRTY);
                lfsm_write_pause();
            }
        }

        if (((i + 1) == cn_item)
            || (ar_item[i + 1].peu->disk_num != peu->disk_num)) {
            fg_inlock = false;
            LFSM_RMUTEX_UNLOCK(&gc[peu->disk_num]->whole_lock);
        } else {
            fg_inlock = true;
        }
    }

    //do aysnc updatelog bh
    for (i = 0; i < cn_item; i++) {
        peu = ar_item[i].peu;
        ret = _wait_UL_logging_async_done(td, ar_item[i].ar_pbioc[0]);
        ret1 = _wait_UL_logging_async_done(td, ar_item[i].ar_pbioc[1]);
    }

    return 0;
}

// ar_item -> array of item to indicate eus to be erase
// cn_item -> cn_item to erase
static int32_t ersmgr_eu_erase(lfsm_dev_t * td, sersmgr_item_t * ar_item,
                               int32_t cn_item)
{
    struct EUProperty *peu;
    int32_t i, ret;

    //do erase aynsc
    for (i = 0; i < cn_item; i++) {
        peu = ar_item[i].peu;
        init_devio_cb(&ar_item[i].cb);
        if (atomic_read(&peu->erase_state) == LOG_OK) {
            atomic_set(&peu->erase_state, ERASE_SUBMIT);

            if ((ret =
                 devio_erase_async(peu->start_pos, NUM_EUS_PER_SUPER_EU,
                                   &ar_item[i].cb)) < 0) {
                atomic_set(&peu->erase_state, ERASE_SUBMIT_FAIL);
            }
        }
    }

    for (i = 0; i < cn_item; i++) {
        peu = ar_item[i].peu;
        if ((ret = devio_erase_async_bh(&ar_item[i].cb)) < 0) {
            diskgrp_logerr(&grp_ssds, peu->disk_num);
            atomic_set(&peu->erase_state, ERASE_SUBMIT_FAIL);
            continue;
        }

        atomic_set(&peu->erase_state, ERASE_DONE);
        diskgrp_cleanlog(&grp_ssds, peu->disk_num);
        if (gcn_free_total > 0) {
            gcn_free_total--;
        } else {
            ECT_erase_count_inc(peu);
        }

        atomic_inc(&gc[peu->disk_num]->cn_clean_eu);
        cn_erase_back++;
        //printk("eu_state=%d\n",atomic_read(&peu->erase_state));
        //printk("clean eu:%d\n", atomic_read(&gc[peu->disk_num]->cn_clean_eu));
    }

    return 0;
}

int32_t ersmgr_pre_erase(lfsm_dev_t * td)
{
    sersmgr_item_t *ar_item;
    int32_t ret, max_pick, cn_pick;

    max_pick = ERSMGR_MAX_ERASE_NUM * td->cn_ssds;
    ret = -ENOMEM;

    if (NULL ==
        (ar_item =
         track_kmalloc(sizeof(sersmgr_item_t) * max_pick, GFP_KERNEL,
                       M_OTHER))) {
        return ret;
    }

    memset(ar_item, 0, sizeof(sersmgr_item_t) * max_pick);

    cn_pick = ersmgr_eu_pick(ar_item, td);
    //TODO: we log first and do earse, how we handle it if crash after logging before erase
    if (ersmgr_updatelog_logging(td, ar_item, cn_pick) != 0) {
        goto l_end;
    }

    if (ersmgr_eu_erase(td, ar_item, cn_pick) != 0) {
        goto l_end;
    }

    ret = 0;

  l_end:
    ersmgr_eu_insertfree(ar_item, cn_pick);
    if (ar_item) {
        track_kfree(ar_item, sizeof(sersmgr_item_t) * max_pick, M_OTHER);
    }

    return ret;
}

int32_t ersmgr_result_handler(lfsm_dev_t * td, int32_t err,
                              struct EUProperty *eu)
{
    struct bad_block_info bad_blk_info;
    int32_t ret;

    if (err == 0) {
        goto l_end;
    }

    if ((err == -ENOMEM) || (err == -ENODEV) || (err == -EINVAL)) {
        return err;
    }
    // below func return -EIO, or the number of fail items
    if ((ret =
         special_read_enquiry(eu->start_pos, 1, 0, 1, &bad_blk_info)) == 0) {
        goto l_end;
    } else if (ret < 0) {       // -EIO
        LOGERR("Erase failure for %llu special query return value %d\n ",
               eu->start_pos, ret);
        diskgrp_logerr(&grp_ssds, eu->disk_num);
        return ret;             // -EIO or -ENODEV
    } else if (ret > 0) {
        //Erase Failure, mark as bad
        LOGERR("Erase Failure for %llu dropping\n ", eu->start_pos);
        drop_EU(((eu->start_pos - eu_log_start) >> (SECTORS_PER_SEU_ORDER)));
        return -EIO;
    } else {
        return -ENOEXEC;
    }

  l_end:
    ECT_erase_count_inc(eu);
    return 0;
}

void ersmgr_wakeup_worker(ersmgr_t * pemgr)
{
    wake_up_interruptible(&pemgr->wq);
}

static int32_t ersmgr_process(void *arg)
{
    lfsm_dev_t *td;
    ersmgr_t *pemgr;
    int32_t cn_fail;

    td = arg;
    pemgr = &td->ersmgr;
    cn_fail = 0;

    current->flags |= PF_MEMALLOC;
    while (true) {
        wait_event_interruptible(pemgr->wq, kthread_should_stop() ||
                                 ersmgr_should_enable(ERSMGR_ENABLE_THRESHOLD));
        if (kthread_should_stop()) {
            break;
        }

        cn_fail =
            (atomic_read(&td->dev_stat) == LFSM_STAT_REJECT) ? cn_fail + 1 : 0;
        if (cn_fail == 0) {
            ersmgr_pre_erase(td);
        } else if (cn_fail > 10) {
            schedule_timeout_interruptible((cn_fail > 180) ? 10000 : 1000);
        }
    }
    return 0;
}

/*
 * Description: create lfsm_erase thread
 * return : true : fail
 *          false: create ok
 */
bool ersmgr_createthreads(ersmgr_t * pemgr, void *arg)
{

    pemgr->pIOthread = kthread_run(ersmgr_process, arg, "lfsm_erase");

    if (IS_ERR_OR_NULL(pemgr->pIOthread)) {
        ersmgr_destroy(pemgr);
        return true;
    }
#ifdef THD_EXEC_STEP_MON
    reg_thread_monitor(pemgr->pIOthread->pid, "lfsm_erase");
#endif

    geu_count_all_free();
    return false;
}

int32_t ersmgr_init(ersmgr_t * pemgr)
{
    init_waitqueue_head(&pemgr->wq);
    init_waitqueue_head(&pemgr->wq_item);

    return 0;
}

void ersmgr_destroy(ersmgr_t * pemgr)
{
#if LFSM_PREERASE == 1
    if (IS_ERR_OR_NULL(pemgr->pIOthread)) {
        return;
    }
    if (kthread_stop(pemgr->pIOthread) < 0) {
        LOGERR("can't stop ersmgr pIOthread\n");
    }
    pemgr->pIOthread = 0;
#endif
}
