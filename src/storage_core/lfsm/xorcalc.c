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
#include "xorcalc.h"
#include "io_manager/io_common.h"

#if LFSM_XOR_METHOD == 3
static bool xorcalc_save_inside_xor_result(struct page
                                           *ar_xored_page[MEM_PER_FLASH_PAGE],
                                           struct bio_container *ecc_bioc)
{
    // save the xored result
    int8_t *src, *dest;
    int32_t i;

    for (i = 0; i < MEM_PER_FLASH_PAGE; i++) {
        ar_xored_page[i] = track_alloc_page(GFP_KERNEL);
        dest = kmap(ar_xored_page[i]);
        src = kmap(ecc_bioc->bio->bi_io_vec[i].bv_page);

        memcpy(dest, src, PAGE_SIZE);

        kunmap(ecc_bioc->bio->bi_io_vec[i].bv_page);
        kunmap(ar_xored_page[i]);
    }
    return true;
}

static bool xorcalc_check_compare_xor_result(struct page
                                             *ar_xored_page[MEM_PER_FLASH_PAGE],
                                             struct bio_container *ecc_bioc)
{
    // save the xored result
    int8_t *src, *dest;
    int32_t i;
    bool result;

    result = true;
    for (i = 0; i < MEM_PER_FLASH_PAGE; i++) {

        dest = kmap(ar_xored_page[i]);
        src = kmap(ecc_bioc->bio->bi_io_vec[i].bv_page);

        if (memcmp(dest, src, PAGE_SIZE) != 0) {    // result different
            LOGINFO("calc   inside result ");
            hexdump(dest, 16);
            LOGINFO("calc  outside result ");
            hexdump(src, 16);
            result = true;
        }

        kunmap(ar_xored_page[i]);
        kunmap(ecc_bioc->bio->bi_io_vec[i].bv_page);
        track_free_page(ar_xored_page[i]);
    }
    return result;
}
#endif

#if LFSM_XOR_METHOD == 3 ||  LFSM_XOR_METHOD == 2

int32_t xorcalc_outside_stripe_lock(struct bio_container *ecc_bioc)
{
    /*
     * Ding: wf says if use kmalloc here, we may encounter performance issue.
     * So we give it static, and the size is limited as max HPEU disk num(16)
     */
    int8_t *src[(1 << RDDCY_ECC_MARKER_BIT_NUM)];
    struct lfsm_stripe_w_ctrl *pstripe_wctrl;
    struct lfsm_stripe *cur_stripe;
    int8_t *dest, *pbuf_xor;
    int32_t i, idx_mpage, cn_data_page, cn_ExtEcc, retValue, max_data_page,
        time_out;

    pstripe_wctrl = ecc_bioc->pstripe_wctrl;
    cur_stripe = pstripe_wctrl->psp;

    pbuf_xor = pstripe_wctrl->pbuf->pbuf_xor;
    cn_data_page = atomic_read(&pstripe_wctrl->cn_allocated);
    cn_ExtEcc = pstripe_wctrl->cn_ExtEcc;
    retValue = cn_ExtEcc + cn_data_page;
    max_data_page =
        hpeu_get_maxcn_data_disk(ecc_bioc->lfsm_dev->hpeutab.cn_disk);

    LFSM_ASSERT2(cn_data_page < (1 << RDDCY_ECC_MARKER_BIT_NUM),
                 "hpeu disk number %d over bound %d\n",
                 cn_data_page, 1 << RDDCY_ECC_MARKER_BIT_NUM);

    if (retValue >= max_data_page) {
        if (retValue > max_data_page) {
            LOGERR
                ("full stripe but retValue is to large start_pbno %llx cn_data_page %d cn_ExtEcc %d cur_stripe %p pstripe_wctrl %p\n",
                 cur_stripe->start_pbno, cn_data_page, cn_ExtEcc, cur_stripe,
                 pstripe_wctrl);
        }
        retValue = 0;
    }

    time_out = 100;
    while (atomic_read(&pstripe_wctrl->pbuf->wio_cn) < pstripe_wctrl->cn_ExtEcc) {
        msleep(1);
        time_out--;
        if (time_out <= 0) {
            LOGWARN
                ("Wait previous Ext ecc calculate time out pbuf->wio_cn %d pstripe_wctrl->cn_ExtEcc %d\n",
                 atomic_read(&pstripe_wctrl->pbuf->wio_cn),
                 pstripe_wctrl->cn_ExtEcc);
            break;
        }
    }

    //LFSM_MUTEX_LOCK(&cur_stripe->buf_lock);

#if LFSM_XOR_METHOD == 3
    struct page *ar_xored_page[MEM_PER_FLASH_PAGE];
    xorcalc_save_inside_xor_result(ar_xored_page, ecc_bioc);
#endif

    for (idx_mpage = 0; idx_mpage < MEM_PER_FLASH_PAGE; idx_mpage++) {
        dest = kmap(ecc_bioc->bio->bi_io_vec[idx_mpage].bv_page);
        memset(dest, 0, PAGE_SIZE);

        if (cn_ExtEcc < 1) {
            for (i = 0; i < cn_data_page; i++) {
                src[i] = kmap(pstripe_wctrl->ar_page[i][idx_mpage]);
            }
            xorcalc_xor_blocks(cn_data_page, PAGE_SIZE, dest, (void **)src);    // no return value
        } else {
            src[0] = pbuf_xor;
            for (i = 0; i < cn_data_page; i++)
                src[i + 1] = kmap(pstripe_wctrl->ar_page[i][idx_mpage]);
            xorcalc_xor_blocks(cn_data_page + 1, PAGE_SIZE, dest, (void **)src);    // no return value
        }
        if (retValue > 0) {
            memcpy(pbuf_xor, dest, PAGE_SIZE);  // copy Ecc data
        }

        for (i = 0; i < cn_data_page; i++) {
            kunmap(pstripe_wctrl->ar_page[i][idx_mpage]);
        }

        kunmap(ecc_bioc->bio->bi_io_vec[idx_mpage].bv_page);
    }

    atomic_set(&pstripe_wctrl->pbuf->wio_cn, retValue);
    //LFSM_MUTEX_UNLOCK(&cur_stripe->buf_lock); 
#if LFSM_XOR_METHOD == 3
    if (!xorcalc_check_compare_xor_result(ar_xored_page, ecc_bioc)) {
        LOGERR("ECC calculate different between inside and outside xor\n");
        return -EIO;
    }
#endif

    return retValue;
}
#endif
