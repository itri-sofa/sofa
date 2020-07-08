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
#include "io_manager/io_common.h"
#include "EU_access.h"
#include "stripe.h"

int32_t stripe_async_read(lfsm_dev_t * td, struct EUProperty **ar_peu,
                          sector64 ** p_metadata, int32_t * map, int32_t start,
                          int32_t len, int32_t idx_fail,
                          stsk_stripe_rebuild_t * ptsr, int32_t maxcn_data_disk,
                          struct status_ExtEcc_recov *status_ExtEcc)
{
    sector64 pbno;
    sector64 lbno;
    int32_t i, ret, cn_used, disk;
    bool isP = 0;

    cn_used = 0;
    ptsr->lpn = 0;

    if (idx_fail == maxcn_data_disk || (status_ExtEcc->s_ExtEcc & 0x0F) == 0x01) {
        isP = 1;
    }

    if (len <= maxcn_data_disk) {
        if (isP == 1) {
            len--;
        } else if ((status_ExtEcc->s_ExtEcc & 0x8F) == 0x80) {  // use Ext ecc P
            i = status_ExtEcc->idx;
            lbno =
                td->ExtEcc[status_ExtEcc->ecc_idx].eu_main->act_metadata.
                mdnode->lpn[i];
            ptsr->lpn ^= RDDCY_CLEAR_ECCBIT(lbno);
            pbno =
                td->ExtEcc[status_ExtEcc->ecc_idx].eu_main->start_pos +
                (i << SECTORS_PER_FLASH_PAGE_ORDER);
            ret = devio_async_read_pages(pbno, 1, &ptsr->arcb_read[cn_used++]);
            if (ret < 0) {
                return ret;
            }
            len--;
        }
    }

    for (i = 0; i < len; i++) {
        if (i == idx_fail) {
            continue;
        }

        disk = map[i];
        pbno =
            ar_peu[disk]->start_pos + (start << SECTORS_PER_FLASH_PAGE_ORDER);
        lbno = p_metadata[disk][start];
        if (pbno > grp_ssds.capacity) {
            LOGERR("invalid pbno %llx. idx_fpage_in_hpeu=%d ar_peu %p\n", pbno,
                   i, ar_peu);
            return -EFAULT;
        }

        ret = devio_async_read_pages(pbno, 1, &ptsr->arcb_read[cn_used]);
        if (ret < 0) {
            return ret;
        }
        ptsr->lpn ^= RDDCY_CLEAR_ECCBIT(lbno);
        cn_used++;
        if (cn_used >= maxcn_data_disk)
            break;
    }

    if (isP == 1) {             // the fpage in the disk is a ECC page
        ptsr->lpn |= RDDCY_ECCBIT(ptsr->len);
    }
    return 0;
}

bool stripe_locate(lfsm_dev_t * td, sector64 start_pos, sector64 ** p_metadata,
                   struct rddcy_fail_disk *fail_disk, int32_t start,
                   int32_t * ret_len, int32_t cn_ssds_per_hpeu,
                   struct status_ExtEcc_recov *status_ExtEcc)
{
    sector64 lbno, pbno, start_pbno, new_pbno;
    int32_t i, len, p_pos, old_len;
    struct EUProperty *eu_ExtEcc;
    struct external_ecc *pExt_ecc;
    uint8_t fail_status;
    int32_t *map, ecc_idx;

    map = rddcy_get_stripe_map(td->stripe_map, start, cn_ssds_per_hpeu);
    p_pos = map[cn_ssds_per_hpeu - 1];

    /* Case 1: Check Ecc is valid, so it is full stripe */
    fail_status = 0;
    if (p_pos == fail_disk->idx[0]) {
        fail_status |= 0x01;
    }

    lbno = p_metadata[p_pos][start];
    if (((fail_status & 0x01) == 0)
        && ((lbno >> (RDDCY_ECC_BIT_OFFSET + RDDCY_ECC_MARKER_BIT_NUM)) == 0)
        && RDDCY_IS_ECCLPN(lbno)
        && RDDCY_GET_STRIPE_SIZE(lbno) == cn_ssds_per_hpeu) {
        goto _FULL_STRIPE;
    }

    /* Get Stripe length */
    old_len = 0;
    for (i = 0; i < cn_ssds_per_hpeu; i++) {
        if (i == p_pos) {
            continue;
        }
        old_len++;

        if (i == fail_disk->idx[0]) {
            continue;
        }

        lbno = p_metadata[i][start];

        if (SPAREAREA_IS_EMPTY(lbno)) {
            break;
        }
    }

    if (i == cn_ssds_per_hpeu) {
        old_len++;
    }

    ecc_idx = ((start / STRIPE_CHUNK_SIZE) % cn_ssds_per_hpeu) +
        (fail_disk->pg_id * cn_ssds_per_hpeu);
    pExt_ecc = &td->ExtEcc[ecc_idx];
    status_ExtEcc->s_ExtEcc = fail_status;
    status_ExtEcc->ecc_idx = ecc_idx;

    eu_ExtEcc = pExt_ecc->eu_main;
    start_pbno = start_pos + (start << SECTORS_PER_FLASH_PAGE_ORDER);
    new_pbno = (status_ExtEcc->new_start_pbno == 0) ? start_pbno :
        status_ExtEcc->new_start_pbno + (start << SECTORS_PER_FLASH_PAGE_ORDER);

    switch (status_ExtEcc->s_ExtEcc) {
    case 0x01:                 // fail disk is ExtEcc P
        /* Case 2: Ecc ssd are fail , return old_len */
        if (old_len == cn_ssds_per_hpeu) {
            status_ExtEcc->s_ExtEcc = 0;
            goto _FULL_STRIPE;
        }
        /* Not full stripe */
        *ret_len = old_len;
        status_ExtEcc->idx =
            eu_ExtEcc->log_frontier >> SECTORS_PER_FLASH_PAGE_ORDER;
        status_ExtEcc->start_pbno = new_pbno;
        LOGINFO
            ("All Ext Ecc disks are fail state, return len %d idx %d start_pbno %llx\n",
             *ret_len, status_ExtEcc->idx, status_ExtEcc->start_pbno);
        return true;
    }

    /* Case 3: Get stripe len from ExtEcc */
    i = (eu_ExtEcc->log_frontier >> SECTORS_PER_FLASH_PAGE_ORDER) - 2;
    lbno = 0;
    len = 0;
    for (; i >= 0; i -= 2) {
        pbno = eu_ExtEcc->act_metadata.mdnode->lpn[i + 1];
        lbno = eu_ExtEcc->act_metadata.mdnode->lpn[i];
        if (pbno == start_pbno) {
            len = RDDCY_GET_STRIPE_SIZE(lbno);
            if (len > old_len) {    // the ExtEcc info is wrong
                len = 0;
                break;
            }

            status_ExtEcc->idx = i;
            status_ExtEcc->start_pbno = new_pbno;
            status_ExtEcc->s_ExtEcc |= 0x80;    // 0x8x is indicated must read data from ExtEcc
            break;
        }
    }

    if (len == 0)
        status_ExtEcc->s_ExtEcc = 0;

    *ret_len = len;
    return true;

  _FULL_STRIPE:
    *ret_len = cn_ssds_per_hpeu;

    return true;
}

int32_t stripe_async_rebuild(lfsm_dev_t * td, sector64 pbno,
                             stsk_stripe_rebuild_t * ptsr, int8_t ** buf_fpage,
                             int8_t * buf_xor,
                             struct status_ExtEcc_recov *status_ExtEcc)
{
    int32_t i, ret, maxcn_data_disk;
    int8_t *buf_tmp = buf_fpage[0];
    sector64 start_pbno;
    struct external_ecc *ext_ecc;

    memset(buf_xor, 0, FLASH_PAGE_SIZE);
    maxcn_data_disk = hpeu_get_maxcn_data_disk(td->hpeutab.cn_disk);

    if (ptsr->len <= maxcn_data_disk)
        maxcn_data_disk = ptsr->len - 1;

    for (i = 0; i < maxcn_data_disk; i++) {
        if (!devio_async_read_pages_bh(1, buf_tmp, &ptsr->arcb_read[i])) {
            LOGWARN("wrong prbcb %p len %d i %d, submit reread %llds\n",
                    ptsr, ptsr->len, i,
                    (sector64) ptsr->arcb_read[i].pBio->bi_iter.bi_sector);
            if (!devio_reread_pages_after_fail
                (ptsr->arcb_read[i].pBio->bi_iter.bi_sector, buf_tmp, 1)) {
                LOGERR("fail to reread page %llds\n",
                       (sector64) ptsr->arcb_read[i].pBio->bi_iter.bi_sector);
                return -EIO;
            }
        }
        xor_block(buf_xor, buf_tmp, FLASH_PAGE_SIZE);
    }

    if (status_ExtEcc && status_ExtEcc->s_ExtEcc != 0) {
        start_pbno = status_ExtEcc->start_pbno;
        ext_ecc = &td->ExtEcc[status_ExtEcc->ecc_idx];
        if (status_ExtEcc->new_start_pbno != 0) {
            mdnode_set_lpn(ext_ecc->eu_main->act_metadata.mdnode, start_pbno,
                           status_ExtEcc->idx + 1);
        }
        if (status_ExtEcc->s_ExtEcc & 0x0F) {
            pbno =
                ext_ecc->eu_main->start_pos +
                (status_ExtEcc->idx << SECTORS_PER_FLASH_PAGE_ORDER);
            mdnode_set_lpn(ext_ecc->eu_main->act_metadata.mdnode, ptsr->lpn,
                           status_ExtEcc->idx);
            mdnode_set_lpn(ext_ecc->eu_main->act_metadata.mdnode, start_pbno,
                           status_ExtEcc->idx + 1);
            ext_ecc->eu_main->log_frontier += (SECTORS_PER_FLASH_PAGE << 1);
        }
    }

    if ((ret = devio_write_page(pbno, buf_xor, ptsr->lpn, &ptsr->cb_write))
        < 0) {
        return ret;
    }

    return 0;
}

bool stripe_init(lfsm_stripe_t * psp, int32_t id_pgroup)
{
    mutex_init(&psp->lock);
    mutex_init(&psp->buf_lock);
    psp->id_hpeu = -1;
    psp->id_pgroup = id_pgroup;
    atomic_set(&psp->wio_cn, 0);
    atomic_set(&psp->idx, 0);
    psp->wctrl = NULL;

    return stripe_data_init(psp, false);
}

void stripe_destroy(lfsm_stripe_t * psp)
{
    if (!IS_ERR_OR_NULL(&psp->lock)) {
        mutex_destroy(&psp->lock);
    }

    if (!IS_ERR_OR_NULL(&psp->buf_lock)) {
        mutex_destroy(&psp->buf_lock);
    }
    psp->id_hpeu = -1;
    psp->id_pgroup = -1;
    if (NULL != psp->wctrl->pbuf) {
        if (NULL != psp->wctrl->pbuf->pbuf_xor) {
            track_vfree(psp->wctrl->pbuf->pbuf_xor, FLASH_PAGE_SIZE, M_OTHER);
        }
        track_vfree(psp->pbuf, sizeof(lfsm_stripe_buf_t), M_OTHER);
    }

    if (atomic_read(&psp->wctrl->cn_allocated) != 0) {
        LOGWARN("psp->wctrl->cn_allocated !=0 %d\n",
                atomic_read(&psp->wctrl->cn_allocated));
    }
//    track_kfree(psp->wctrl,sizeof(struct lfsm_stripe_w_ctrl),M_GC);
    kmem_cache_free(ppool_wctrl, psp->wctrl);
}

bool stripe_data_init(lfsm_stripe_t * psp, bool isExtEcc)
{
    struct lfsm_stripe_w_ctrl *wctrl;
    //unsigned long flag;

    if (!isExtEcc) {
        if (NULL != psp->wctrl) {
            psp->wctrl->isEndofStripe = 1;
            psp->wctrl->cn_ExtEcc = atomic_read(&psp->cn_ExtEcc);
        }
        psp->xor_lbns = 0;
        atomic_set(&psp->cn_ExtEcc, 0);
        //spin_lock_irqsave(&psp->buf_lock, flag);
        if (NULL ==
            (psp->pbuf =
             track_vmalloc(sizeof(lfsm_stripe_buf_t), GFP_KERNEL, M_OTHER))) {
            LOGERR("pbuf memory allocation fail\n");
            return false;
        }
        if (NULL == (psp->pbuf->pbuf_xor =
                     track_vmalloc(FLASH_PAGE_SIZE, GFP_KERNEL | __GFP_ZERO,
                                   M_OTHER))) {
            LOGERR("pbuf_xor memory allocation fail\n");
            return false;
        }

        memset(psp->pbuf->pbuf_xor, 0, FLASH_PAGE_SIZE);
        atomic_set(&psp->pbuf->wio_cn, 0);
        //spin_unlock_irqrestore(&psp->buf_lock, flag); 
    } else {
        psp->wctrl->cn_ExtEcc = atomic_read(&psp->cn_ExtEcc);
        atomic_set(&psp->cn_ExtEcc, atomic_read(&psp->wio_cn));
    }

    psp->wctrl = wctrl = (struct lfsm_stripe_w_ctrl *)
        kmem_cache_alloc(ppool_wctrl, GFP_KERNEL);
    if (NULL == (psp->wctrl)) {
        LOGERR("psp->wctrl memory allocation fail\n");
        return false;
    }

    wctrl->psp = psp;
    wctrl->pbuf = psp->pbuf;
    wctrl->isEndofStripe = 0;
    atomic_set(&(wctrl->cn_completed), 0);
    atomic_set(&(wctrl->error), 0);
    atomic_set(&(wctrl->cn_allocated), 0);
    atomic_set(&(wctrl->cn_finish), 0);

    return true;
}

// below three functions are supposed to called under psp->lock!!
int32_t stripe_current_disk(lfsm_stripe_t * psp)
{
    LFSM_ASSERT(mutex_is_locked(&psp->lock));
    return ((atomic_read(&psp->wio_cn) % lfsmdev.hpeutab.cn_disk) +
            (psp->id_pgroup * lfsmdev.hpeutab.cn_disk));
}

void stripe_commit_disk_select(lfsm_stripe_t * psp)
{
    LFSM_ASSERT(mutex_is_locked(&psp->lock));
    atomic_set(&psp->wio_cn,
               (atomic_read(&psp->wio_cn) + 1) % lfsmdev.hpeutab.cn_disk);
}

bool stripe_is_last_page(lfsm_stripe_t * psp, int32_t maxcn_data_disk)
{
    LFSM_ASSERT(mutex_is_locked(&psp->lock));
    return (atomic_read(&psp->wio_cn) == maxcn_data_disk);
}
