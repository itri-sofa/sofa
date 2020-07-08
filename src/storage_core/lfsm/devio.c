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
#include <scsi/scsi.h>
#include <scsi/sg.h>
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
#include "sysapi_linux_kernel.h"
#include "devio.h"
#include "special_cmd.h"
#include "io_manager/io_common.h"
#include "metabioc.h"
#include "EU_access.h"
#include "spare_bmt_ul.h"

#include "bio_ctrl.h"
#include "devio_private.h"

void init_devio_cb(devio_fn_t * cb)
{
    init_waitqueue_head(&cb->io_queue);
    atomic_set(&cb->result, DEVIO_RESULT_FAILURE);
    cb->id_disk = -1;
    cb->rq = NULL;
    cb->pBio = NULL;
}

int32_t devio_make_request(struct bio *bio, bool isDirectMakeReq)
{
    struct blk_plug plug;
    int32_t i, sum;

    LFSM_ASSERT(bio->bi_iter.bi_size > 0);
    LFSM_ASSERT(bio->bi_vcnt > 0);

    if ((lfsm_readonly == 1) && (bio_data_dir(bio) == WRITE) &&
        (bio->bi_iter.bi_sector & LFSM_BREAD_BIT) == 0) {
        LOGERR("Can't support write cmd when readonly mode\n");
        dump_stack();
        return -ENOEXEC;
    }

    if (bio_flagged(bio, BIO_SEG_VALID)) {
        LOGWARN("bio %p %llx bi_flags=%x seg=%d\n", bio,
                (sector64) bio->bi_iter.bi_sector, bio->bi_flags,
                bio->bi_phys_segments);
        dump_stack();
    }

    sum = 0;
    for (i = 0; i < bio->bi_vcnt; i++) {
        if (bio->bi_io_vec[i].bv_offset != 0) {
            LOGWARN("bv_offset!=0 bio %p %llx bi_flags=%x seg=%d\n",
                    bio, (sector64) bio->bi_iter.bi_sector, bio->bi_flags,
                    bio->bi_phys_segments);
            LOGWARN("iovec %d,  %d, %d bi_size %d\n", i,
                    bio->bi_io_vec[i].bv_len, bio->bi_io_vec[i].bv_offset,
                    bio->bi_iter.bi_size);
        }
        sum += bio->bi_io_vec[i].bv_len;
    }

    if ((sum != bio->bi_iter.bi_size) && !IS_TRIM_BIO(bio)) {
        dump_stack();
        LOGWARN("bv_len wrong bio %p %llx bi_flags=%x seg=%d\n",
                bio, (sector64) bio->bi_iter.bi_sector, bio->bi_flags,
                bio->bi_phys_segments);
        for (i = 0; i < bio->bi_vcnt; i++) {
            LOGWARN("iovec %d,  %d, %d bi_size %d\n", i,
                    bio->bi_io_vec[i].bv_len, bio->bi_io_vec[i].bv_offset,
                    bio->bi_iter.bi_size);
        }
    }

    if (!isDirectMakeReq) {
        bio->bi_opf = REQ_OP_READ;
        submit_bio(bio);
    } else {
        blk_start_plug(&plug);
        generic_make_request(bio);
        blk_finish_plug(&plug);
    }

    return 0;
}

static void _devio_wait(devio_fn_t * ret, const char *caller)
{
    int32_t cnt, wait_ret;

    cnt = 0;
    while (true) {
        cnt++;
        wait_ret = wait_event_interruptible_timeout(ret->io_queue,
                                                    atomic_read(&ret->result) !=
                                                    DEVIO_RESULT_WORKING,
                                                    LFSM_WAIT_EVEN_TIMEOUT);
        if (wait_ret <= 0) {
            LOGWARN("devio IO no response after seconds %d, caller=%s\n",
                    LFSM_WAIT_EVEN_TIMEOUT_SEC * cnt, caller);
        } else {
            break;
        }
    }
}

bool devio_specwrite(int32_t id_disk, int32_t type, void *pcmd)
{
    devio_fn_t ret;
    struct bio *pBio;
    int32_t cn_retry;
    struct block_device *bdev;

    cn_retry = 0;
    init_waitqueue_head(&ret.io_queue);
    atomic_set(&ret.result, DEVIO_RESULT_WORKING);

    while (NULL == (pBio = track_bio_alloc(__GFP_RECLAIM | __GFP_ZERO, 1))) {   //bi_vec_size=1 page
        LOGERR("allocate page_bio fails in compose_bio\n");
        schedule();
        if (cn_retry++ > 10) {
            return false;
        }
    }

    // FUNK ME: should not use lfsmdev below
    if (compose_special_command_bio(&grp_ssds, type, pcmd, pBio) != 0) {
        goto l_fail;
    }

    pBio->bi_opf = REQ_OP_WRITE;
    pBio->bi_next = NULL;
    pBio->bi_private = &ret;
    pBio->bi_end_io = end_bio_devio_general;    //function pointer

    if (NULL == (bdev = diskgrp_get_drive(&grp_ssds, id_disk))) {   //pgrp->ar_drives[dev_id].bdev;
        goto l_fail;
    }

    bio_set_dev(pBio, bdev);
    devio_make_request(pBio, true); // always return 0
    diskgrp_put_drive(&grp_ssds, id_disk);

    //lfsm_wait_event_interruptible(ret.io_queue,
    //        atomic_read(&ret.result) != DEVIO_RESULT_WORKING); // find bed to wake him and to see erase is not DEVIO_RESULT_WORKING
    _devio_wait(&ret, __func__);

    if (atomic_read(&ret.result) != DEVIO_RESULT_SUCCESS) {
        goto l_fail;
    }

    free_composed_bio(pBio, pBio->bi_vcnt);
    return true;

  l_fail:
    free_composed_bio(pBio, pBio->bi_vcnt);
    return false;
}

static void end_bio_generic(devio_fn_t * pRet, blk_status_t err)
{
    if (err) {
        atomic_set(&pRet->result, DEVIO_RESULT_FAILURE);
    } else {
        atomic_set(&pRet->result, DEVIO_RESULT_SUCCESS);
    }
    wake_up_interruptible(&pRet->io_queue);
    return;
}

#if LFSM_ENABLE_TRIM == 1  || LFSM_ENABLE_TRIM ==4
static void end_bio_devio_trim(struct request *rq, blk_status_t err)
{
    end_bio_generic(rq->end_io_data, err);
    wake_up_interruptible(&lfsmdev.ersmgr.wq_item);
    return;
}
#endif

#if LFSM_ENABLE_TRIM == 2 || LFSM_ENABLE_TRIM == 3
static void end_bio_devio_erase(struct bio *bio)
{
    end_bio_generic(bio->bi_private, bio->bi_status);
    wake_up_interruptible(&lfsmdev.ersmgr.wq_item);
    return;
}
#endif

void end_bio_devio_general(struct bio *bio)
{
    end_bio_generic(bio->bi_private, bio->bi_status);
}

// op==0 -> copy user buffer contents into bio page (WRITE)
// op==1 -> copy bio page back to user buffer (READ)
// op==2 -> clean bio_page as erase (ERASE)
bool bio_vec_data_rw(struct bio_vec *page_vec, int8_t * buffer, int32_t op)
{
    int8_t *addr;
    int32_t i;

    for (i = 0; i < MEM_PER_FLASH_PAGE; i++) {
        addr = kmap(page_vec->bv_page);
        LFSM_ASSERT(addr != 0);
        switch (op) {
        case 0:
            memcpy(addr, buffer + (i * PAGE_SIZE), PAGE_SIZE);
            break;
        case 1:
            memcpy(buffer + (i * PAGE_SIZE), addr, PAGE_SIZE);
            break;
        case 2:
            memset(addr, 0, PAGE_SIZE);
            break;
        default:
            LOGERR("%s: unexpected op %d\n", __FUNCTION__, op);
            break;
        }
        kunmap(page_vec->bv_page);
        page_vec++;
    }
    return true;
}

bool bio_vec_hpage_rw(struct bio_vec *page_vec, int8_t * buffer, int32_t op)
{
    int8_t *addr;
    int32_t i;

    for (i = 0; i < MEM_PER_HARD_PAGE; i++) {
        addr = kmap(page_vec->bv_page);
        LFSM_ASSERT(addr != 0);
        switch (op) {
        case 0:
            memcpy(addr, buffer + (i * PAGE_SIZE), PAGE_SIZE);
            break;
        case 1:
            memcpy(buffer + (i * PAGE_SIZE), addr, PAGE_SIZE);
            break;
        case 2:
            memset(addr, 0, PAGE_SIZE);
            break;
        default:
            LOGERR("%s: unexpected op %d\n", __FUNCTION__, op);
            break;
        }
        kunmap(page_vec->bv_page);
        page_vec++;
    }
    return true;
}

int32_t devio_write_pages_safe(sector64 addr, int8_t * buffer,
                               sector64 * ar_spare_data, int32_t cn_flash_pages)
{
    devio_fn_t *pCb;
    int32_t j, i, ret, cn_batchs, cn_write, idx_fpage;

    cn_batchs = LFSM_CEIL_DIV(cn_flash_pages, CN_FPAGE_PER_BATCH);
    ret = 0;
    if (NULL == (pCb = track_kmalloc(CN_FPAGE_PER_BATCH * sizeof(devio_fn_t),
                                     GFP_KERNEL | __GFP_ZERO, M_UNKNOWN))) {
        LOGERR("Cannot alloc memory \n");
        return -ENOMEM;
    }

    for (i = 0; i < cn_batchs; i++) {
        memset(pCb, 0, CN_FPAGE_PER_BATCH * sizeof(devio_fn_t));
        cn_write = CN_FPAGE_PER_BATCH;
        if (i == cn_batchs - 1 && (cn_flash_pages % CN_FPAGE_PER_BATCH) != 0) {
            cn_write = (cn_flash_pages % CN_FPAGE_PER_BATCH);   // handling the special case
        }

        for (j = 0; j < cn_write; j++) {
            idx_fpage = (i * CN_FPAGE_PER_BATCH + j);
            if (!devio_write_page(addr + idx_fpage * SECTORS_PER_FLASH_PAGE,
                                  buffer + idx_fpage * FLASH_PAGE_SIZE, 0,
                                  &pCb[j])) {
                LOGWARN("pbno %llu fpag submit write fail \n",
                        addr + idx_fpage * SECTORS_PER_FLASH_PAGE);
                ret = -EIO;
                break;
            }
        }

        for (j = 0; j < cn_write; j++) {
            if (NULL == pCb[j].pBio) {
                continue;
            }

            if (!devio_async_write_bh(&pCb[j])) {
                LOGWARN("pbno %llu fpag write fail \n",
                        addr + (i * CN_FPAGE_PER_BATCH +
                                j) * SECTORS_PER_FLASH_PAGE);
                ret = -EIO;
            }
        }

        if (ret < 0) {
            break;
        }
    }
    track_kfree(pCb, CN_FPAGE_PER_BATCH * sizeof(devio_fn_t), M_UNKNOWN);
    return ret;
}

bool devio_write_pages(sector64 addr, int8_t * buffer,
                       sector64 * ar_spare_data, int32_t cn_hard_pages)
{
    devio_fn_t ret;
    devio_fn_t *pret;
    struct bio *pBio;
    int32_t i;

    pret = &ret;
    init_waitqueue_head(&pret->io_queue);
    atomic_set(&pret->result, DEVIO_RESULT_WORKING);

    if (compose_bio(&pBio, NULL, end_bio_devio_general, pret,
                    cn_hard_pages * MEM_PER_HARD_PAGE * PAGE_SIZE,
                    cn_hard_pages * MEM_PER_HARD_PAGE) != 0) {
        return false;
    }

    pBio->bi_opf = REQ_OP_WRITE;
    pBio->bi_next = NULL;
    pBio->bi_iter.bi_sector = addr;
    //pReadData = kmap(pBio->bi_io_vec[0].bv_page);
    //memcpy(pReadData,buffer ,PAGE_SIZE);
    //kunmap(pBio->bi_io_vec[0].bv_page);
    for (i = 0; i < cn_hard_pages * MEM_PER_HARD_PAGE; i++) {
        if ((i % MEM_PER_HARD_PAGE) == 0) {
            if (NULL == buffer) {
                bio_vec_hpage_rw(&(pBio->bi_io_vec[i]), 0, 2);  // clean bio_vec page to zero
            } else {
                bio_vec_hpage_rw(&(pBio->bi_io_vec[i]), buffer + (i * PAGE_SIZE), 0);   // FROM BUFFER TO BI_IO_VEC
            }
        }
    }

    if (my_make_request(pBio) == 0) {
        //lfsm_wait_event_interruptible(pret->io_queue,
        //        atomic_read(&pret->result) != DEVIO_RESULT_WORKING);
        _devio_wait(pret, __func__);
    }

    if (atomic_read(&pret->result) != DEVIO_RESULT_SUCCESS) {
        free_composed_bio(pBio, pBio->bi_vcnt);
        return false;
    }

    free_composed_bio(pBio, pBio->bi_vcnt);
    return true;

}

// spare_data should be 8 byte long with 6 byte meaningful data
// -1 means no spare_data attached
// buffer==0 means write zero-page to dev
bool devio_write_page(sector64 addr, int8_t * buffer, sector64 spare_data,
                      devio_fn_t * pCb)
{
    devio_fn_t ret;
    devio_fn_t *pret;
    struct bio *pBio;

    if (NULL == pCb) {
        pret = &ret;
    } else {
        pret = pCb;
    }

    pret->pBio = NULL;
    init_waitqueue_head(&pret->io_queue);
    atomic_set(&pret->result, DEVIO_RESULT_WORKING);

    if (compose_bio(&pBio, NULL, end_bio_devio_general, pret,
                    MEM_PER_FLASH_PAGE * PAGE_SIZE, MEM_PER_FLASH_PAGE) != 0) {
        return false;
    }

    pBio->bi_opf = REQ_OP_WRITE;
    pBio->bi_next = NULL;
    pBio->bi_iter.bi_sector = addr;
    //pReadData = kmap(pBio->bi_io_vec[0].bv_page);
    //memcpy(pReadData,buffer ,PAGE_SIZE);
    //kunmap(pBio->bi_io_vec[0].bv_page);
    if (NULL == buffer) {
        bio_vec_data_rw(&(pBio->bi_io_vec[0]), 0, 2);   // clean bio_vec page to zero
    } else {
        bio_vec_data_rw(&(pBio->bi_io_vec[0]), buffer, 0);  // FROM BUFFER TO BI_IO_VEC
    }

    if (my_make_request(pBio) == 0) {
        if (pCb) {
            pret->pBio = pBio;
            return true;
        }
        //lfsm_wait_event_interruptible(pret->io_queue,
        //        atomic_read(&pret->result) != DEVIO_RESULT_WORKING);
        _devio_wait(pret, __func__);
    }

    if (atomic_read(&pret->result) != DEVIO_RESULT_SUCCESS) {
        free_composed_bio(pBio, pBio->bi_vcnt);
        return false;
    }

    free_composed_bio(pBio, pBio->bi_vcnt);
    return true;
}

//size is byte
bool devio_query_lpn(int8_t * buffer, sector64 addr, int32_t size)
{
    if (mdnode_get_metadata(buffer, addr, size) < 0) {
        return false;
    } else {
        return true;
    }
}

// read io
// Author: tifa
bool devio_raw_rw_page(sector64 addr, struct block_device *bdev,
                       int8_t * buffer, int32_t hpage_len, uint64_t rw)
{
    struct bio *pBio;
    devio_fn_t cb;
    int32_t i;

    init_waitqueue_head(&cb.io_queue);

    if (compose_bio(&pBio, bdev, end_bio_devio_general, &cb,
                    hpage_len * HARD_PAGE_SIZE,
                    hpage_len * MEM_PER_HARD_PAGE) < 0) {
        return false;
    }

    pBio->bi_opf = rw;
    pBio->bi_next = NULL;
    pBio->bi_iter.bi_sector = addr;
    //printk("super map pre_read get record id: device(%p) addr %llu size %d rw %ld\n",pBio->bi_disk,pBio->bi_iter.bi_sector,pBio->bi_iter.bi_size,pBio->bi_opf);
    atomic_set(&cb.result, DEVIO_RESULT_WORKING);
    devio_make_request(pBio, true);
    //lfsm_wait_event_interruptible(cb.io_queue,
    //        atomic_read(&cb.result) != DEVIO_RESULT_WORKING);
    _devio_wait(&cb, __func__);

    for (i = 0; i < hpage_len; i++) {
        if (NULL == buffer) {
            bio_vec_hpage_rw(&(pBio->bi_io_vec[i * MEM_PER_HARD_PAGE]), buffer + i * HARD_PAGE_SIZE, 2);    // fill int zero
        } else {
            bio_vec_hpage_rw(&(pBio->bi_io_vec[i * MEM_PER_HARD_PAGE]),
                             buffer + i * HARD_PAGE_SIZE, 1);
        }
    }

    if (atomic_read(&cb.result) != DEVIO_RESULT_SUCCESS) {
        LOGERR("read super map of disk %s sector 0\n",
               grp_ssds.ar_drives[BiSectorAdjust_bdev2lfsm(pBio)].nm_dev);
        free_composed_bio(pBio, pBio->bi_vcnt);
        return false;
    }

    free_composed_bio(pBio, pBio->bi_vcnt);
    return true;
}

bool devio_reread_pages_after_fail(sector64 pbno, int8_t * buffer,
                                   int32_t cn_fpage)
{
    struct bad_block_info bad_blk_info;
    special_read_enquiry(pbno, cn_fpage, 0, 0, &bad_blk_info);
    return devio_reread_pages(pbno, buffer, cn_fpage);
}

bool devio_reread_pages(sector64 pbno, int8_t * buffer, int32_t cn_fpage)
{
    struct bad_block_info bad_blk_info;
    int32_t cn_retry;

    cn_retry = 0;
    while (!devio_read_pages(pbno, buffer, cn_fpage, NULL)) {
        if (special_read_enquiry(pbno, cn_fpage, 0, 0, &bad_blk_info) != 0) {
            return false;
        }

        if (cn_retry >= LFSM_MIN_RETRY) {
            return false;
        }
        cn_retry++;
    }

    return true;
}

int32_t devio_async_read_hpages(sector64 pbno, int32_t cn_hpage,
                                devio_fn_t * pcb)
{
    struct bio *pBio;
    init_waitqueue_head(&pcb->io_queue);
    atomic_set(&pcb->result, DEVIO_RESULT_WORKING);

    if (compose_bio(&pBio, NULL, end_bio_devio_general, pcb,
                    cn_hpage * HARD_PAGE_SIZE,
                    cn_hpage * MEM_PER_HARD_PAGE) != 0) {
        return -ENOMEM;
    }

    pBio->bi_opf = REQ_OP_READ;
    pBio->bi_next = NULL;
    pBio->bi_iter.bi_sector = pbno;

    pBio->bi_seg_front_size = 0;
    pBio->bi_seg_back_size = 0;
    pBio->bi_status = BLK_STS_OK;
    pBio->bi_flags &= ~(1 << BIO_SEG_VALID);

    pcb->pBio = pBio;
    return my_make_request(pBio);
}

bool devio_async_read_hpages_bh(int32_t cn_hpages, int8_t * buffer,
                                devio_fn_t * pcb)
{
    struct bio *pBio;
    int32_t i;

    //lfsm_wait_event_interruptible(pcb->io_queue,
    //        atomic_read(&pcb->result) != DEVIO_RESULT_WORKING);
    _devio_wait(pcb, __func__);

    pBio = pcb->pBio;
    if (atomic_read(&pcb->result) != DEVIO_RESULT_SUCCESS) {
        LOGERR("%s: %p got a wrong bio %p result=%d\n", __FUNCTION__,
               pcb, pBio, atomic_read(&pcb->result));
        //free_composed_bio(pBio,pBio->bi_vcnt);
        return false;
    }

    for (i = 0; i < cn_hpages; i++) {
        bio_vec_data_rw(&(pBio->bi_io_vec[i * MEM_PER_HARD_PAGE]),
                        buffer + (i * HARD_PAGE_SIZE), 1);
    }

    free_composed_bio(pBio, pBio->bi_vcnt);
    return true;
}

int32_t devio_async_read_pages(sector64 pbno, int32_t cn_fpage,
                               devio_fn_t * pcb)
{
    struct bio *pBio;

    init_waitqueue_head(&pcb->io_queue);
    atomic_set(&pcb->result, DEVIO_RESULT_WORKING);

    if (compose_bio(&pBio, NULL, end_bio_devio_general, pcb,
                    cn_fpage * FLASH_PAGE_SIZE,
                    cn_fpage * MEM_PER_FLASH_PAGE) != 0) {
        return -ENOMEM;
    }
    pBio->bi_opf = REQ_OP_READ;
    pBio->bi_next = NULL;
    pBio->bi_iter.bi_sector = pbno;

    pBio->bi_seg_front_size = 0;
    pBio->bi_seg_back_size = 0;
    pBio->bi_status = BLK_STS_OK;
    pBio->bi_flags &= ~(1 << BIO_SEG_VALID);

    pcb->pBio = pBio;
    return my_make_request(pBio);
}

bool devio_async_read_pages_bh(int32_t cn_fpages, int8_t * buffer,
                               devio_fn_t * pcb)
{
    struct bio *pBio;
    int32_t i;

    //lfsm_wait_event_interruptible(pcb->io_queue,
    //        atomic_read(&pcb->result) != DEVIO_RESULT_WORKING);
    _devio_wait(pcb, __func__);
    pBio = pcb->pBio;
    if (atomic_read(&pcb->result) != DEVIO_RESULT_SUCCESS) {
        LOGERR("%p got a wrong bio %p result=%d\n", pcb, pBio,
               atomic_read(&pcb->result));
        free_composed_bio(pBio, pBio->bi_vcnt);
        return false;
    }

    for (i = 0; i < cn_fpages; i++) {
        bio_vec_data_rw(&(pBio->bi_io_vec[i * MEM_PER_FLASH_PAGE]),
                        buffer + (i * FLASH_PAGE_SIZE), 1);
    }

    free_composed_bio(pBio, pBio->bi_vcnt);
    return true;
}

bool devio_read_hpages(sector64 pbno_in_sects, int8_t * buffer,
                       int32_t cn_hpage)
{
    devio_fn_t ret;
    struct bio *pBio;
    int32_t i;

    init_waitqueue_head(&ret.io_queue);
    atomic_set(&ret.result, DEVIO_RESULT_WORKING);

    if (compose_bio(&pBio, NULL, end_bio_devio_general, &ret,
                    cn_hpage * HARD_PAGE_SIZE,
                    cn_hpage * MEM_PER_HARD_PAGE) != 0) {
        return false;
    }

    pBio->bi_opf = REQ_OP_READ;
    pBio->bi_next = NULL;
    pBio->bi_iter.bi_sector = pbno_in_sects;
    pBio->bi_seg_front_size = 0;
    pBio->bi_seg_back_size = 0;
    pBio->bi_next = NULL;
    pBio->bi_status = BLK_STS_OK;
    pBio->bi_flags &= ~(1 << BIO_SEG_VALID);

    if (my_make_request(pBio) == 0) {
        //lfsm_wait_event_interruptible(ret.io_queue,
        //        atomic_read(&ret.result) != DEVIO_RESULT_WORKING);
        _devio_wait(&ret, __func__);
    }

    if (atomic_read(&ret.result) != DEVIO_RESULT_SUCCESS) {
        free_composed_bio(pBio, pBio->bi_vcnt);
        return false;
    }

    for (i = 0; i < cn_hpage; i++) {
        bio_vec_hpage_rw(&(pBio->bi_io_vec[i * MEM_PER_HARD_PAGE]),
                         buffer + (i * HARD_PAGE_SIZE), 1);
    }

    free_composed_bio(pBio, pBio->bi_vcnt);
    return true;
}

bool devio_read_hpages_batch_bh(devio_fn_t * pCb, int8_t * buffer, int32_t cn_fpage)    // Peter added for reading data in batch.
{
    bool ret;

    ret = true;
    /** LOGINFO("devio_asysnc_read_bh cpu=%d  dev=%s sector=%llu \n", smp_processor_id() , pCb->pBio->bi_disk->disk_name , (long long unsigned) pCb->pBio->bi_iter.bi_sector ); **/
    _devio_wait(pCb, __func__);

    if (atomic_read(&pCb->result) != DEVIO_RESULT_SUCCESS) {
        LOGERR("hpages_batch read error %d\n", atomic_read(&pCb->result));
        ret = false;
    }

    if (ret) {
        int32_t i = 0;
        for (i = 0; i < cn_fpage; i++) {
           /** LOGINFO(" i*MEM_PER_FLASH_PAGE=%u, MEM_PER_FLASH_PAGE=%u , (i*FLASH_PAGE_SIZE)=%u , FLASH_PAGE_SIZE=%u \n",i*MEM_PER_FLASH_PAGE, MEM_PER_FLASH_PAGE,(i*FLASH_PAGE_SIZE) ,FLASH_PAGE_SIZE);  **/
            bio_vec_data_rw(&(pCb->pBio->bi_io_vec[i * MEM_PER_FLASH_PAGE]),
                            buffer + (i * FLASH_PAGE_SIZE), 1);
        }
    }

    free_composed_bio(pCb->pBio, pCb->pBio->bi_vcnt);
    pCb->pBio = NULL;
    return ret;
}

// Peter added for NVME read issues like:
// // ERR: (1) blk_update_request: I/O error, dev nvme0n1, sector 2883584
// //      (2) bio too big device nvme ( 1024 > 256)
// // The problem is due to the fact that trasmitting data size is beyond than nvme hw defined.  /sys/block/nvme0n1/queue/max_hw_sectors_kb
// // For,if 128 is defined, So 128k / 4k = 32, which means you only can send 32 pages to nvme device each time.

bool devio_read_hpages_batch(sector64 pbno_in_sects, int8_t * buffer,
                             int32_t cn_hpage)
{
    devio_fn_t *pCb;

    diskgrp_t *pgrp;
    int32_t id_dev;
    subdisk_info_t *psdi;

    int32_t ret;
    sector64 ppn;
    struct block_device *bdev;
    int32_t rounds;
    int32_t i;
    int32_t cn, last;

    int32_t max_sectors_kb = 0;
    int32_t page_cn = 0;

    pgrp = &grp_ssds;

    if (!pbno_lfsm2bdev_A(pbno_in_sects, (sector64 *) (&ppn), &id_dev, pgrp)) {
        LOGINFO("lpn=%llu get PPN or devID failed in hpages_batch. \n",
                (long long unsigned)pbno_in_sects);
        return false;
    }

    psdi = &pgrp->ar_drives[id_dev];
    if (psdi->actived == 0) {
        LOGINFO("id_dev=%d get block_device failed. \n", id_dev);
        return false;
    } else {
        bdev = psdi->bdev;
    }

    max_sectors_kb = bdev->bd_disk->queue->limits.max_sectors >> 1;
    page_cn = max_sectors_kb >> 2;

    if (page_cn >= cn_hpage) {
        LOGINFO
            ("## Do devio_read_hpages() without batch, cn_hpage=%d, dev page_cn=%d dev=%s\n",
             cn_hpage, page_cn, bdev->bd_disk->disk_name);
        return devio_read_hpages(pbno_in_sects, buffer, cn_hpage);
    }
    //LOGINFO("## Do devio_read_hpages_batch(), cn_hpage=%d, dev page_cn=%d, dev=%s\n",cn_hpage, page_cn,  bdev->bd_disk->disk_name );
    rounds = LFSM_CEIL_DIV(cn_hpage, page_cn);

    if (NULL == (pCb = track_kmalloc(rounds * sizeof(devio_fn_t),
                                     GFP_KERNEL | __GFP_ZERO, M_UNKNOWN))) {
        LOGERR("Cannot alloc memory in hpages_batch. \n");
        return false;
    }
    ret = 0;
    last = cn_hpage & (page_cn - 1);  /**  last = cn_hpage % page_cn; **/
    if (last == 0)
        last = page_cn;

    cn = page_cn;
    for (i = 0; i < rounds; i++) {
        if (i + 1 == rounds)
            cn = last;
        //LOGINFO("hpages_batch ## i=%d, rounds=%d, lpn=%llu, buf_loc=%d ,cn=%d ,page_cn=%d \n", i ,rounds ,(long long unsigned) pbno_in_sects + (i* page_cn)* SECTORS_PER_FLASH_PAGE, (i*page_cn) * FLASH_PAGE_SIZE , cn, page_cn );
        if (!devio_read_pages
            (pbno_in_sects + (i * page_cn) * SECTORS_PER_FLASH_PAGE,
             buffer + (i * page_cn) * FLASH_PAGE_SIZE, cn, &pCb[i])) {
            LOGWARN("hpages_batch pbno %llu fpag submit read fail \n",
                    pbno_in_sects + (i * page_cn) * SECTORS_PER_FLASH_PAGE);
            ret = -EIO;
            break;
        }
    }

    cn = page_cn;
    for (i = 0; i < rounds; i++) {
        if (NULL == pCb[i].pBio) {
            continue;
        }
        if (i + 1 == rounds)
            cn = last;
         /**  if any error, we treat all transaction failed.  **/
        //LOGINFO("hpages_batch @@ i=%d, rounds=%d, buf_loc=%d, cn=%d \n", i,rounds,  (i* page_cn)* FLASH_PAGE_SIZE, cn );
        if (!devio_read_hpages_batch_bh
            (&pCb[i], buffer + (i * page_cn) * FLASH_PAGE_SIZE,
             (ret < 0) ? 0 : cn)) {
            LOGWARN("hpages_batch pbno %llu fpag read fail \n",
                    pbno_in_sects + (i * page_cn) * SECTORS_PER_FLASH_PAGE);
            ret = -EIO;
        }
    }

    track_kfree(pCb, rounds * sizeof(devio_fn_t), M_UNKNOWN);

    return (ret == 0) ? true : false;
}

bool devio_read_pages(sector64 pbno, int8_t * buffer, int32_t cn_fpage,
                      devio_fn_t * pCb)
{
    devio_fn_t ret;
    struct bio *pBio;
    int32_t i;

    devio_fn_t *pret;
    if (NULL == pCb) {
        pret = &ret;
    } else {
        pret = pCb;
    }

    init_waitqueue_head(&pret->io_queue);
    atomic_set(&pret->result, DEVIO_RESULT_WORKING);

    if (compose_bio(&pBio, NULL, end_bio_devio_general, pret,
                    cn_fpage * FLASH_PAGE_SIZE,
                    cn_fpage * MEM_PER_FLASH_PAGE) != 0) {
        return false;
    }

    pBio->bi_opf = REQ_OP_READ;
    pBio->bi_next = NULL;
    pBio->bi_iter.bi_sector = pbno;
    pBio->bi_seg_front_size = 0;
    pBio->bi_seg_back_size = 0;
    pBio->bi_next = NULL;
    pBio->bi_status = BLK_STS_OK;
    pBio->bi_flags &= ~(1 << BIO_SEG_VALID);

    if (my_make_request(pBio) == 0) {
        if (pCb) {
            pret->pBio = pBio;
            return true;
        }
        //lfsm_wait_event_interruptible(ret.io_queue,
        //        atomic_read(&ret.result) != DEVIO_RESULT_WORKING);
        _devio_wait(pret, __func__);
    }

    if (atomic_read(&pret->result) != DEVIO_RESULT_SUCCESS) {
        free_composed_bio(pBio, pBio->bi_vcnt);
        return false;
    }

    for (i = 0; i < cn_fpage; i++) {
        bio_vec_data_rw(&(pBio->bi_io_vec[i * MEM_PER_FLASH_PAGE]),
                        buffer + (i * FLASH_PAGE_SIZE), 1);
    }

    free_composed_bio(pBio, pBio->bi_vcnt);
    return true;
}

int32_t devio_compare(struct bio_vec *iovec, sector64 tar_pbno)
{
    uint8_t *s_data, *d_data;
    int32_t ret, i;

    if (NULL == (d_data = (int8_t *)
                 track_kmalloc(FLASH_PAGE_SIZE, __GFP_ZERO, M_UNKNOWN))) {
        ret = LFSM_ERR_WR_ERROR;
        LOGERR("alloc buffer fail\n");
        goto l_end2;
    }

    err_handle_printk("enter check ssd contect target sector %llu\n", tar_pbno);
    // Later run this code in a loop for bi_size/page_size times
    if (!devio_read_pages(tar_pbno, d_data, 1, NULL)) {
        ret = LFSM_ERR_WR_ERROR;
        LOGERR("reread 4k page fail\n");
        goto l_end;
    }

    s_data = kmap(iovec->bv_page);
    err_handle_printk("dump source data\n");
    err_handle_printk("dump dest data\n");
    if (memcmp(s_data, d_data, PAGE_SIZE) == 0) {
        ret = LFSM_NO_FAIL;
        goto l_kmapped;
    }

    ret = LFSM_ERR_WR_RETRY;
    for (i = 0; i < PAGE_SIZE; i++) {
        if (d_data[i] != 0xff) {
            ret = LFSM_ERR_WR_ERROR;
            err_handle_printk("check ssd data position %d fail\n", i);
            hexdump(d_data + i, 16);
            break;
        }
    }

  l_kmapped:
    kunmap(iovec->bv_page);
  l_end:
    track_kfree(d_data, FLASH_PAGE_SIZE, M_UNKNOWN);
  l_end2:
    err_handle_printk("check ssd contect return code is %x\n", ret);
    return ret;
}

static int32_t devio_raw_erase_async(struct block_device *bdev, sector64 lsn,
                                     int32_t size_in_eus, devio_fn_t * pcb)
{
#if LFSM_ENABLE_TRIM == 1
    return devio_trim_async(bdev, lsn, size_in_eus, pcb, end_bio_devio_trim);
#elif LFSM_ENABLE_TRIM == 2     // ## Peter remind: SCSI with stack dump ERR in freeing bio_vec_page
    return devio_discard_async(bdev, lsn, size_in_eus, pcb,
                               end_bio_devio_erase);
#elif LFSM_ENABLE_TRIM == 3     // ## Peter For NVME allocate method and SATA by linux  default discard functions.
    return devio_discard_async_NVME(bdev, lsn, size_in_eus, pcb,
                                    end_bio_devio_erase);
#elif LFSM_ENABLE_TRIM == 4     // ## Peter For kvm trim failed in ssd, we implement sg_io unmap for scsi ssd.
    return devio_trim_by_sgio_unmap(bdev, lsn, size_in_eus, pcb,
                                    end_bio_devio_trim);
#else
    return 0;
#endif
}

/*
 * Description: erase data on disks
 * Parameter: lsn : the start address erase range, in sector unit.
 *                  logical addressing (0 : 0 of disk 0)
 *                                     (0 + disk size of disk 0 : 0 of disk 1)
 *            size_in_eus : length to erase
 */
int32_t devio_erase(sector64 lsn, int32_t size_in_eus,
                    struct block_device *bdev)
{
    devio_fn_t cb;
    int32_t max_discard_eus, sz_eus_per_round, ret;

    ret = 0;
    if (lfsm_readonly == 1) {
        return 0;
    }

    if ((ret = diskgrp_get_erase_upbound(lsn)) < 0) {
        return ret;
    }

    max_discard_eus =
        (ret >> (FLASH_PAGE_PER_BLOCK_ORDER + SECTORS_PER_FLASH_PAGE_ORDER));

    while (size_in_eus > 0) {
        init_devio_cb(&cb);

        sz_eus_per_round =
            (size_in_eus < max_discard_eus) ? size_in_eus : max_discard_eus;
        if (NULL == bdev) {
            devio_erase_async(lsn, sz_eus_per_round, &cb);  // the lsn is lfsm address
        } else {
            devio_raw_erase_async(bdev, lsn, sz_eus_per_round, &cb);    // the lsn is bdev address
        }

        // AN: no matter the return value, we must call bh
        ret = devio_erase_async_bh(&cb);
        if (ret < 0) {
            LOGERR("erase fail addr %llu round %d\n", lsn, sz_eus_per_round);
        }
        size_in_eus -= sz_eus_per_round;
        lsn +=
            (sz_eus_per_round <<
             (FLASH_PAGE_PER_BLOCK_ORDER + SECTORS_PER_FLASH_PAGE_ORDER));
    }

    return ret;
}

#if LFSM_ENABLE_TRIM == 1
static int32_t devio_trim_async_A(struct block_device *bdev,
                                  sector64 lba_trim, sector64 cn_sectors,
                                  devio_fn_t * pcb, rq_end_io_fn * rq_end_io)
{
    int8_t buf[SECTOR_SIZE], sense[SCSI_SENSE_BUFFERSIZE];
    struct bio *pbio;
    struct request_queue *q;
    struct request *rq;
    struct scsi_request *s_rq;
    sector64 range, lba, sz_trim;
    int32_t nsect, cn_segments;

    lba = 0;
    nsect = 1;
    cn_segments = 0;
    memset(buf, 0, sizeof(buf));
    while (cn_sectors > 0) {
        sz_trim =
            (cn_sectors > SZ_TRIM_SECTOR_MAX) ? SZ_TRIM_SECTOR_MAX : cn_sectors;
        range = (sz_trim << 48) | lba_trim;
        *(((uint64_t *) buf) + cn_segments) = __cpu_to_le64(range);

        lba_trim += sz_trim;
        cn_sectors -= SZ_TRIM_SECTOR_MAX;
        cn_segments++;
    }

    if (!bdev) {
        return -EPERM;
    }

    if (!bdev->bd_disk) {
        return -EPERM;
    }

    if (!bdev->bd_disk->queue) {
        return -EPERM;
    }

    q = bdev->bd_disk->queue;
    if (!q) {
        return -EPERM;
    }

    rq = blk_get_request(q, REQ_OP_SCSI_OUT, 0);
    if (IS_ERR(rq)) {
        LOGERR("blk_get_request error disk %s\n", bdev->bd_disk->disk_name);
        return -EPERM;
    }

    s_rq = scsi_req(rq);
    s_rq->cmd[15] = 0;
    s_rq->cmd[1] = SG_ATA_PROTO_DMA;
    s_rq->cmd[2] |=
        SG_CDB2_TLEN_NSECT | SG_CDB2_TLEN_SECTORS | SG_CDB2_TDIR_TO_DEV;
    s_rq->cmd[0] = SG_ATA_16;
    s_rq->cmd[4] = 1;           //tf->lob.feat;
    s_rq->cmd[6] = nsect;       //tf->lob.nsect;
    s_rq->cmd[8] = lba;         //tf->lob.lbal;   
    s_rq->cmd[10] = lba >> 8;   //tf->lob.lbam;
    s_rq->cmd[12] = lba >> 16;  //tf->lob.lbah;
    s_rq->cmd[13] = ATA_USING_LBA;  //tf->dev  
    s_rq->cmd[14] = ATA_OP_DSM; //tf->command;
    s_rq->cmd[1] |= SG_ATA_LBA48;
    s_rq->cmd[3] = 0;           //tf->hob.feat;
    s_rq->cmd[5] = nsect >> 8;  //tf->hob.nsect;
    s_rq->cmd[7] = lba >> 24;   //tf->hob.lbal;  
    s_rq->cmd[9] = lba >> 32;   //tf->hob.lbam;  
    s_rq->cmd[11] = lba >> 40;  //tf->hob.lbah;  
    s_rq->cmd_len = SG_ATA_16_LEN;

    rq->timeout = q->sg_timeout;

    atomic_set(&pcb->result, DEVIO_RESULT_WORKING);
    //pcb->pBio->bi_iter.bi_sector = (sector_t)lsn;
    rq->end_io = rq_end_io;
    rq->end_io_data = pcb;
    pcb->rq = rq;

    //printk("cdb[1] %p %x %x\n",rq->cmd,rq->cmd[1],cdb[1]);
    blk_rq_map_kern(q, rq, buf, 512, GFP_KERNEL);

    pbio = rq->bio;
    memset(sense, 0, sizeof(sense));
    s_rq->sense = sense;
    s_rq->sense_len = 0;
    s_rq->retries = 0;

    blk_execute_rq_nowait(q, bdev->bd_disk, rq, 0, rq->end_io);
    //blk_put_request(rq);

    //kfree(pbuf);
    return 0;
}

static int32_t devio_trim_async(struct block_device *bdev, sector64 lba_trim,
                                int32_t size_in_eus, devio_fn_t * pcb,
                                rq_end_io_fn * rq_end_io)
{
    sector64 cn_sectors;
    int32_t ret;

    cn_sectors = (sector64) (size_in_eus <<
                             (FLASH_PAGE_PER_BLOCK_ORDER +
                              SECTORS_PER_FLASH_PAGE_ORDER));

    if ((ret = devio_trim_async_A(bdev, lba_trim, cn_sectors, pcb, rq_end_io))
        < 0) {
        LOGERR("%s() trim_sector fail:, lsn=%llu, cn_sectors=%llu\n", __func__,
               lba_trim, cn_sectors);
    }

    //diskgrp_put_drive(pgrp, id_dev);    // move to devio_trim_async_bh
    return ret;
}
#endif

#if LFSM_ENABLE_TRIM == 2
static int32_t devio_discard_async(struct block_device *bdev,
                                   sector64 lba_discard, int32_t size_in_eus,
                                   devio_fn_t * pcb, bio_end_io_t * bi_end_io)
{
    struct bio *pBio;
    int32_t ret;

    ret = 0;
    if (compose_bio(&pBio, NULL, bi_end_io, pcb, SECTOR_SIZE, 1) != 0) {
        return -ENOMEM;
    }

    pBio->bi_iter.bi_sector = lba_discard;
    pBio->bi_opf |= (WRITE | DISCARD_BIO_BIRW_FLAG);
    pBio->bi_status = BLK_STS_OK;
    pBio->bi_next = NULL;
    pBio->bi_iter.bi_size = (size_in_eus <<
                             (FLASH_PAGE_PER_BLOCK_ORDER +
                              SECTORS_PER_FLASH_PAGE_ORDER + SECTOR_ORDER));
    bio_set_dev(pBio, bdev);
    pcb->pBio = pBio;
    atomic_set(&pcb->result, DEVIO_RESULT_WORKING);

    if ((ret = devio_make_request(pBio, true)) != 0) {
        free_composed_bio(pBio, pBio->bi_vcnt);
        return ret;
    }

    return ret;
}
#endif

#if LFSM_ENABLE_TRIM == 3       // ## Peter added for trim method by deallocation of nvme.
static int32_t devio_discard_async_NVME(struct block_device *bdev,
                                        sector64 lba_discard,
                                        int32_t size_in_eus, devio_fn_t * pcb,
                                        bio_end_io_t * bi_end_io)
{
    struct bio *pBio;
    int32_t ret = 0;
    struct request_queue *q = bdev_get_queue(bdev);
    int type = REQ_OP_WRITE | DISCARD_BIO_BIRW_FLAG;

    //LOGINFO("devio_discard_async_NVME() lba=%llu, sizeInbytes=%llu, sizeIn1M=%llu dev=%s \n", lba_discard, (size_in_eus <<(FLASH_PAGE_PER_BLOCK_ORDER+SECTORS_PER_FLASH_PAGE_ORDER+SECTOR_ORDER)) , size_in_eus,  bdev->bd_disk->disk_name  );

    if (!blk_queue_discard(q)) {
        LOGERR("%s does not provide discard queue for trim action.\n",
               bdev->bd_disk->disk_name);
        return -EPERM;
    }

    //pBio = bio_alloc(GFP_KERNEL , 1);
    if (NULL == (pBio = track_bio_alloc(__GFP_RECLAIM | __GFP_ZERO, 1))) {
        LOGERR("Bio allocate failed in trim action of nvme. \n");
        return -ENOMEM;
    }

    pBio->bi_private = pcb;

    pBio->bi_opf = type;
    pBio->bi_iter.bi_sector = lba_discard;
    pBio->bi_end_io = bi_end_io;
    pBio->bi_status = BLK_STS_OK;
    pBio->bi_next = NULL;
    pBio->bi_iter.bi_size =
        (size_in_eus <<
         (FLASH_PAGE_PER_BLOCK_ORDER + SECTORS_PER_FLASH_PAGE_ORDER +
          SECTOR_ORDER));
    bio_set_dev(pBio, bdev);

    pcb->pBio = pBio;
    atomic_set(&pcb->result, DEVIO_RESULT_WORKING);

    // submit_bio(type, pBio);
    if ((ret = devio_make_request(pBio, true)) != 0) {
        free_composed_bio(pBio, 0); // Key!!! For discard we don't need to alloc bi_io_vec ; otherwise, it will stack dump when freeing bi_io_vec.
        return ret;
    }

    return ret;
}
#endif

// TO little endian.  Peter added, I should switch it to macro for speeding up.
uint16_t _be_16(uint16_t x)
{
    return ((((x) >> 8) & 0xff) | (((x) & 0xff) << 8));
}

uint32_t _be_32(uint32_t x)
{
    return ((((x) & 0xff000000u) >> 24) | (((x) & 0x00ff0000u) >> 8)
            | (((x) & 0x0000ff00u) << 8) | (((x) & 0x000000ffu) << 24));
}

uint64_t _be_64(uint64_t x)
{
    return ((((x) & 0xff00000000000000ull) >> 56)
            | (((x) & 0x00ff000000000000ull) >> 40)
            | (((x) & 0x0000ff0000000000ull) >> 24)
            | (((x) & 0x000000ff00000000ull) >> 8)
            | (((x) & 0x00000000ff000000ull) << 8)
            | (((x) & 0x0000000000ff0000ull) << 24)
            | (((x) & 0x000000000000ff00ull) << 40)
            | (((x) & 0x00000000000000ffull) << 56));
}

#if LFSM_ENABLE_TRIM == 4       // ## Peter added trim by sgio_unmap of scsi ssd

static void compose_sgio_trim_data(uint8_t * param_arr, int32_t param_len,
                                   sector64 lba_trim, sector64 cn_sectors)
{
    uint64_t addr_arr = lba_trim;
    uint32_t num_arr = (uint32_t) cn_sectors;
    int32_t k = 8;
    int32_t num = 0;
    uint64_t tmp64 = 0;
    uint32_t tmp32 = 0;
    uint16_t tmp16 = 0;

    //LOGINFO("%s() param_len=%d lba_trim=%llu , cn_sectors=%llu \n", __func__, param_len , lba_trim ,cn_sectors  );

    k = 8;
    tmp64 = _be_64(addr_arr);
    memcpy(param_arr + k, &tmp64, 8);

    k += 8;
    tmp32 = _be_32(num_arr);
    memcpy(param_arr + k, &tmp32, 4);

    k = 0;
    num = param_len - 2;
    tmp16 = _be_16(num);
    memcpy(param_arr + k, &tmp16, 2);

    k += 2;
    num = param_len - 8;
    tmp16 = _be_16(num);
    memcpy(param_arr + k, &tmp16, 2);

/*  // for debugging only.
    char str[param_len];
    uint8_t * ptr = str;
    int32_t x=0;
    for( x=0;x<param_len;x++){
         LOGINFO("param[%d]=%02x\n", x, param_arr[x] );
         sprintf(ptr++, "%d", param_arr[x]);
    }
    LOGINFO("param=%s \n",str );
*/

}

static int32_t devio_trim_async_by_sgio_unmap(struct block_device *bdev,
                                              sector64 lba_trim,
                                              sector64 cn_sectors,
                                              devio_fn_t * pcb,
                                              rq_end_io_fn * rq_end_io)
{
    int8_t sense[SCSI_SENSE_BUFFERSIZE];
    struct bio *pbio;
    struct request_queue *q;
    struct request *rq;
    // sector64 range, lba, sz_trim;
    uint8_t unmap_cmd = 0x42;
    uint8_t cmd[10] = { unmap_cmd, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    int32_t param_len = 8 + (16 * 1);
    uint8_t param_arr[param_len];
    uint16_t tmp16 = 0;
    //int32_t x=0;

    memset(param_arr, 0, sizeof(param_arr));

    if (!bdev) {
        return -EPERM;
    }

    if (!bdev->bd_disk) {
        return -EPERM;
    }

    if (!bdev->bd_disk->queue) {
        return -EPERM;
    }

    q = bdev->bd_disk->queue;
    if (!q) {
        return -EPERM;
    }

    rq = blk_get_request(q, REQ_OP_SCSI_OUT, 0);
    if (IS_ERR(rq)) {
        return -EPERM;
    }

    // LOGINFO("%s() param_len=%d lba_trim=%llu , cn_sectors=%llu \n", __func__, param_len , lba_trim ,cn_sectors  );
    // compose sgio_unmap cmd.
    tmp16 = _be_16(param_len);
    memcpy(cmd + 7, &tmp16, 2);

    /*  // for debugging only. 
       x=0;
       for( x=0;x<sizeof(cmd);x++){
       LOGINFO("cmd[%d]=%02x\n",x , cmd[x]);
       }
     */

    memcpy(rq->cmd, cmd, sizeof(cmd));
    rq->cmd_len = sizeof(cmd);
    rq->cmd_type = REQ_TYPE_BLOCK_PC;
    rq->timeout = q->sg_timeout;

    atomic_set(&pcb->result, DEVIO_RESULT_WORKING);
    //pcb->pBio->bi_iter.bi_sector = (sector_t)lsn;
    rq->end_io = rq_end_io;
    rq->end_io_data = pcb;
    pcb->rq = rq;

    // compose sg_io unmap data in bio_pages
    memset(param_arr, 0, sizeof(param_arr));
    compose_sgio_trim_data(param_arr, sizeof(param_arr), lba_trim, cn_sectors);

    //printk("cdb[1] %p %x %x\n",rq->cmd,rq->cmd[1],cdb[1]);
    blk_rq_map_kern(q, rq, param_arr, sizeof(param_arr), GFP_KERNEL);

    pbio = rq->bio;
    memset(sense, 0, sizeof(sense));
    rq->sense = sense;
    rq->sense_len = 0;
    rq->retries = 0;

    blk_execute_rq_nowait(q, bdev->bd_disk, rq, 0, rq->end_io);
    //blk_put_request(rq);

    //kfree(pbuf);
    return 0;
}

static int32_t devio_trim_by_sgio_unmap(struct block_device *bdev,
                                        sector64 lba_trim, int32_t size_in_eus,
                                        devio_fn_t * pcb,
                                        rq_end_io_fn * rq_end_io)
{
    sector64 cn_sectors;
    int32_t ret;

    cn_sectors = (sector64) (size_in_eus <<
                             (FLASH_PAGE_PER_BLOCK_ORDER +
                              SECTORS_PER_FLASH_PAGE_ORDER));
    if ((ret =
         devio_trim_async_by_sgio_unmap(bdev, lba_trim, cn_sectors, pcb,
                                        rq_end_io))
        < 0) {
        LOGERR("%s() trim_sector fail:, lsn=%llu, cn_sectors=%llu\n", __func__,
               lba_trim, cn_sectors);
    }

    //diskgrp_put_drive(pgrp, id_dev);    // move to devio_trim_async_bh
    return ret;
}
#endif

static int32_t devio_get_smart_data(struct block_device *bdev,
                                    int8_t * pbuf_sector)
{
    int8_t sense[SCSI_SENSE_BUFFERSIZE];
    struct request_queue *q;
    struct request *rq;
    struct scsi_request *s_rq;
    int32_t ret;

    if (!bdev) {
        return -EPERM;
    }

    if (!bdev->bd_disk) {
        return -EPERM;
    }

    if (!bdev->bd_disk->queue) {
        return -EPERM;
    }

    q = bdev->bd_disk->queue;
    if (NULL == q) {
        return -EPERM;
    }

    rq = blk_get_request(q, REQ_OP_SCSI_IN, 0);

    if (IS_ERR(rq)) {
        return -EPERM;
    }

    s_rq = scsi_req(rq);
    memset(s_rq->cmd, 0, 16);
    s_rq->cmd[0] = SG_ATA_16;
    s_rq->cmd[1] = SG_ATA_PROTO_PIO_IN;
    s_rq->cmd[2] |=
        SG_CDB2_TLEN_NSECT | SG_CDB2_TLEN_SECTORS | SG_CDB2_TDIR_FROM_DEV;
    s_rq->cmd[4] = ATA_FEATURE_SMART_READ_DATA; //tf->lob.feat;        
    s_rq->cmd[10] = 0x4F;       //tf->lob.lbam;
    s_rq->cmd[12] = 0xC2;       //tf->lob.lbah; 
    s_rq->cmd[13] = 0;          //tf->dev
    s_rq->cmd[14] = ATA_OP_SMART;   //tf->command;
    s_rq->cmd_len = SG_ATA_16_LEN;

    rq->timeout = q->sg_timeout;
    rq->end_io = 0;
    rq->end_io_data = 0;

    blk_rq_map_kern(q, rq, pbuf_sector, 512, GFP_KERNEL);

    memset(sense, 0, sizeof(sense));
    s_rq->sense = sense;
    s_rq->sense_len = 0;
    s_rq->retries = 0;

    blk_execute_rq(q, bdev->bd_disk, rq, 0);
    ret = s_rq->result;

    blk_put_request(rq);
    return ret;
}

static int32_t devio_get_identify_A(struct block_device *bdev,
                                    int8_t * pbuf_sector)
{
    int8_t sense[SCSI_SENSE_BUFFERSIZE];
    struct request_queue *q;
    struct request *rq;
    struct scsi_request *s_rq;
    int32_t ret;

    if (!bdev) {
        return -EPERM;
    }

    if (!bdev->bd_disk) {
        return -EPERM;
    }

    if (!bdev->bd_disk->queue) {
        return -EPERM;
    }

    q = bdev->bd_disk->queue;
    if (NULL == q) {
        return -EPERM;
    }

    rq = blk_get_request(q, REQ_OP_SCSI_IN, 0);
    if (IS_ERR(rq)) {
        return -EPERM;
    }

    s_rq = scsi_req(rq);
    memset(s_rq->cmd, 0, 16);
    s_rq->cmd[0] = SG_ATA_16;
    s_rq->cmd[1] = SG_ATA_PROTO_PIO_IN;
    s_rq->cmd[2] |=
        SG_CDB2_TLEN_NSECT | SG_CDB2_TLEN_SECTORS | SG_CDB2_TDIR_FROM_DEV;
    s_rq->cmd[13] = 0;          //tf->dev
    s_rq->cmd[14] = ATA_OP_IDENT;   //tf->command;
    s_rq->cmd_len = SG_ATA_16_LEN;
    rq->timeout = q->sg_timeout;
    rq->end_io = 0;
    rq->end_io_data = 0;

    blk_rq_map_kern(q, rq, pbuf_sector, 512, GFP_KERNEL);

    memset(sense, 0, sizeof(sense));
    s_rq->sense = sense;
    s_rq->sense_len = 0;
    s_rq->retries = 0;

    blk_execute_rq(q, bdev->bd_disk, rq, 0);
    ret = s_rq->result;

    blk_put_request(rq);
    return ret;
}

int32_t devio_get_identify(int8_t * nm_dev, uint32_t * pret_key)
{
    int8_t buf[SECTOR_SIZE];
    struct block_device *pbdev;
    uint16_t *p;
    int32_t ret, i;

    p = (uint16_t *) & (buf[20]);
    memset(buf, '?', SECTOR_SIZE);

    if (IS_ERR(pbdev = lookup_bdev(nm_dev))) {
        LOGERR("Fail to open dev %s: %ld\n", nm_dev, PTR_ERR(pbdev));
        return PTR_ERR(pbdev);
    }

    if ((ret = devio_get_identify_A(pbdev, buf)) < 0) {
        LOGERR("get identify err %d\n", ret);
        goto l_fail;
    }

    for (i = 0; i < 10; i++) {
        __be16_to_cpus(&p[i]);
    }

    *pret_key = ((sector64) p[0]) + p[5];
    *pret_key = ((*pret_key) * p[2] * p[4]) - (p[3] * (*pret_key));
    *pret_key = *pret_key + p[9];

#if LFSM_RELEASE_VER == 0
    LOGINFO("dev %s series num [%20s] key %08x\n", nm_dev, buf + 20, *pret_key);
#endif

  l_fail:
    return 0;
}

int32_t devio_get_spin_hour(int8_t * nm_dev, int32_t * pret_spin_hour)
{
    int8_t buf[SECTOR_SIZE];
    struct block_device *pbdev;
    struct ata_smart_attribute *par_attrib;
    int32_t i, ret;

    par_attrib = (struct ata_smart_attribute *)(buf + 2);
    ret = -EIO;
    memset(buf, '?', SECTOR_SIZE);
    if (IS_ERR(pbdev = lookup_bdev(nm_dev))) {
        LOGERR("Fail to open dev %s: %ld\n", nm_dev, PTR_ERR(pbdev));
        return PTR_ERR(pbdev);
    }

    if ((ret = devio_get_smart_data(pbdev, buf)) < 0) {
        LOGERR("get identify err %d\n", ret);
        goto l_fail;
    }

    /*  // for debug only.
       for (i = 0; i < 10; i++) {
       printk("[%d] ",i);
       for(j=0;j<12;j++)
       printk("%02X ",((unsigned char*)&par_attrib[i])[j]);
       printk("\n");
       printk("id %d raw %d\n",par_attrib[i].id, *((unsigned int*)par_attrib[i].raw));
       }
     */

    for (i = 0; i < 10; i++) {
        if (par_attrib[i].id == 9) {
            *pret_spin_hour = *((uint32_t *) par_attrib[i].raw);
        }
    }

#if LFSM_RELEASE_VER == 0
    LOGINFO("spin hour %d (0x%x) \n", *pret_spin_hour, *pret_spin_hour);
#endif
    ret = 0;

  l_fail:
    return ret;
}

int32_t devio_erase_async(sector64 lsn, int32_t size_in_eus, devio_fn_t * pcb)
{
    struct block_device *bdev;
    sector64 lba_erase;
    int32_t id_dev;

    if (!pbno_lfsm2bdev_A(lsn, &lba_erase, &id_dev, &grp_ssds)) {
        return -EINVAL;
    }
    if (NULL == (bdev = diskgrp_get_drive(&grp_ssds, id_dev))) {    //pgrp->ar_drives[dev_id].bdev;
        return -EINVAL;
    }

    pcb->id_disk = id_dev;      //the diskgrp_put_drive is at devio_erase_async_bh
    if (devio_raw_erase_async(bdev, lba_erase, size_in_eus, pcb) < 0) {
        LOGERR("send erase fail id_disk %d\n", id_dev);
        return -EIO;
    }

    return 0;
}

int32_t devio_erase_async_bhA(devio_fn_t * pcb, int32_t bi_vcnt)
{
    struct bio *pBio = pcb->pBio;
    if (pBio == NULL)
        return -ENOMEM;
    lfsm_wait_event_interruptible(pcb->io_queue,
                                  atomic_read(&pcb->result) !=
                                  DEVIO_RESULT_WORKING);
    if (atomic_read(&pcb->result) != DEVIO_RESULT_SUCCESS) {
        free_composed_bio(pBio, bi_vcnt);
        return -EIO;
    } else {
        free_composed_bio(pBio, bi_vcnt);
        pcb->pBio = NULL;
        return 0;
    }
}

#if LFSM_ENABLE_TRIM == 1 || LFSM_ENABLE_TRIM == 4
static int32_t devio_trim_async_bh(devio_fn_t * pcb)
{
    int32_t ret;

    ret = 0;
    //lfsm_wait_event_interruptible(pcb->io_queue,
    //        atomic_read(&pcb->result) != DEVIO_RESULT_WORKING);
    _devio_wait(pcb, __func__);
    //printk("pcb_result in bh: %d\n", atomic_read(&pcb->result));
    if (atomic_read(&pcb->result) != DEVIO_RESULT_SUCCESS) {
        ret = -EIO;
    }

    if (pcb->rq) {
        blk_put_request(pcb->rq);
    }

    return ret;
}
#endif

int32_t devio_erase_async_bh(devio_fn_t * pcb)
{
    int32_t ret;

#if LFSM_ENABLE_TRIM == 0
    return 0;
#elif LFSM_ENABLE_TRIM == 1
    ret = devio_trim_async_bh(pcb);
#elif LFSM_ENABLE_TRIM == 2
    ret =
        devio_erase_async_bhA(pcb,
                              (pcb->pBio != NULL) ? pcb->pBio->bi_vcnt : 0);
#elif LFSM_ENABLE_TRIM == 3
    ret = devio_erase_async_bhA(pcb, 0);    // Peter Key!!! For discard we don't need to alloc bi_io_vec ; otherwise, it will stack dump in freeing bi_io_vec action.
#else
    ret = devio_trim_async_bh(pcb); // Peter add, I use the same method when LFSM_ENABLE_TRIM ==1 or 4
#endif

    if (pcb->id_disk >= 0) {    // AN means get drive in previous
        diskgrp_put_drive(&grp_ssds, pcb->id_disk);
    }

    return ret;
}

bool devio_async_write_bh(devio_fn_t * pCb)
{
    bool ret;

    ret = true;
    //lfsm_wait_event_interruptible(pCb->io_queue,
    //        atomic_read(&pCb->result) != DEVIO_RESULT_WORKING);
    _devio_wait(pCb, __func__);

    if (atomic_read(&pCb->result) != DEVIO_RESULT_SUCCESS) {
        LOGERR("error query %d\n", atomic_read(&pCb->result));
        ret = false;
    }

    free_composed_bio(pCb->pBio, pCb->pBio->bi_vcnt);
    pCb->pBio = NULL;
    return ret;
}

int32_t devio_copy_fpage(sector64 src_pbno, sector64 dest_pbno1,
                         sector64 dest_pbno2, int32_t cn_fpage)
{
    int8_t *buf;
    int32_t i, cn_copied_page;
    bool ret_w1, ret_w2;

    cn_copied_page = 0;
    buf = track_vmalloc(FLASH_PAGE_SIZE, GFP_KERNEL, M_OTHER);

    if (NULL == buf) {
        LOGINFO("Cannot alloc memory\n");
        return -EIO;
    }

    for (i = 0; i < cn_fpage; i++) {
        LOGINFO("copy from sector %llu to sector %llu\n",
                src_pbno + i * SECTORS_PER_FLASH_PAGE,
                dest_pbno1 + cn_copied_page * SECTORS_PER_FLASH_PAGE);

        devio_read_pages(src_pbno + i * SECTORS_PER_FLASH_PAGE, buf, 1, NULL);
        LOGINFO("Copy %d ,data %8x\n", i, *((int32_t *) buf));
        if (*((int32_t *) buf) == UL_EMPTY_MARK) {
            LOGINFO("data is empty, skip\n");
            continue;
        }

        ret_w1 =
            devio_write_pages(dest_pbno1 +
                              cn_copied_page * SECTORS_PER_FLASH_PAGE, buf,
                              NULL, 1);
        ret_w2 =
            devio_write_pages(dest_pbno2 +
                              cn_copied_page * SECTORS_PER_FLASH_PAGE, buf,
                              NULL, 1);

        if (ret_w1 == false && ret_w2 == false) {
            LOGINFO("write_page fail,ret1 %d ret2 %d\n", ret_w1, ret_w2);
            track_vfree(buf, FLASH_PAGE_SIZE, M_OTHER);
            return -EIO;
        }
        cn_copied_page++;
    }
    track_vfree(buf, FLASH_PAGE_SIZE, M_OTHER);

    return cn_copied_page;
}
