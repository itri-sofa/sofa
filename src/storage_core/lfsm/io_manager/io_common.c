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
#include "../bmt.h"
#include "io_common.h"
#include "io_read.h"
#include "io_write.h"
#include "../bmt_ppq.h"
#include "../bmt_ppdm.h"
#include "../bmt_commit.h"
#include "../EU_access.h"
#include "../special_cmd.h"
#include "../GC.h"
#include "../err_manager.h"
#include "../autoflush.h"
#include "../batchread.h"
#include "../EU_create.h"
#include "../spare_bmt_ul.h"
#include "../conflict.h"
#include "../sysapi_linux_kernel.h"
#include "../bio_ctrl.h"
#include "../ioctl.h"
#include "../perf_monitor/thd_perf_mon.h"
#include "../perf_monitor/ioreq_pendstage_mon.h"
#include "../perf_monitor/calculate_iops.h"

int32_t cn_err_print = 0;

struct bio_container **gar_pbioc;
int32_t gidx_arbioc = 0;

static int32_t garbioc_search_extra_idx(lfsm_dev_t * td,
                                        struct bio_container *pbioc)
{
    int32_t i;

    for (i = td->freedata_bio_capacity; i < gidx_arbioc; i++) {
        if (pbioc == gar_pbioc[i]) {
            return i;
        }
    }

    return -1;
}

static int32_t garbioc_replace(lfsm_dev_t * td, int32_t idx)
{
    gidx_arbioc--;
    gar_pbioc[idx] = gar_pbioc[gidx_arbioc];
    gar_pbioc[gidx_arbioc] = NULL;

    return 0;
}

int32_t bioc_cn_fpage(struct bio_container *bioc)
{
    return (bioc->end_lbno - bioc->start_lbno + 1);
}

sector64 bioc_get_psn(struct bio_container *pbioc, int32_t idx_fpage)
{
    if (pbioc->par_bioext) {
        return pbioc->par_bioext->items[idx_fpage].pbn_dest;
    } else {
        return pbioc->dest_pbn;
    }
}

sector64 bioc_get_sourcepsn(struct bio_container *pbioc, int32_t idx_fpage)
{
    if (pbioc->par_bioext) {
        return pbioc->par_bioext->items[idx_fpage].pbn_source;
    } else {
        return pbioc->source_pbn;
    }
}

void bioc_resp_user_bio(struct bio *bio, int32_t err)
{
    struct bio_vec tmpvec;
    struct bio *bio_next;

    if (NULL == bio) {
        return;
    }
//    LFSM_ASSERT(bio->bi_iter.bi_size > 0);
    if (bio->bi_iter.bi_size == 0) {
        LOGWARN("zero size bio: addr %llu size=%d vcnt=%d\n",
                (sector64) bio->bi_iter.bi_sector, bio->bi_iter.bi_size,
                bio->bi_vcnt);
    }

    bio_next = bio->bi_next;
    while (bio_next) {
        bio->bi_vcnt--;
        tmpvec = bio->bi_io_vec[bio->bi_vcnt];
        bio->bi_io_vec[bio->bi_vcnt] = bio_next->bi_io_vec[0];
        bio_next->bi_io_vec[0] = tmpvec;
        //bio->bi_io_vec[bio->bi_vcnt].bv_page=0;
        //bio->bi_io_vec[bio->bi_vcnt].bv_offset=0;
        //bio->bi_io_vec[bio->bi_vcnt].bv_len=0;
        bio_next = bio_next->bi_next;
    }

    while (NULL != bio) {
        bio_next = bio->bi_next;
        if ((err < 0) && (err != -ENODEV)) {
//            dump_stack();
            cn_err_print++;
            if ((cn_err_print < 100) || (cn_err_print % 1000) == 0) {
                LOGERR("[%d] resp user bio %p (%llds, %uB) err=%d\n",
                       cn_err_print, bio, (sector64) bio->bi_iter.bi_sector,
                       bio->bi_iter.bi_size, err);
            }
        }

        bio->bi_next = 0;

        bio->bi_status = errno_to_blk_status(err);
        bio_endio(bio);
        bio = bio_next;
    }

    return;
}

static int32_t bioc_alloc_page(struct bio *bio, int32_t page_num,
                               int32_t bi_size, int32_t idx_start)
{
    struct page *bio_page;
    int32_t i, retry;

    for (i = 0; i < page_num; i++) {
        retry = 0;
        /* Grab a free page and free bio to hold the log record header */
        if (NULL == (bio_page = track_alloc_page(__GFP_ZERO))) {
            LOGERR
                ("Fail to allocate header_page in compose_bio i=%d (retry=%d)\n",
                 i, retry);
            return -ENOMEM;
        }

        bio->bi_io_vec[idx_start + i].bv_page = bio_page;
        bio->bi_io_vec[idx_start + i].bv_offset = 0;
        if (bi_size < PAGE_SIZE) {
            bio->bi_io_vec[idx_start + i].bv_len = bi_size;
        } else {
            bio->bi_io_vec[idx_start + i].bv_len = PAGE_SIZE;
        }

        bi_size -= PAGE_SIZE;
    }
    return 0;
}

void bioc_wait_conflict(struct bio_container *bio_c, lfsm_dev_t * td,
                        int32_t HasConflict)
{
    int32_t cnt, wait_ret;

    if (HasConflict) {
        cnt = 0;
        while (true) {
            cnt++;
            wait_ret = wait_event_interruptible_timeout(bio_c->io_queue,
                                                        atomic_read(&bio_c->
                                                                    conflict_pages)
                                                        == 0,
                                                        LFSM_WAIT_EVEN_TIMEOUT);
            if (wait_ret <= 0) {
                LOGWARN
                    ("bio wait conflict IO no response %lld after seconds %d\n",
                     (sector64) bio_c->bio->bi_iter.bi_sector,
                     LFSM_WAIT_EVEN_TIMEOUT_SEC * cnt);
            } else {
                break;
            }
        }
    }
}

bool bioc_bio_free(lfsm_dev_t * td, struct bio_container *pBioc)
{
    LFSM_ASSERT(pBioc->org_bio)
        free_composed_bio(pBioc->org_bio, MEM_PER_FLASH_PAGE);
    post_rw_check_waitlist(td, pBioc, 1);
    put_bio_container(td, pBioc);

    return true;
}

int32_t compose_bio_no_page(struct bio **biop, struct block_device *bdev,
                            bio_end_io_t * bi_end_io, void *bi_private,
                            int32_t bi_size, int32_t bi_vec_size)
{
    struct bio *bio;
    int32_t i;

    while (NULL == (bio = track_bio_alloc(GFP_ATOMIC, bi_vec_size))) {
        LOGERR("allocate header_bio fails in compose_bio\n");
        schedule();
    }

    bio->bi_iter.bi_sector = -1;    /* we do not know the dest_LBA yet */
    if (bdev) {
        bio_set_dev(bio, bdev);
    } else {
        bio->bi_disk = NULL;
    }
    bio->bi_vcnt = bi_vec_size;
    bio->bi_iter.bi_idx = 0;
    bio->bi_iter.bi_size = bi_size;
    bio->bi_end_io = bi_end_io;
    bio->bi_private = bi_private;

    for (i = 0; i < bi_vec_size; i++) {
        bio->bi_io_vec[i].bv_page = NULL;
        bio->bi_io_vec[i].bv_offset = 0;
        bio->bi_io_vec[i].bv_len = 0;
    }

    *biop = bio;

    return 0;
}

int32_t compose_bio(struct bio **biop, struct block_device *bdev,
                    bio_end_io_t * bi_end_io, void *bi_private, int32_t bi_size,
                    int32_t bi_vec_size)
{
    struct bio *bio;
    int32_t retry;

    retry = 0;
    if (NULL ==
        (bio = track_bio_alloc(__GFP_RECLAIM | __GFP_ZERO, bi_vec_size))) {
        LOGERR("Fail to allocate header_bio in compose_bio (retry=%d)\n",
               retry);
        return -1;
    }

    if (bioc_alloc_page(bio, bi_vec_size, bi_size, 0) < 0) {
        LOGERR("Fail to alloc bio page\n");
        return -1;
    }

    bio->bi_iter.bi_sector = -1;    /* we do not know the dest_LBA yet */
    if (bdev) {
        bio_set_dev(bio, bdev);
    } else {
        bio->bi_disk = NULL;
    }
    bio->bi_vcnt = bi_vec_size;
    bio->bi_iter.bi_idx = 0;
    bio->bi_iter.bi_size = bi_size;
    bio->bi_end_io = bi_end_io;
    bio->bi_private = bi_private;
    *biop = bio;

    return 0;
}

/* [IN]page_array: page array to be initialized
 * [IN]bio_c: bio container
 * [IN]td: SFTL device
 * [IN]org_bio: the very original bio
 * [IN]org_bi_sector: the original bi_sector
 * [IN]s_lbno: the starting lbno of interest
 * [IN]e_lbno: the end lbno of interest
 * [IN]write_type: 0, non-partial; 1, partial
 * */
static int32_t init_per_page_list_item(struct per_page_active_list_item
                                       **page_array,
                                       struct bio_container *bio_c,
                                       lfsm_dev_t * td, struct bio *org_bio,
                                       sector64 org_bi_sector, sector64 s_lbno,
                                       sector64 e_lbno, int32_t write_type,
                                       int32_t rw)
{
    int64_t i, local_start_lbno, local_end_lbno, arr_index;
    struct bio_vec *my_vec;

    LFSM_ASSERT(((write_type != TP_GC_IO) || (org_bi_sector >= 0)))
        LFSM_ASSERT(page_array != NULL);
    LFSM_ASSERT(e_lbno >= s_lbno);

    dprintk("s_lbno: %llu, e_lbno: %llu, write_type: %d, rw: %d, "
            "org_bio: %p, vec: %p\n", s_lbno, e_lbno, write_type, rw, org_bio,
            org_bio->bi_io_vec);

/* Initialize the per-page active list item */
    if (write_type == TP_NORMAL_IO) {   /*  If it is non-partial, map one to one */
        for (i = s_lbno; i <= e_lbno; i++) {
            arr_index = i - s_lbno;
            LFSM_ASSERT(page_array[arr_index] != NULL);

            my_vec = &(org_bio->bi_io_vec[arr_index * MEM_PER_FLASH_PAGE]);

            page_array[arr_index]->lbno = i;
            page_array[arr_index]->len = my_vec->bv_len;
            page_array[arr_index]->offset = my_vec->bv_offset;
            page_array[arr_index]->ppage = my_vec->bv_page;

            if (rw == WRITE) {
                page_array[i - s_lbno]->ready_bit = EPPLRB_READY2GO;
            } else {
                page_array[i - s_lbno]->ready_bit = EPPLRB_READ_DISK;
            }
        }
    } else if (write_type == TP_PARTIAL_IO) {   /* This means it is a partial one, see which one is partial set "ready_bit" accordingly */
/* Test which one is a partial write */

        local_start_lbno = s_lbno;
        local_end_lbno = e_lbno;

        if (s_lbno ==
            (org_bio->bi_iter.bi_sector >> SECTORS_PER_FLASH_PAGE_ORDER)) {
            if ((org_bio->bi_iter.bi_sector % SECTORS_PER_FLASH_PAGE) != 0) {
                /**Means 0th page is partial**/
                my_vec = &(org_bio->bi_io_vec[0]);
                page_array[0]->lbno = s_lbno;
                page_array[0]->len = my_vec->bv_len;
                page_array[0]->offset = my_vec->bv_offset;
                page_array[0]->ppage = my_vec->bv_page;
                page_array[0]->ready_bit = EPPLRB_READ_DISK;
                local_start_lbno = s_lbno + 1;  /*Skip this page in for loop */
            }
        }

        if (e_lbno == ((org_bio->bi_iter.bi_sector + bio_sectors(org_bio)) >>
                       SECTORS_PER_FLASH_PAGE_ORDER)) {
            if (((org_bio->bi_iter.bi_sector +
                  bio_sectors(org_bio)) % SECTORS_PER_FLASH_PAGE) != 0) {
                                                     /**Last page is partial**/
                arr_index = e_lbno - s_lbno;
                my_vec =
                    &(org_bio->bi_io_vec[(arr_index) * MEM_PER_FLASH_PAGE]);
                page_array[arr_index]->lbno = e_lbno;
                page_array[arr_index]->len = my_vec->bv_len;
                page_array[arr_index]->offset = my_vec->bv_offset;
                page_array[arr_index]->ready_bit = EPPLRB_READ_DISK;
                local_end_lbno = e_lbno - 1;  /**Skip Last page in for loop*/
            }
        }

        for (i = local_start_lbno; i <= local_end_lbno; i++) {
            LFSM_ASSERT(page_array[i - s_lbno] != NULL);
            arr_index = i - s_lbno;
            my_vec = &(org_bio->bi_io_vec[(arr_index) * MEM_PER_FLASH_PAGE]);
            page_array[arr_index]->lbno = i;
            page_array[arr_index]->len = my_vec->bv_len;
            page_array[arr_index]->offset = my_vec->bv_offset;
            page_array[arr_index]->ppage = my_vec->bv_page;

            if (rw == WRITE) {
                page_array[arr_index]->ready_bit = EPPLRB_READY2GO;
            } else {
                page_array[arr_index]->ready_bit = EPPLRB_READ_DISK;
            }
        }
    } else if (write_type == TP_GC_IO) {
        /**only has a single page**/
        for (i = s_lbno; i <= e_lbno; i++) {

            arr_index = i - s_lbno;

            LFSM_ASSERT(page_array[arr_index] != NULL);
            //      if (index>0) printk("\nINDEX = %d",index);

            page_array[arr_index]->lbno = i;
            page_array[arr_index]->len = 0;
            page_array[arr_index]->offset = PAGE_SIZE;
            page_array[arr_index]->ppage = NULL;
            page_array[arr_index]->ready_bit = EPPLRB_READ_DISK;
        }
    } else {
        LFSM_ASSERT(0);
    }

    return 0;
}

int32_t dump_bio_active_list(lfsm_dev_t * td, int8_t * buffer)
{
    struct bio_container *tmp_bio_c, *tmp_bio_c_next;
    int32_t len;

    len = 0;
    spin_lock(&td->datalog_list_spinlock[0]);
    list_for_each_entry_safe(tmp_bio_c, tmp_bio_c_next, &td->gc_bioc_list, list) {
        len += sprintf(buffer + len, "(%lldS,%uB)->(%lldP,%lldP) c %d [%p]) \n",
                       (tmp_bio_c->org_bio) ? (sector64) tmp_bio_c->org_bio->
                       bi_iter.bi_sector : 0,
                       (tmp_bio_c->org_bio) ? tmp_bio_c->org_bio->bi_iter.
                       bi_size : 0, tmp_bio_c->start_lbno,
                       tmp_bio_c->end_lbno - tmp_bio_c->start_lbno + 1,
                       atomic_read(&tmp_bio_c->conflict_pages), tmp_bio_c);
        if (len > 256) {
            len += sprintf(buffer + len, "...\n");
            break;
        }
    }

    spin_unlock(&td->datalog_list_spinlock[0]);
    len += sprintf(buffer + len, "------\n");
    return len;
}

struct bio_container *bioc_lock_lpns_wait(lfsm_dev_t * td, sector64 lpn,
                                          int32_t cn_fpages)
{
    struct bio_container *pbioc;
    int32_t HasConflict;

    if (NULL == (pbioc = bioc_get(td, NULL, 0, 0, 0, 0, WRITE, 0))) {
        return ERR_PTR(-ENOMEM);
    }

    pbioc->start_lbno = lpn;
    pbioc->end_lbno = lpn + cn_fpages - 1;
    pbioc->write_type = TP_FLUSH_IO;

    if ((HasConflict =
         bioc_insert_activelist(pbioc, td, true, TP_FLUSH_IO, WRITE)) < 0) {
        LOGERR("Error conflict\n");
        put_bio_container(td, pbioc);
        return ERR_PTR(-EINVAL);
    }

    bioc_wait_conflict(pbioc, td, HasConflict);
    return pbioc;
}

void bioc_unlock_lpns(lfsm_dev_t * td, struct bio_container *pbioc)
{
    post_rw_check_waitlist(td, pbioc, 1);
    put_bio_container(td, pbioc);
}

/* get_bio_container get one temp bio container from datalog_free_list
 * @[in]td: disk info
 * @[in]org_bio: original bio, indicating if the allocated active list item needs to consider the confliction
 * @[in]s_lbno: lbno after alignment in the unit of flash page (>=memory page)
 * @[in]e_lbno: the last lbno after alignment in the unit of flash page
 * @[in]write_type: 0, non-partial write; 1, partial write
 * @[in]rw: READ/WRITE for the arrival cmd
 * @[return] bio_container* bio_c ready to be used
 */
int32_t bioc_extra_free(lfsm_dev_t * td)
{
    struct bio_container *pbioc, *pbioc_next;
    int32_t idx;

    spin_lock(&td->datalog_list_spinlock[0]);
    list_for_each_entry_safe(pbioc, pbioc_next, &td->datalog_free_list[0], list) {
        if ((idx = garbioc_search_extra_idx(td, pbioc)) < 0) {
            continue;
        }
        garbioc_replace(td, idx);

        list_del_init(&pbioc->list);
        atomic_dec(&td->len_datalog_free_list);
        atomic_dec(&td->cn_extra_bioc);
    }
    spin_unlock(&td->datalog_list_spinlock[0]);

    return 0;
}

void bioc_get_init(struct bio_container *pbioc, sector64 lpn_start,
                   int32_t cn_fpage, int32_t write_type)
{
    int32_t i;
    pbioc->start_lbno = lpn_start;
    pbioc->end_lbno = lpn_start + cn_fpage - 1;
    pbioc->write_type = write_type;
    atomic_set(&pbioc->conflict_pages, 0);
    INIT_LIST_HEAD(&pbioc->wait_list);
    INIT_LIST_HEAD(&pbioc->submitted_list);
    for (i = 0; i < MAX_CONFLICT_HOOK; i++) {
        INIT_LIST_HEAD(&pbioc->arhook_conflict[i]);
    }
    pbioc->fg_pending = FG_IO_NO_PEND;
    init_waitqueue_head(&pbioc->io_queue);
    return;

}

int32_t bioc_alloc_add2list(lfsm_dev_t * td, int32_t idx)
{
    struct bio_container *pbioc;

    if (NULL ==
        (pbioc = bioc_alloc(td, td->data_bio_max_size, MEM_PER_FLASH_PAGE))) {
        LOGERR(" alloc_bio_container fail\n");
        return -ENOMEM;
    }

    spin_lock(&td->datalog_list_spinlock[idx]);
    if (gidx_arbioc < (td->freedata_bio_capacity + CN_EXTRA_BIOC)) {
        gar_pbioc[gidx_arbioc] = pbioc;
        gidx_arbioc++;
    }
    list_add(&pbioc->list, &td->datalog_free_list[idx]);
    pbioc->idx_group = idx;
    spin_unlock(&td->datalog_list_spinlock[idx]);
    atomic_inc(&td->len_datalog_free_list);

    return 0;
}

static int32_t bioc_list_empty_handler(lfsm_dev_t * td, int32_t tp_get,
                                       int32_t idx)
{
    int32_t ret;

    ret = 0;
    switch (tp_get) {
    case 0:                    // wait
        wait_event_interruptible(td->wq_datalog,
                                 atomic_read(&td->len_datalog_free_list) > 0);
        break;
    case 1:                    // alloc a new
        if ((ret = bioc_alloc_add2list(td, idx)) >= 0) {    //>=0 is success
            atomic_inc(&td->cn_extra_bioc);
        }
        break;
    default:                   // error
        return -ENOMEM;
    }
    return ret;
}

static struct bio_container *bioc_get_A(lfsm_dev_t * td, int32_t tp_get)
{
    struct bio_container *pbioc;
    int32_t cn_retry, idx;

    pbioc = NULL;
    cn_retry = 0;

  l_begin:
    idx = smp_processor_id() % DATALOG_GROUP_NUM;

    spin_lock(&td->datalog_list_spinlock[idx]);
    if (list_empty(&td->datalog_free_list[idx])) {
        spin_unlock(&td->datalog_list_spinlock[idx]);
        if (bioc_list_empty_handler(td, tp_get, idx) < 0) {
            return NULL;
        }

        cn_retry++;
        goto l_begin;
    }

    pbioc =
        list_entry(td->datalog_free_list[idx].next,
                   typeof(struct bio_container), list);
    list_del_init(&pbioc->list);
    spin_unlock(&td->datalog_list_spinlock[idx]);

    atomic_dec(&td->len_datalog_free_list);
    atomic_set(&pbioc->active_flag, ACF_ONGOING);   //suppose that this bio won't be used while the flag is ONGOING
    return pbioc;
}

struct per_page_active_list_item *get_per_page_list_item(lfsm_dev_t * td)
{
    struct per_page_active_list_item *pRetItem, *pTmp;
    unsigned long perpagepool_list_flag;
    spin_lock_irqsave(&td->per_page_pool_lock, perpagepool_list_flag);

    LFSM_ASSERT(td->per_page_pool_cnt > 0);
    td->per_page_pool_cnt--;
    LFSM_ASSERT(list_empty(&td->per_page_pool_list) == 0);

//    pRetItem=list_entry(td->per_page_pool_list.next,typeof(struct per_page_active_list_item),page_list);
//    list_del(pRetItem);

    //TODO: change to get first entry and bad coding style
    list_for_each_entry_safe(pRetItem, pTmp, &td->per_page_pool_list, page_list) {
        list_del(&pRetItem->page_list);
        spin_unlock_irqrestore(&td->per_page_pool_lock, perpagepool_list_flag);
        return pRetItem;
    }

    spin_unlock_irqrestore(&td->per_page_pool_lock, perpagepool_list_flag);

    return 0;
}

/*
 * if org_bio is NULL means not user's IO
 *    org_bio.bi_sector = 0, means GC (bioc_gc_get_and_wait)
 *    otherwise, for user IO (bioc_get_and_wait)
 */
struct bio_container *bioc_get(lfsm_dev_t * td, struct bio *org_bio,
                               sector64 org_bi_sector, sector64 lpn_start,
                               sector64 lpn_end, int32_t write_type, int32_t rw,
                               int32_t tp_get)
{
    struct bio_container *bio_c;
    int32_t i, cn_page;

    //    printk("bi_sector=%llus (%llup) %llup \n",org_bi_sector, org_bi_sector>>4,s_lbno);
    LFSM_ASSERT2(lpn_end >= lpn_start, "lpn_end %llu ,  lpn_start %llu\n",
                 lpn_end, lpn_start);

    if (NULL == (bio_c = bioc_get_A(td, tp_get))) {
        return NULL;
    }

    bio_c->dest_pbn = -1;
    bio_c->source_pbn = -1;

#if LFSM_PAGE_SWAP == 1
    for (i = 0; i < MEM_PER_FLASH_PAGE; i++) {
        bio_c->ar_ppage_swap[i] = NULL;
    }
#endif
    if (org_bio == NULL) {
        bioc_get_init(bio_c, 0, 1, write_type);
        for (i = 0; i < MAX_FLASH_PAGE_NUM; i++) {
            bio_c->per_page_list[i] = 0;
        }
    } else {
        bio_c->org_bio = org_bio;
        bioc_get_init(bio_c, lpn_start, lpn_end - lpn_start + 1, write_type);
        cn_page = bio_c->end_lbno - bio_c->start_lbno + 1;

#if LFSM_REDUCE_BIOC_PAGE == 1
        cn_page -= MEM_PER_FLASH_PAGE;  //first page already allocated
        bio_c->cn_alloc_pages += cn_page;
        if (bioc_alloc_page
            (bio_c->bio, cn_page, org_bi_sector, MEM_PER_FLASH_PAGE) < 0) {
            LOGERR("Fail to alloc follow pages %d\n", cn_page);
            return NULL;
        }
#endif

        if (bio_c->end_lbno == bio_c->start_lbno) {
            bio_c->per_page_list[0] = &bio_c->ppl_local;
        } else {
            for (i = 0; i < (bio_c->end_lbno - bio_c->start_lbno + 1); i++) {
                LFSM_ASSERT(i < MAX_FLASH_PAGE_NUM);
                bio_c->per_page_list[i] = get_per_page_list_item(td);
                LFSM_ASSERT(bio_c->per_page_list[i] != NULL);
            }
        }

        LFSM_ASSERT2(bio_c->per_page_list[0] != NULL,
                     "lpn_start=%llu e_lbno=%llu\n", lpn_start, lpn_end);
        /* Init the per-page data structure */
        init_per_page_list_item(bio_c->per_page_list, bio_c, td, org_bio,
                                org_bi_sector, bio_c->start_lbno,
                                bio_c->end_lbno, write_type, rw);
    }

    return bio_c;
}

// old version
// default true
// gc conflict false
// new version
// 1: actual conflict
// 0: no conflict
// -ENOENT : gc abort
int32_t bioc_insert_activelist(struct bio_container *bioc, lfsm_dev_t * td,
                               bool isConflictCheck, int32_t write_type,
                               int32_t rw)
{
    int32_t ret;
    int32_t isGC;

    isGC = (write_type == TP_GC_IO);

    if (isConflictCheck) {
        ret = conflict_bioc_login(bioc, td, isGC);
        if (ret == -ENOENT) {   // if bioc is a gc then, don't force conflict
            conflict_gc_abort_helper(bioc, td);
        } else {
            if (isGC) {         // gc bioc should be insert into active list to allow reback during conflict
                spin_lock(&td->datalog_list_spinlock[bioc->idx_group]);
                list_move_tail(&bioc->list, &td->gc_bioc_list); // datalog_active_list become gc_bioc_list, only insert gc bioc
                spin_unlock(&td->datalog_list_spinlock[bioc->idx_group]);
            }
        }
        return ret;
    }

    return 0;
}

void bioc_return(lfsm_dev_t * td, struct bio_container *bio_c)
{
#if LFSM_PAGE_SWAP == 1
    int32_t i;
#endif
    int32_t idx = bio_c->idx_group;

    bio_c->bio->bi_iter.bi_size = 0;
    bio_c->bio->bi_vcnt = 0;
#if LFSM_PAGE_SWAP == 1
    for (i = 0; i < MEM_PER_FLASH_PAGE; i++) {
        if (NULL != (bio_c->ar_ppage_swap[i])) {
            bio_c->bio->bi_io_vec[i].bv_page = bio_c->ar_ppage_swap[i];
        }
    }
#endif

    atomic_set(&bio_c->active_flag, ACF_IDLE);
    spin_lock(&td->datalog_list_spinlock[idx]);
    list_move_tail(&bio_c->list, &td->datalog_free_list[idx]);
    spin_unlock(&td->datalog_list_spinlock[idx]);

    atomic_inc(&td->len_datalog_free_list);
    if (waitqueue_active(&td->wq_datalog)) {
        wake_up_interruptible(&td->wq_datalog);
    }
}

/*
 * TODO: remove this function
 * only used at following function call
 * build_mark_bad_command: only validate at fw-modification version
 * build_lpn_query: only validate at fw-modification version
 */
struct bio_container *get_bio_container(lfsm_dev_t * td, struct bio *org_bio,
                                        sector64 org_bi_sector,
                                        sector64 lpn_start, sector64 lpn_end,
                                        int32_t write_type, int32_t rw)
{
    struct bio_container *bio_c;
    int32_t ret;

    bio_c =
        bioc_get(td, org_bio, org_bi_sector, lpn_start, lpn_end, write_type, rw,
                 0);
    if (NULL == bio_c) {
        LOGERR("Fail to get bioc for lpn %lld %lld tp %d\n", lpn_start, lpn_end,
               write_type);
        return NULL;
    }
#if LFSM_CONFLICT_CHECK == 1
    if (org_bio) {
        ret = bioc_insert_activelist(bio_c, td, true, write_type, rw);
    } else {
        ret = bioc_insert_activelist(bio_c, td, false, write_type, rw);
    }
#endif

    if (ret >= 0) {             // either conflict or not is acceptable
        return bio_c;
    } else {                    // gc abort
        bioc_return(td, bio_c);
        return NULL;
    }
}

int32_t free_all_per_page_list_items(lfsm_dev_t * td,
                                     struct bio_container *bio_c)
{
    unsigned long flag;
    int32_t page_cnt, i;

    if (bio_c->write_type == TP_FLUSH_IO) {
        return 0;
    }
    page_cnt = (int32_t) (bio_c->end_lbno - bio_c->start_lbno) + 1;
//    LFSM_ASSERT(page_cnt>0);
    if (page_cnt == 0) {
        return 0;
    }

    if (page_cnt == 1) {
        bio_c->per_page_list[0] = NULL;
        return 0;
    }

    spin_lock_irqsave(&td->per_page_pool_lock, flag);
    for (i = 0; i < page_cnt; i++) {
        td->per_page_pool_cnt++;
        if (NULL == (bio_c->per_page_list[i])) {
            spin_unlock_irqrestore(&td->per_page_pool_lock, flag);
            LFSM_ASSERT2(bio_c->per_page_list[i] != NULL,
                         "bioc=%p lbn= {%llu,%llu} \n", bio_c,
                         bio_c->start_lbno, bio_c->end_lbno);
        }
        list_add_tail(&bio_c->per_page_list[i]->page_list,
                      &td->per_page_pool_list);
        bio_c->per_page_list[i] = NULL;
    }
    spin_unlock_irqrestore(&td->per_page_pool_lock, flag);
    return td->per_page_pool_cnt;
}

static int32_t bioc_free_page(struct bio *bio, int32_t page_num,
                              int32_t idx_start)
{
    int32_t i;

    if (NULL == bio) {
        return 0;
    }

    for (i = 0; i < page_num; i++) {
        if (NULL != (bio->bi_io_vec[idx_start + i].bv_page)) {
            track_free_page(bio->bi_io_vec[idx_start + i].bv_page);
        }
        bio->bi_io_vec[idx_start + i].bv_page = NULL;
    }
    return 0;
}

 /* put_bio_container mark the active_flag = ACF_IDLE for move_from_active_to_free to recycle the bio container 
  * @[in]bio_c: bio container
  * @[return] void
  */
void put_bio_container(lfsm_dev_t * td, struct bio_container *bio_c)
{
#if LFSM_REDUCE_BIOC_PAGE ==1
    int32_t cn_page;
#endif

    if (bio_c->par_bioext) {
        bio_c->bio = bio_c->par_bioext->bio_orig;
        bioext_free(bio_c->par_bioext, bioc_cn_fpage(bio_c));
        bio_c->par_bioext = NULL;
    }
#if LFSM_REDUCE_BIOC_PAGE == 1
    cn_page = bio_c->cn_alloc_pages - MEM_PER_FLASH_PAGE;
    bioc_free_page(bio_c->bio, cn_page, MEM_PER_FLASH_PAGE);
    bio_c->cn_alloc_pages -= cn_page;
#endif

    bioc_return(td, bio_c);
}

// internal version of put_bio_container . Without locking.
/* put_bio_container mark the active_flag = ACF_IDLE for move_from_active_to_free to recycle the bio container 
 * @[in]bio_c: bio container
 * @[return] void
 */
// TODO: revise it, don't gain long lock
void put_bio_container_nolock(lfsm_dev_t * td, struct bio_container *bio_c,
                              sector64 prelocked_lbno)
{
    LFSM_ASSERT(spin_is_locked(&td->datalog_list_spinlock[bio_c->idx_group]));
    bio_c->bio->bi_iter.bi_size = 0;
    bio_c->bio->bi_vcnt = 0;
    atomic_set(&bio_c->active_flag, ACF_IDLE);
    list_move_tail(&bio_c->list, &td->datalog_free_list[bio_c->idx_group]);
    atomic_inc(&td->len_datalog_free_list);
    wake_up_interruptible(&td->wq_datalog);
}

/* init_bio_container initialization of the bio container
 * @[in]bio_c: bio container
 * @[in]bio: bio
 * @[in]bdev: device info
 * @[in]bi_end_io: HWint32_t handler for the end_bio
 * @[in]rw: rw flag
 * @[return] void
 */
void init_bio_container(struct bio_container *bio_c, struct bio *bio,
                        struct block_device *bdev, bio_end_io_t * bi_end_io,
                        int32_t rw, sector64 bi_sector, int32_t vcnt)
{
    bio_c->org_bio = bio;
    bio_c->bytes_submitted = vcnt * PAGE_SIZE;
    bio_c->status = BLK_STS_OK;
    bio_c->par_bioext = NULL;
    bio_c->io_queue_fin = 0;
    bio_c->io_rw = rw;
    //bio_c->bioc_req_id = atomic_add_return(1, &ioreq_counter);
    bio_c->is_user_bioc = false;

//    atomic_set(&bio_c->cn_wait, 0);
    atomic_set(&bio_c->cn_wait, vcnt / MEM_PER_FLASH_PAGE);
    bio_lfsm_init(bio_c->bio, bio_c, bdev, bi_end_io, rw, bi_sector, vcnt);
    return;
}

void bio_lfsm_init(struct bio *bio, struct bio_container *bio_c,
                   struct block_device *bdev, bio_end_io_t * bi_end_io,
                   int32_t rw, sector64 bi_sector, int32_t vcnt)
{
    int32_t i;

    bio->bi_private = bio_c;
    bio->bi_disk = (bdev != NULL) ? bdev->bd_disk : NULL;
    bio->bi_end_io = bi_end_io;
    bio->bi_private = bio_c;
    bio->bi_opf = rw;
    bio->bi_iter.bi_idx = 0;

    bio->bi_iter.bi_size = vcnt * PAGE_SIZE;
    bio->bi_status = BLK_STS_OK;
    bio->bi_flags &= ~(1 << BIO_SEG_VALID);
    bio->bi_iter.bi_sector = bi_sector;
    bio->bi_vcnt = vcnt;
    for (i = 0; i < vcnt; i++) {
        bio->bi_io_vec[i].bv_len = PAGE_SIZE;
        bio->bi_io_vec[i].bv_offset = 0;
    }

    bio->bi_phys_segments = 0;
    bio->bi_seg_front_size = 0;
    bio->bi_seg_back_size = 0;
    bio->bi_next = NULL;
    bio->bi_iter.bi_idx = 0;
}

sector64 glpn_last_conflict = 0;
atomic_t gcn_gc_conflict_giveup = { 0 };

struct bio_container *bioc_gc_get_and_wait(lfsm_dev_t * td,
                                           struct bio *bio_user, sector64 lpn)
{
    struct bio_container *bio_c;
    int32_t ret;

    bio_c =
        bioc_get(td, bio_user, bio_user->bi_iter.bi_sector, lpn, lpn, TP_GC_IO,
                 WRITE, 1);
    ret = 0;
    if (NULL == bio_c) {
        return bio_c;
    }
#if LFSM_CONFLICT_CHECK == 1
    ret = bioc_insert_activelist(bio_c, td, true, TP_GC_IO, WRITE); // ret must be > 0 because it's not a gc bio_c
#endif
    if (ret == -ENOENT) {
        glpn_last_conflict = lpn;
        atomic_inc(&gcn_gc_conflict_giveup);
        free_all_per_page_list_items(td, bio_c);
        bioc_return(td, bio_c);
        return NULL;
    }
    LFSM_ASSERT2(ret == 0,
                 "GC bioc should not be confliced with others ret=%d\n", ret);
    init_bio_container(bio_c, NULL, NULL, end_bio_write, REQ_OP_READ, 0, 0);
    do_gettimeofday(&bio_c->tm_start);
    return bio_c;
}

struct bio_container *bioc_get_and_wait(lfsm_dev_t * td, struct bio *bio_user,
                                        int32_t opf)
{
    struct bio_container *bio_c;
    int32_t rw = ((opf & REQ_OP_MASK) & REQ_OP_WRITE ? WRITE : READ);
    bio_end_io_t *func_end_bio = ((rw) ? end_bio_write : NULL);
    sector64 local_lbno;
    int32_t HasConflict, io_type, destbio_page_no;

    local_lbno = bio_user->bi_iter.bi_sector >> SECTORS_PER_FLASH_PAGE_ORDER;
    destbio_page_no =
        BIO_SIZE_ALIGN_TO_FLASH_PAGE(bio_user) >> FLASH_PAGE_ORDER;
    io_type =
        ((destbio_page_no << FLASH_PAGE_ORDER) !=
         bio_user->bi_iter.bi_size) ? TP_PARTIAL_IO : TP_NORMAL_IO;

    bio_c = bioc_get(td, bio_user, bio_user->bi_iter.bi_sector,
                     local_lbno, local_lbno + destbio_page_no - 1, io_type, rw,
                     0);
    if (NULL == bio_c) {
        return bio_c;
    }

    HasConflict = 0;
#if LFSM_CONFLICT_CHECK == 1
    HasConflict = bioc_insert_activelist(bio_c, td, true, io_type, rw); // ret must be > 0 because it's not a gc bio_c
#endif
    if (HasConflict) {
        (rw ==
         0) ? atomic_inc(&td->cn_conflict_pages_r) : atomic_inc(&td->
                                                                cn_conflict_pages_w);
    }

    init_bio_container(bio_c, bio_user, NULL, func_end_bio, opf,
                       (rw) ? 0 : LFSM_FLOOR(bio_user->bi_iter.bi_sector,
                                             SECTORS_PER_FLASH_PAGE),
                       destbio_page_no * MEM_PER_FLASH_PAGE);
    bio_c->is_user_bioc = true;
    do_gettimeofday(&bio_c->tm_start);
    bioc_wait_conflict(bio_c, td, HasConflict);
    return bio_c;
}

/* end_bio_page_read for read_page
 * @[in]bio: bio 
 * @[in]err: 
 * @[return] void
 */
void end_bio_page_read_pipeline(struct bio *bio)
{
    struct bio_container *entry;
    lfsm_dev_t *td;

    entry = bio->bi_private;
    td = entry->lfsm_dev;
    LFSM_ASSERT((atomic_read(&bio->__bi_cnt) % MEM_PER_FLASH_PAGE) == 0);

    entry->per_page_list[atomic_read(&bio->__bi_cnt) / MEM_PER_FLASH_PAGE]->ready_bit = 1;  // page num is hidden in bio->__bi_cnt
    if (bio->bi_status != BLK_STS_OK) {
        entry->io_queue_fin = blk_status_to_errno(bio->bi_status);
        entry->per_page_list[atomic_read(&bio->__bi_cnt) / MEM_PER_FLASH_PAGE]->ready_bit = 2;  // for fail re_read
//        printk("%s: Get a bio %p bioc %p err message %d\n",__FUNCTION__,entry, bio, err);
    }

    atomic_set(&bio->__bi_cnt, 1);
    track_bio_put(bio);

    if (atomic_dec_and_test(&entry->cn_wait) == true) { // all pages have result
        if (entry->io_queue_fin == 0) {
            entry->io_queue_fin = 1;
        }

        biocbh_io_done(&td->biocbh, entry, true);
    }

    return;
}

void end_bio_page_read(struct bio *bio)
{
    struct bio_container *entry;
    int32_t err;
//    lfsm_dev_t *td = entry->lfsm_dev;

    entry = bio->bi_private;

    if (bio->bi_status != BLK_STS_OK) {
        err = blk_status_to_errno(bio->bi_status);
        atomic_set(&entry->active_flag, err);
        entry->io_queue_fin = err;
    } else {
        atomic_set(&entry->active_flag, ACF_PAGE_READ_DONE);
        entry->io_queue_fin = 1;
    }
//    wake_up_interruptible(&td->subdev[0].page_read_wait_q);
    wake_up_interruptible(&entry->io_queue);

    return;
}

static void _read_pages_wait(struct bio_container *entry)
{
    int32_t cnt, wait_ret;

    cnt = 0;
    while (true) {
        cnt++;
        wait_ret = wait_event_interruptible_timeout(entry->io_queue,
                                                    atomic_read(&entry->
                                                                active_flag) !=
                                                    ACF_ONGOING,
                                                    LFSM_WAIT_EVEN_TIMEOUT);
        if (wait_ret <= 0) {
            LOGWARN("read pages IO no response %lld after seconds %d\n",
                    (sector64) entry->bio->bi_iter.bi_sector,
                    LFSM_WAIT_EVEN_TIMEOUT_SEC * cnt);
        } else {
            break;
        }
    }

}

static int32_t read_pages(lfsm_dev_t * td, sector64 pos,
                          struct flash_page *flashpage,
                          int32_t page_num_in_bioc, struct bio_container *entry,
                          bool is_pipe_line, int32_t page_num,
                          bio_end_io_t * bi_end_io)
{
    struct bio *page_bio;
    struct bio_vec *bv;
    int32_t ret, try, i;
    //Compose a page bio for read

    //printk("read_flash_page pbno: %llu page_ptr:%p is_pipeline : %d\n",pos,flashpage,(int)is_pipe_line);

    try = 0;
    ret = 0;
    while (NULL ==
           (page_bio = track_bio_alloc(GFP_ATOMIC | __GFP_ZERO, page_num))) {
        LOGERR("allocate header_bio fails in read_flash_page(retry=%d)\n", try);
        schedule();
        if (try++ > LFSM_MAX_RETRY) {
            return -ENOMEM;
        }
    };
    entry->bio->bi_iter.bi_sector = pos;    /// tifa add for logger_enqueue
    page_bio->bi_iter.bi_sector = pos;
    page_bio->bi_disk = NULL;
    page_bio->bi_iter.bi_idx = 0;
    page_bio->bi_end_io = bi_end_io;
    page_bio->bi_private = entry;
    page_bio->bi_vcnt = page_num;   // flash page
    page_bio->bi_iter.bi_size = page_num * PAGE_SIZE;   // 1 pages
    page_bio->bi_opf = REQ_OP_READ;
    page_bio->bi_status = BLK_STS_OK;
    page_bio->bi_flags &= ~(1 << BIO_SEG_VALID);
    page_bio->bi_phys_segments = 0;
    //page_bio->bi_hw_segments = 0;
    page_bio->bi_seg_front_size = 0;
    page_bio->bi_seg_back_size = 0;
    page_bio->bi_next = NULL;
    if (is_pipe_line) {
        atomic_set(&page_bio->__bi_cnt, page_num_in_bioc);  // for read pipe_line usage
    }

    bio_for_each_segment_all(bv, page_bio, i) {
        bv->bv_page = flashpage->page_array[i];
        bv->bv_offset = 0;
        bv->bv_len = PAGE_SIZE;
    }
    try = 0;

    entry->ts_submit_ssd = jiffies_64;
    while (try++ < 1) {
        if (is_pipe_line) {
            return my_make_request(page_bio);
        } else {
            atomic_set(&entry->active_flag, ACF_ONGOING);
            if ((ret = my_make_request(page_bio)) != 0) {
                goto l_fail;
            }
        }

        //lfsm_wait_event_interruptible(entry->io_queue,
        //        atomic_read(&entry->active_flag) != ACF_ONGOING);
        _read_pages_wait(entry);

        if (atomic_read(&entry->active_flag) == ACF_PAGE_READ_DONE) {
            break;
        }
    }

    if (atomic_read(&entry->active_flag) != ACF_PAGE_READ_DONE) {
        ret = -EIO;
    }

  l_fail:
    atomic_set(&entry->active_flag, ACF_ONGOING);
    dprintk("pid#thread:%d#%d Decompress bio pbno:%llu bi_size: %d\n",
            current->pid, current->tgid, pos, page_bio->bi_iter.bi_size);

    track_bio_put(page_bio);

    return ret;

}

int32_t read_flash_page_vec(lfsm_dev_t * td, sector64 pos,
                            struct bio_vec *page_vec, int32_t idx_mpage_in_bio,
                            struct bio_container *entry, bool is_pipe_line,
                            bio_end_io_t * ar_bi_end_io[MAX_ID_ENDBIO])
{
    struct flash_page flashpage;
    int32_t i;

    for (i = 0; i < MEM_PER_FLASH_PAGE; i++) {
        flashpage.page_array[i] = page_vec->bv_page;
        page_vec++;
    }
    return read_flash_page(td, pos, &flashpage, idx_mpage_in_bio, entry,
                           is_pipe_line, ar_bi_end_io);
}

int32_t read_flash_page(lfsm_dev_t * td, sector64 pos,
                        struct flash_page *flashpage, int32_t idx_mpage_in_bio,
                        struct bio_container *entry, bool is_pipe_line,
                        bio_end_io_t * ar_bi_end_io[MAX_ID_ENDBIO])
{
//    entry->dest_pbn = pos; //can't assign dest_pbn, because read function could be used by copy and source_pbn

    return (read_pages(td, pos, flashpage, idx_mpage_in_bio, entry,
                       is_pipe_line, MEM_PER_FLASH_PAGE,
                       ar_bi_end_io[ID_ENDBIO_NORMAL]));
}

/* ibvpage_RW used to construct the dest_bio->bi_io_vec[ori_vcnt].bv_page
 * @[in]ulldsk_boffset: byte disk offset of current bv_page 
 * @[in]in_buf: (W)kmap(source_bio->bi_io_vec[ori_vcnt].bv_page)
 *             (R)destination buffer bio-> bi_io_vec[ori_vcnt].bv_page
 * @[in]uiblen: source_bio->bi_io_vec[ori_vcnt].bv_len
 * @[in]ori_vcnt: current index of bi_io_vec
 * @[in]bio_c: (W)bio container include source bio (bio_c->org_bio) and dest bio (bio_c->bio)
 *                  (R) bio container of the source bio (bio_c->bio) which has disk real data
 * @[in]RW: (W)WRITE (R)READ
 * @[return] 0:ok -1:error
 */
int32_t ibvpage_RW(uint64_t ulldsk_boffset, void *in_buf, uint32_t uiblen,
                   uint32_t ori_vcnt, struct bio_container *bio_c, int32_t RW)
{
    //bytes has been written in the incoming vec
    //bytes offset of the dest
    //bytes copied
    uint64_t done_cnt, this_off, this_cnt;
    struct bio *ori_bio = bio_c->org_bio;
    void *out_page;
    int32_t new_vcnt;

    done_cnt = 0;
    new_vcnt = 0;

    while (done_cnt < uiblen) {
        this_off = (ulldsk_boffset + done_cnt) & N_PAGE_MASK;   //(ulldsk_boffset + done_cnt)-->current disk offset
        this_cnt = min(uiblen - done_cnt, (uint32_t) PAGE_SIZE - this_off); //min(data remained, free page size remained)
        // align new_vcnt to flash page before converting it back to memory page
        new_vcnt = (((ulldsk_boffset + done_cnt) >> PAGE_ORDER) - ((ori_bio->bi_iter.bi_sector >> SECTORS_PER_FLASH_PAGE_ORDER) * MEM_PER_FLASH_PAGE)); // current out-vec index
        mpp_printk(" this_off %llu  this_cnt %llu new_vcnt %d \n", this_off,
                   this_cnt, new_vcnt);
        if (RW == WRITE) {
            out_page = kmap(bio_c->bio->bi_io_vec[new_vcnt].bv_page);
            if (NULL == out_page) {
                LOGERR("Can not convert address back from a bio\n");
                return -1;
            }
// TURN-ON/OFF (memcpy)
            memcpy((out_page + this_off), (in_buf + (done_cnt % PAGE_SIZE)),
                   this_cnt);
            kunmap(bio_c->bio->bi_io_vec[new_vcnt].bv_page);
        } else if (RW == READ) {
            if (bio_c->bio->bi_io_vec[new_vcnt].bv_len != 0) {
                out_page = kmap(bio_c->bio->bi_io_vec[new_vcnt].bv_page);
                if (NULL == out_page) {
                    LOGERR("Can not convert address back from a bio\n");
                    return -1;
                }

                memcpy(in_buf + done_cnt, (out_page + this_off), this_cnt);

                kunmap(bio_c->bio->bi_io_vec[new_vcnt].bv_page);
            } else {
                memset(in_buf + done_cnt, 0, this_cnt);
            }
        }
        done_cnt += this_cnt;
    }

    return 0;
}

uint64_t get_elapsed_time2(struct timeval *cur, struct timeval *prev)
{
    return (cur->tv_sec - prev->tv_sec) * 1000000 + (cur->tv_usec -
                                                     prev->tv_usec);
}

static bool bio_verify(lfsm_dev_t * td, struct bio *bio)
{
    sector64 lpn_start, lpn_next;
#ifdef CHK_BIO_BVPAG_KMAP
    int8_t *addr;
#endif

    if (NULL == bio) {
        return false;
    }

    if (NULL == bio->bi_io_vec) {
        // AN: remove log after l_fail
        goto l_fail;
    }
    if (NULL == bio->bi_io_vec[0].bv_page) {
        // AN : remove log after l_fail
        goto l_fail;
    }
#ifdef CHK_BIO_BVPAG_KMAP
    /*
     * NOTE: when enable this, application threads consume high CPU.
     * especially in low CPU freq environment
     *
     * disable it internally
     */
    //check bv_page isn't NULL
    addr = kmap(bio->bi_io_vec[0].bv_page);
    if (NULL == addr) {
        LOGERR
            ("Page is null for %llu, size: %d, page: %p, addr: %p, page-address: %p, dir: %lu\n",
             (sector64) bio->bi_iter.bi_sector, bio_sectors(bio),
             bio->bi_io_vec[0].bv_page, addr,
             page_address(bio->bi_io_vec[0].bv_page), bio_data_dir(bio));

        kunmap(bio->bi_io_vec[0].bv_page);
        goto l_fail;
    }
    kunmap(bio->bi_io_vec[0].bv_page);
#endif

    // check bi_sector is overbound
    if (bio->bi_iter.bi_sector + (bio->bi_iter.bi_size >> SECTOR_ORDER) >
        SECTORS_PER_FLASH_PAGE * td->logical_capacity) {
        LOGERR("Over the size of LFSM %llx > %llx\n",
               (sector64) (bio->bi_iter.bi_sector +
                           (bio->bi_iter.bi_size >> SECTOR_ORDER)),
               SECTORS_PER_FLASH_PAGE * td->logical_capacity);
        goto l_fail;
    }

    if (bio->bi_vcnt == 0) {
        goto l_fail;
    }

    lpn_start = LFSM_FLOOR_DIV(bio->bi_iter.bi_sector, SECTORS_PER_FLASH_PAGE);
    lpn_next = LFSM_CEIL_DIV(bio_end_sector(bio), SECTORS_PER_FLASH_PAGE);
    if ((lpn_next - lpn_start) > MAX_FLASH_PAGE_NUM) {
        goto l_fail;
    }

    return true;

  l_fail:

    if (!IS_FLUSH_BIO(bio)) {
        LOGERR("bi_io_vec is null for %llu, size: %d, rw: %u flags: %u\n",
               (sector64) bio->bi_iter.bi_sector, bio_sectors(bio), bio->bi_opf,
               bio->bi_flags);
        bio->bi_status = BLK_STS_RESOURCE;
    } else {
        bio->bi_status = BLK_STS_OK;
    }
    bio_endio(bio);

    return false;
}

EXPORT_SYMBOL(request_enqueue);

static void io_enqueue(lfsm_dev_t * td, struct bio *bio)
{
    if (bioboxpool_free2used(&td->bioboxpool, bio, -1)) {
        bio->bi_status = BLK_STS_RESOURCE;
        bio_endio(bio);
    }
}

static void bio_verify_and_enqueue(lfsm_dev_t * td, struct bio *bio)
{
    if (bio_verify(td, bio)) {  // bio is endio inside bio_verify if false
        io_enqueue(td, bio);
    }
}

static int32_t ctrlbio_preprocess(lfsm_dev_t * td, struct bio *bio)
{
    int32_t ret;

    ret = 0;
#if LFSM_FLUSH_SUPPORT == 1
    struct sflushbio *pflushbio = NULL;
#endif

    if (IS_TRIM_BIO(bio)) {     // is trim bit is set, always insert
        atomic_inc(&td->cn_trim_bio);
        io_enqueue(td, bio);
        return 0;
    } else
#if LFSM_FLUSH_SUPPORT == 1
    if (IS_FLUSH_BIO(bio)) {
        atomic_inc(&td->cn_flush_bio);
        if ((pflushbio = flushbio_alloc(bio)) == NULL) {    // bio's CB and private data is intercepted(changed)
            return 0;           //bio is ended inside
        }

        bio_verify_and_enqueue(td, bio);    // bio is io ended if verify fail, no return value
        flushbio_handler(pflushbio, bio, &td->bioboxpool);  // no matter true or false, user_bio is ended inside
        return 0;
    } else
#endif
        return -1;
}

static lfsm_dev_t *REQUEST_QUEUE_TO_PITDEV(struct request_queue *q)
{
    return (lfsm_dev_t *) (((struct gendisk *)(q->queuedata))->private_data);
}

static int32_t process_io_request(lfsm_dev_t * td, struct bio *bio,
                                  int32_t thdindex)
{
    int32_t err;
    if (ctrl_bio_filter(td, bio) == 0) {
        return 0;               // special bio and handled, direct rturn
    }
    err = 0;

    if (likely(bio_data_dir(bio) == WRITE)) {
        if (unlikely((err = iwrite_bio(td, bio)) != 0)) {
            LOGERR("iwrite_bio error\n");
            return err;         //-EIO;
        }
    } else {
        if ((err = iread_bio(td, bio, thdindex)) != 0) {
            LOGERR("iread-bio fail\n");
            return err;         //-EIO;
        }
    }

    return err;
}

int32_t request_enqueue_A(struct request_queue *q, struct bio *bio)
{
    lfsm_dev_t *td;
    if (q) {
        td = REQUEST_QUEUE_TO_PITDEV(q);
    } else {
        td = &lfsmdev;
    }
/*
        if (process_io_request(td, bio, 0) < 0) {
            LOGERR("process_io_request bio %p <0\n", bio);
            bioc_resp_user_bio(bio, -EIO);
        }

   if ( 3>2)  return 0;
*/

    if ((atomic_read(&td->dev_stat) == LFSM_STAT_REJECT) ||
        ((atomic_read(&td->dev_stat) == LFSM_STAT_READONLY)
         && (bio_data_dir(bio) == WRITE))) {
        bioc_resp_user_bio(bio, -EIO);
        return 0;
    }

    if (ctrlbio_preprocess(td, bio) == 0) {
        return 0;               // this bio is handled inside preprocess
    }

    bio_verify_and_enqueue(td, bio);    // bio is  endio if verify fail, or insert in to lfsm, no return value

    return 0;
}

int request_dequeue(void *arg)
{
    usrbio_handler_t *myhandler;
    struct bio *bio;
    lfsm_dev_t *td;
    int64_t t_start;
    int32_t ret, io_rw, thd_index;

    myhandler = (usrbio_handler_t *) arg;
    td = (lfsm_dev_t *) myhandler->usr_private_data;
    current->flags |= PF_MEMALLOC;

    LOGINFO("io thread %s start with index %d\n", myhandler->pIOthread->comm,
            myhandler->handler_index);
    thd_index = myhandler->handler_index;
    while (true) {
#if LFSM_REVERSIBLE_ASSERT == 1
        LFSM_SHOULD_PAUSE();
#else
        LFSM_ASSERT2(assert_flag == 0,
                     "An assert has been broken by other threads %d\n", 0);
#endif

        if (NULL == (bio = bioboxpool_used2free(&td->bioboxpool))) {
            break;
        }

        INC_IOR_STAGE_CNT(IOR_STAGE_PROCESS_IOT, thd_index);

        if ((atomic_read(&td->dev_stat) == LFSM_STAT_REJECT) ||
            ((atomic_read(&td->dev_stat) == LFSM_STAT_READONLY)
             && (bio_data_dir(bio) == WRITE))) {
            bioc_resp_user_bio(bio, -EIO);
            DEC_IOR_STAGE_CNT(IOR_STAGE_PROCESS_IOT, thd_index);
            INC_IOR_STAGE_CNT(IOR_STAGE_LFSMIOT_PROCESS_DONE, thd_index);
            continue;
        }

        io_rw = bio_data_dir(bio);
        t_start = jiffies_64;
        if ((ret = process_io_request(td, bio, thd_index)) < 0) {
            LOGERR("process_io_request bio %p ret %d <0\n", bio, ret);
            bioc_resp_user_bio(bio, ret);
            DEC_IOR_STAGE_CNT(IOR_STAGE_PROCESS_IOT, thd_index);
            INC_IOR_STAGE_CNT(IOR_STAGE_LFSMIOT_PROCESS_DONE, thd_index);
            continue;
        }

        INC_IOR_STAGE_CNT(IOR_STAGE_SUBMIT_SSD, thd_index);
        DEC_IOR_STAGE_CNT(IOR_STAGE_PROCESS_IOT, thd_index);
        LOG_THD_PERF(current->pid, io_rw, t_start, jiffies_64);
        INC_REQ_IOPS();
    }

    return 0;
}

blk_qc_t request_enqueue(struct request_queue *q, struct bio *bio)
{
    return request_enqueue_A(q, bio);
}

bool set_flash_page_vec_zero(struct bio_vec *page_vec)
{
    int8_t *addr;
    int32_t i;

    for (i = 0; i < MEM_PER_FLASH_PAGE; i++) {
        addr = kmap(page_vec->bv_page);
        LFSM_ASSERT(addr != 0);
        memset(addr, 0, PAGE_SIZE);
        kunmap(page_vec->bv_page);
        page_vec++;
    }

    return true;
}

void free_composed_bio(struct bio *bio, int32_t bio_vec_size)
{
    if (!bio) {
        return;
    }

    bioc_free_page(bio, bio_vec_size, 0);

    track_bio_put(bio);
}

void free_bio_container(struct bio_container *bio_ct, int32_t bio_vec_size)
{
    free_composed_bio(bio_ct->bio, bio_vec_size);
    track_kfree(bio_ct, sizeof(struct bio_container), M_IO);
}

void end_bio_metalog(struct bio *bio)
{
    struct bio_container *entry;
    lfsm_dev_t *td;

    entry = bio->bi_private;
    td = entry->lfsm_dev;
    if (NULL == entry || NULL == td) {
        LOGINERR("Error in logic: %d\n", bio->bi_iter.bi_size);
        return;
    }

    entry->status = bio->bi_status;

    LFSM_ASSERT(0);

    if (entry) {
        LFSM_ASSERT(atomic_read(&entry->active_flag) == 1);
        atomic_set(&entry->active_flag, 0);
    }

    return;
}

struct bio_container *bioc_alloc(lfsm_dev_t * td, int32_t page_num,
                                 int32_t alloc_pages)
{
    struct bio_container *pbioc;

    pbioc = (struct bio_container *)
        track_kmalloc(sizeof(struct bio_container), GFP_KERNEL | __GFP_ZERO,
                      M_IO);
    if (NULL == pbioc) {
        LOGERR("Fail to alloc bioc\n");
        return NULL;
    }

    if (page_num == alloc_pages) {
        if (compose_bio(&pbioc->bio, NULL, end_bio_metalog, pbioc,
                        PAGE_SIZE * page_num, page_num) != 0) {
            LOGERR("compose_bio fail\n");
            goto l_free;
        }
    } else {
        if (compose_bio_no_page(&pbioc->bio, NULL, end_bio_metalog, pbioc,
                                PAGE_SIZE * page_num, page_num) != 0) {
            LOGERR("compose_bio_no_page fail\n");
            goto l_free;
        }
        if (bioc_alloc_page(pbioc->bio, alloc_pages, PAGE_SIZE * alloc_pages, 0)
            < 0) {
            LOGERR("Fail to alloc bio page\n");
            free_composed_bio(pbioc->bio, page_num);
            goto l_free;
        }

    }
#if LFSM_REDUCE_BIOC_PAGE == 1
    pbioc->cn_alloc_pages = alloc_pages;
#endif
    pbioc->lfsm_dev = td;
    atomic_set(&pbioc->active_flag, 0);
    init_waitqueue_head(&pbioc->io_queue);
    spin_lock_init(&pbioc->clean_waitlist_spinlock);
    pbioc->io_queue_fin = 0;
    atomic_set(&pbioc->conflict_pages, 0);
    pbioc->pstripe_wctrl = NULL;
    return pbioc;

  l_free:
    track_kfree(pbioc, sizeof(struct bio_container), M_IO);
    return NULL;
}

/* Check the waitlist of a bio_container to 1) copy data over, and 2) wakeup the corresponding item
 * @td: the device
 * @bio_c: the bio container
 * */
int32_t post_rw_check_waitlist(lfsm_dev_t * td, struct bio_container *bio_c,
                               bool failed)
{
    //TODO: Is finer-granularity lock needed?
    // move bio_ext to put_bio_container(), because mlog need bio_ext info to end IO
    conflict_bioc_logout(bio_c, td);
    free_all_per_page_list_items(td, bio_c);

    return 0;
}

/* 
** This is a wrapper function for kmalloc. Used to keep track of the memory consumption by LFSM.
**
** @size      : Number of bytes of memory requested.
** @flags     : GFP flags.
** @section    : Which section of the code is requesting the memory.
**
** Returns return value of kmalloc()
*/

/**copy command***/
int32_t copy_pages(lfsm_dev_t * td, sector64 source, sector64 dest,
                   sector64 lpn, int32_t run_length, bool isPipeline)
{
    sector64 addr;
    int32_t id_source, id_dest;

    if (!pbno_lfsm2bdev_A(source, &addr, &id_source, &grp_ssds)) {
    LFSM_ASSERT(0)};
    if (!pbno_lfsm2bdev_A(dest, &addr, &id_dest, &grp_ssds)) {
    LFSM_ASSERT(0)};

    LFSM_ASSERT2((run_length > 0) && (run_length <= MAX_COPY_RUNLEN),
                 "run_length %d over-bound\n", run_length);
    LFSM_ASSERT2(source % SECTORS_PER_FLASH_PAGE == 0,
                 "copy_page src should be in aligned sector: %lld\n", source);
    LFSM_ASSERT2(dest % SECTORS_PER_FLASH_PAGE == 0,
                 "copy_page src should be in aligned sector: %lld\n", source);
    atomic_add(run_length, &td->bioboxpool.cn_total_copied);
    if (id_source == id_dest) {
        return build_copy_command(td, source, dest, run_length, isPipeline);
    } else {
        return sflush_copy_submit(td, source, dest, lpn, run_length);
    }
}

bool copy_whole_eu_inter(lfsm_dev_t * td, sector64 source, sector64 dest,
                         int32_t cn_page)
{
    int32_t page_per_chunk;
    bool ret;

    ret = true;

    while (cn_page > 0) {
        if (cn_page > MAX_COPY_RUNLEN) {
            page_per_chunk = MAX_COPY_RUNLEN;
        } else {
            page_per_chunk = cn_page;
        }

        if (copy_pages(td, source, dest, -1, page_per_chunk, true) < 0) {
            ret = false;
            LOGERR("source %llu dest %llu fail\n", source, dest);
        }
        source += page_per_chunk << SECTORS_PER_FLASH_PAGE_ORDER;
        dest += page_per_chunk << SECTORS_PER_FLASH_PAGE_ORDER;
        cn_page -= page_per_chunk;
    }
    return ret;
}

bool copy_whole_eu_cross(lfsm_dev_t * td, sector64 source, sector64 dest,
                         sector64 * ar_lpn, int32_t cn_page, char *ar_valid_bit)
{
    struct sflushops *ops;
    int32_t i;

    ops = &td->crosscopy;
    sflush_open(ops);
    for (i = 0; i < cn_page; i++) {
        if (ar_valid_bit && (lfsm_test_bit(i, ar_valid_bit) == 0)) {
            continue;
        }
        sflush_copy_submit(td, source + i * SECTORS_PER_FLASH_PAGE,
                           dest + i * SECTORS_PER_FLASH_PAGE, ar_lpn[i], 1);
    }

    return sflush_waitdone(ops);
}

/*
 * For debugging purpose
 */
bool compare_whole_eu(lfsm_dev_t * td, struct EUProperty *eu_source,
                      struct EUProperty *eu_dest, int32_t cn_hpage_perlog,
                      bool compare_spare, bool comp_whole_eu,
                      sftl_err_msg_t * perr_msg,
                      bool (*parse_compare_fn)(lfsm_dev_t * td,
                                               int8_t * buf_main,
                                               int8_t * buf_backup))
{
    sftl_err_item_t item;
    sector64 log_frontier;
    int8_t *ar_buf[2] = { NULL }, *buffer[2] = { NULL };
    int32_t cn_comp_hpage, i, cn_eu;
    bool ret;

    cn_eu = 2;
    ret = false;

    memset(&item, 0, sizeof(sftl_err_item_t));

    LOGINFO
        ("eu_frontier: main : %d sectors(%d hpage) , backup : %d sectors(%d hpage) cn_fpage : %d \n",
         eu_source->log_frontier,
         eu_source->log_frontier >> SECTORS_PER_HARD_PAGE_ORDER,
         eu_dest->log_frontier,
         eu_dest->log_frontier >> SECTORS_PER_FLASH_PAGE_ORDER,
         cn_hpage_perlog);

    if (comp_whole_eu) {
        cn_comp_hpage = HPAGE_PER_SEU;
    } else {
        cn_comp_hpage = eu_dest->log_frontier >> SECTORS_PER_FLASH_PAGE_ORDER;
    }

    for (i = 0; i < cn_eu; i++) {
        if (NULL == (ar_buf[i] = track_kmalloc(HARD_PAGE_SIZE * cn_hpage_perlog,
                                               GFP_KERNEL, M_UNKNOWN))) {
            goto l_end;
        }

        if (NULL == (buffer[i] = (int8_t *)
                     track_kmalloc(SZ_SPAREAREA * FPAGE_PER_SEU, GFP_KERNEL,
                                   M_UNKNOWN))) {
            LOGERR("alloc mem failure\n");
            goto l_end;
        }
    }

    for (i = 0; i < cn_comp_hpage; i = i + cn_hpage_perlog) {
        log_frontier = i * SECTORS_PER_HARD_PAGE;

        if (!devio_read_hpages_batch(eu_source->start_pos + log_frontier,
                                     ar_buf[0], cn_hpage_perlog)) {
            LOGERR("read eu %llu fail, log_frontier %llu \n",
                   eu_source->start_pos, log_frontier);
            item.idx_page = i;
            item.err_code = -ESYS_MAP_DIFFERENT;
            goto l_end;
        }

        if (!devio_read_hpages_batch
            (eu_dest->start_pos + log_frontier, ar_buf[1], cn_hpage_perlog)) {
            LOGERR("read eu %llu fail, log_frontier %llu \n",
                   eu_source->start_pos, log_frontier);
            item.idx_page = i;
            item.err_code = -ESYS_MAP_DIFFERENT;
            goto l_end;
        }

        if (memcmp(ar_buf[0], ar_buf[1], HARD_PAGE_SIZE * cn_hpage_perlog) == 0) {
            //    LOGINFO("eu_pos %llu, eu_pos %llu content is the same log frotier %llu \n",
            //            eu_source->start_pos,eu_dest->start_pos,log_frontier);
            continue;
        }

        LOGWARN
            ("eu_pos %llu, eu_pos %llu content is different log_frotier :%llu \n",
             eu_source->start_pos, eu_dest->start_pos, log_frontier);

        if (parse_compare_fn) {
            (*parse_compare_fn) (td, ar_buf[0], ar_buf[1]);
        }

        item.idx_page = i;
        item.err_code = -ESYS_MAP_DIFFERENT;
        goto l_end;
    }

    ret = true;

  l_end:
    if (perr_msg && item.err_code) {
        memcpy(&perr_msg->ar_item[perr_msg->cn_err++], &item,
               sizeof(sftl_err_item_t));
    }

    for (i = 0; i < cn_eu; i++) {
        if (ar_buf[i]) {
            track_kfree(ar_buf[i], HARD_PAGE_SIZE, M_UNKNOWN);
        }

        if (buffer[i]) {
            track_kfree(buffer[i], SZ_SPAREAREA * FPAGE_PER_SEU, M_UNKNOWN);
        }
    }

    return ret;
}

/************ following function used for debug purpose ************/

// Tifa: this function is used when debug, no need get lock
bool bioc_list_search(lfsm_dev_t * td, struct list_head *plist,
                      struct bio_container *pbioc_target)
{
    struct bio_container *pbioc_cur, *pbioc_next;
    bool ret;

    ret = true;
//    spin_lock_irqsave (&td->datalog_list_spinlock,flag);
    list_for_each_entry_safe(pbioc_cur, pbioc_next, plist, list) {
        if (pbioc_target == pbioc_cur) {
            goto l_end;
        }
    }

    ret = false;

  l_end:
//    spin_unlock_irqrestore (&td->datalog_list_spinlock,flag);
    return ret;
}

int32_t bioc_dump_missing(lfsm_dev_t * td)
{
    struct bio_container *pbioc;
    int32_t i;

    for (i = 0; i < td->freedata_bio_capacity + CN_EXTRA_BIOC; i++) {
        pbioc = gar_pbioc[i];
        if (!bioc_list_search(td, &td->datalog_free_list[0], pbioc)) {
            LOGINFO("Missing bioc(%p) lpn %llu (0x%llx)\n", pbioc,
                    pbioc->start_lbno, pbioc->start_lbno);
        }
    }
    return 0;
}
