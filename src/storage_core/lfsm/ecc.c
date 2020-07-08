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
#include "ecc.h"
#include "io_manager/io_common.h"
#include "io_manager/io_write.h"
#include "stripe.h"
#include "devio.h"
#include "EU_access.h"
#include "EU_create.h"
#include "spare_bmt_ul.h"
#include "devio.h"
#include "bmt_ppdm.h"
#include "bmt.h"
#include "metabioc.h"
#include "mdnode.h"
#include "conflict.h"

#include "xorcalc.h"

static int32_t _rddcy_fpage_read_gut(lfsm_dev_t * td, sector64 pbno,
                                     int32_t sz_stripe,
                                     sector64 * ar_stripe_pbno,
                                     int8_t ** ar_pdatabuf)
{
    int32_t i, ret, id_fail_trunk[MAX_FAIL_DISK], cn_fail;
    devio_fn_t *ar_cb;

    cn_fail = 0;

    for (i = 0; i < MAX_FAIL_DISK; i++)
        id_fail_trunk[i] = -1;
    ar_cb = (devio_fn_t *)
        track_kmalloc(td->hpeutab.cn_disk * sizeof(devio_fn_t), GFP_KERNEL,
                      M_OTHER);
    if (NULL == ar_cb) {
        ret = -ENOMEM;
        goto l_end;
    }

    for (i = 0; i < sz_stripe; i++) {
        if (ar_stripe_pbno[i] == PBNO_SPECIAL_BAD_PAGE) {
            if (cn_fail >= MAX_FAIL_DISK) {
                goto l_fail;
            }
            id_fail_trunk[cn_fail++] = i;
            continue;
        }

        if ((ret = devio_async_read_pages(ar_stripe_pbno[i], 1, &ar_cb[i]))
            < 0) {
            if (cn_fail >= MAX_FAIL_DISK) {
                goto l_fail;
            }
            id_fail_trunk[cn_fail++] = i;
            continue;
        }
    }

    if (cn_fail == 0) {
        ret = -EINVAL;
        goto l_end;
    }
    for (i = 0; i < cn_fail; i++)
        memset(ar_pdatabuf[id_fail_trunk[i]], 0, FLASH_PAGE_SIZE);

    for (i = 0; i < sz_stripe; i++) {
        if (i == id_fail_trunk[0]) {
            continue;
        }

        if (!devio_async_read_pages_bh(1, ar_pdatabuf[i], &ar_cb[i])) {
            if (cn_fail >= MAX_FAIL_DISK) {
                goto l_fail;
            }
            id_fail_trunk[cn_fail++] = i;
            continue;
        } else {
            if (cn_fail == 1)
                xor_block(ar_pdatabuf[id_fail_trunk[0]], ar_pdatabuf[i],
                          FLASH_PAGE_SIZE);
        }
    }

    ret = cn_fail;
    goto l_end;

  l_fail:
    LOGERR("Too many failure disks, can not rebuild data\n");
    ret = -EIO;

  l_end:
    if (ar_cb) {
        track_kfree(ar_cb, td->hpeutab.cn_disk * sizeof(devio_fn_t), M_OTHER);
    }
    return ret;
}

int32_t rddcy_fpage_read(lfsm_dev_t * td, sector64 pbno,
                         struct bio_container *bioc, int32_t idx_fpage)
{
    sector64 *ar_stripe_pbno;
    struct EUProperty *peu;
    int8_t *ar_pdatabuf[(1 << RDDCY_ECC_MARKER_BIT_NUM)];
    struct page *dest_page[(1 << RDDCY_ECC_MARKER_BIT_NUM)];
    int32_t i, ret, sz_stripe, id_hpeu, id_fail_trunk;
    bool fg_fail;

    ret = 0;
    fg_fail = false;

    if (NULL == (ar_stripe_pbno = (sector64 *)
                 track_vmalloc(td->hpeutab.cn_disk * sizeof(sector64),
                               GFP_KERNEL, M_OTHER))) {
        return -ENOMEM;
    }

    memset(ar_pdatabuf, 0, sizeof(int8_t *) * td->hpeutab.cn_disk);
    memset(dest_page, 0, sizeof(struct page *) * td->hpeutab.cn_disk);

    if (NULL == ((peu = eu_find(pbno)))) {
        return -EINVAL;
    }

    if ((id_hpeu = peu->id_hpeu) < 0) {
        LOGERR("error pbno %lld peu:%p id_hpeu=%d\n", pbno, peu, peu->id_hpeu);
        return -EINVAL;
    }

    if ((sz_stripe =
         HPEU_get_stripe_pbnos(&td->hpeutab, id_hpeu, pbno, ar_stripe_pbno,
                               &id_fail_trunk))
        <= 0) {
        return sz_stripe;
    }

    for (i = 0; i < sz_stripe; i++) {
        if (NULL == (dest_page[i] = track_alloc_page(__GFP_ZERO))) {
            fg_fail = true;
            break;
        }
        ar_pdatabuf[i] = kmap(dest_page[i]);
    }

    if (fg_fail) {
        ret = -ENOMEM;
        goto l_end;
    }

    if ((ret =
         _rddcy_fpage_read_gut(td, pbno, sz_stripe, ar_stripe_pbno,
                               ar_pdatabuf)) >= 0) {
        bio_vec_data_rw(&(bioc->bio->bi_io_vec[idx_fpage]),
                        ar_pdatabuf[id_fail_trunk], 0);
        ret = 0;
    }

  l_end:
    for (i = 0; i < sz_stripe; i++) {
        if (ar_pdatabuf[i])
            kunmap(dest_page[i]);
        if (dest_page[i])
            track_free_page(dest_page[i]);
    }

    track_vfree(ar_stripe_pbno, td->hpeutab.cn_disk * sizeof(sector64),
                M_OTHER);
    return ret;
}

int32_t rddcy_pgroup_num_get(int8_t * buffer, int32_t size)
{
    return sprintf(buffer, "%d\n", lfsmdev.cn_pgroup);
}

int32_t rddcy_pgroup_size_get(int8_t * buffer, int32_t size)
{
    return sprintf(buffer, "%d\n", lfsmdev.hpeutab.cn_disk);
}

int32_t rddcy_faildrive_get(int8_t * buffer, int32_t size)
{
    int32_t id, i, disk, len;

    len = 0;
    id = rddcy_search_failslot(&grp_ssds);

    if (rddcy_faildrive_count_get(id) == 0)
        return sprintf(buffer, "-19 n/a\n");

    for_rddcy_faildrive_get(id, disk, i) {
        len += sprintf(buffer + len, "%d %s\n", disk,
                       (grp_ssds.ar_drives[disk].bdev ==
                        NULL) ? "closed" : "to close");
    }
    return len;
}

static int32_t rddcy_end_bio_stripe_wctrl(struct lfsm_dev_struct *td,
                                          struct lfsm_stripe_w_ctrl
                                          *pstripe_wctrl)
{
    if (NULL == pstripe_wctrl) {
        return 0;               //metadata write will not have pstripe_wctrl
    }

    if (NULL == pstripe_wctrl->psp) {
        LOGERR("%s: %p null wstrl->psp\n", __FUNCTION__, pstripe_wctrl);
        return -ENOMEM;
    }

    if ((pstripe_wctrl->psp->id_pgroup < 0)
        && (pstripe_wctrl->psp->id_pgroup >= td->cn_pgroup)) {
        LOGERR("%s: %p invalid pgrp %d\n", __FUNCTION__, pstripe_wctrl->psp,
               pstripe_wctrl->psp->id_pgroup);
        return -EINVAL;
    }
    atomic_inc(&pstripe_wctrl->cn_completed);
    wake_up_interruptible(&(td->ar_pgroup[pstripe_wctrl->psp->id_pgroup].wq));
    return 0;

}

int32_t rddcy_end_bio_write_helper(struct bio_container *entry, struct bio *bio)
{
    int32_t idx;
    if (entry->par_bioext) {
        if ((idx =
             bioext_locate(entry->par_bioext, bio, bioc_cn_fpage(entry))) < 0) {
            LOGERR("bio %p not in bioext. bioc=%p\n", bio, entry);
            return -EFAULT;
        }

        return rddcy_end_bio_stripe_wctrl(entry->lfsm_dev,
                                          entry->par_bioext->items[idx].
                                          pstripe_wctrl);
    } else {
        return rddcy_end_bio_stripe_wctrl(entry->lfsm_dev,
                                          entry->pstripe_wctrl);
    }
}

static int32_t _rddcy_check_and_send_ecc(lfsm_dev_t * td,
                                         struct lfsm_stripe_w_ctrl
                                         *pstripe_wctrl)
{
    struct swrite_param wparam[WPARAM_CN_IDS];
    struct lfsm_stripe *cur_stripe;
    int32_t ret;

    cur_stripe = pstripe_wctrl->psp;

    if (cur_stripe->wctrl != pstripe_wctrl) {
        return 0;               // ECC is gened, check before lock can reduce time
    }

    memset(wparam, 0, sizeof(struct swrite_param) * WPARAM_CN_IDS);

    LFSM_MUTEX_LOCK(&cur_stripe->lock);
    if (cur_stripe->wctrl != pstripe_wctrl) {
        rddcy_printk("ecc has been sent cur_ctrl %p, my ctrl %p \n",
                     cur_stripe->wctrl, pstripe_wctrl);
        LFSM_MUTEX_UNLOCK(&cur_stripe->lock);
        return 0;
    }

    if (atomic_read(&pstripe_wctrl->cn_completed) <
        atomic_read(&pstripe_wctrl->cn_allocated)) {
        LFSM_MUTEX_UNLOCK(&cur_stripe->lock);
        return RET_STRIPE_WAIT_AGAIN;
    }

    if (rddcy_gen_bioc(td, cur_stripe, &(wparam[WPARAM_ID_ECC]), true) == false) {
        goto l_fail;
    }

    LFSM_MUTEX_UNLOCK(&cur_stripe->lock);
    rddcy_printk(" bgset_ecc_bi_sector, %llu\n",
                 wparam[WPARAM_ID_ECC].bio_c->bio->bi_iter.bi_sector);
    ret = rddcy_write_submit(td, wparam);   //RET_BIOCBH, REGET_DESTPBNO, -EIO, 0
    if (ret == REGET_DESTPBNO || ret == -EIO) {
        return -EIO;
    }

    return 0;

  l_fail:
    LFSM_MUTEX_UNLOCK(&cur_stripe->lock);
    return -EIO;
}

static bool _rddcy_end_stripe_gut(lfsm_dev_t * td,
                                  struct lfsm_stripe_w_ctrl *pstripe_wctrl)
{
    int32_t ret_genecc;
    bool ret;

    ret_genecc = 0;

    if (NULL == pstripe_wctrl) {
        LOGERR("pid %d pstripe_wctrl = NULL\n", current->pid);
        return false;
    }

    if (NULL == pstripe_wctrl->psp) {
        dump_stack();
        LOGERR("pstripe_wctrl(%p)->psp = NULL\n", pstripe_wctrl);
        return false;
    }

  stripe_wait_again:
    lfsm_wait_event_interruptible(td->ar_pgroup[pstripe_wctrl->psp->id_pgroup].
                                  wq,
                                  atomic_read(&pstripe_wctrl->cn_completed) >=
                                  atomic_read(&pstripe_wctrl->cn_allocated));

    if ((ret_genecc = _rddcy_check_and_send_ecc(td, pstripe_wctrl))
        == RET_STRIPE_WAIT_AGAIN) {
        goto stripe_wait_again;
    } else if (ret_genecc < 0) {
        atomic_inc(&pstripe_wctrl->cn_completed);   // false means we fail to send out ecc bio, inc cn_complete manually
        ret = false;
        goto l_next;
    }

    ret = true;

  l_next:
    lfsm_wait_event_interruptible(td->ar_pgroup[pstripe_wctrl->psp->id_pgroup].
                                  wq,
                                  atomic_read(&pstripe_wctrl->cn_completed) >
                                  atomic_read(&pstripe_wctrl->cn_allocated));

    if (atomic_read(&pstripe_wctrl->cn_completed) ==
        lfsm_atomic_inc_return(&pstripe_wctrl->cn_finish)) {
//        track_kfree(pstripe_wctrl, sizeof(struct lfsm_stripe_w_ctrl),M_GC);
        if (pstripe_wctrl->isEndofStripe == 1) {
            if (NULL != pstripe_wctrl->pbuf) {
                if (NULL != pstripe_wctrl->pbuf->pbuf_xor) {
                    track_vfree(pstripe_wctrl->pbuf->pbuf_xor, FLASH_PAGE_SIZE,
                                M_OTHER);
                }
                track_vfree(pstripe_wctrl->pbuf, sizeof(lfsm_stripe_buf_t),
                            M_OTHER);
            }
        }

        kmem_cache_free(ppool_wctrl, pstripe_wctrl);
    }

    return ret;
}

bool rddcy_end_stripe(lfsm_dev_t * td, struct bio_container *pBioc)
{
    int32_t i, cn_fpage;
    bool ret;

    i = -1;
    ret = true;

    if (NULL == pBioc->par_bioext) {
        ret = _rddcy_end_stripe_gut(td, pBioc->pstripe_wctrl);
        goto l_end;
    }

    cn_fpage = bioc_cn_fpage(pBioc);
    for (i = 0; i < cn_fpage; i++) {
        if (!_rddcy_end_stripe_gut
            (td, pBioc->par_bioext->items[i].pstripe_wctrl)) {
            LOGERR("bioc %p fpage %d fail end stripe (status: %d)\n", pBioc, i,
                   pBioc->status);
            ret = false;
        }
    }

  l_end:
    pBioc->pstripe_wctrl = NULL;
    return ret;

}

// Weafon:
// The function change bio_c and lbno of pret_wparam.
// And initial and setup the pret_wparam->bioc
static bool rddcy_alloc_bioc(lfsm_dev_t * td, struct swrite_param *pret_wparam,
                             struct lfsm_stripe *active_st, bool isEndStripe)
{
    struct bio_container *pbioc;
    sector64 allocated_page_num;

    if (NULL ==
        (pret_wparam->bio_c =
         bioc_get(td, NULL, 0, 0, 0, TP_NORMAL_IO, WRITE, 1))) {
        LOGERR("rddcy alloc bioc err\n");
        return false;
    }

    pbioc = pret_wparam->bio_c;
    init_bio_container(pbioc, NULL, NULL, end_bio_write, REQ_OP_WRITE, 0,
                       MEM_PER_FLASH_PAGE);
    allocated_page_num = atomic_read(&(active_st->wio_cn));

    if (allocated_page_num >= td->hpeutab.cn_disk) {
        LOGERR("Internal active_st %p %lld\n", active_st, allocated_page_num);
        return false;
    }
    // weafon: we actually write ecc bit and xor data to postpadding page.
    // Even though I don't like this, one bool parameter can be saved.
    if (isEndStripe) {
#if LFSM_XOR_METHOD !=2
        LFSM_MUTEX_LOCK(&active_st->buf_lock);
        if (!bio_vec_data_rw
            (pbioc->bio->bi_io_vec, active_st->wctrl->pbuf->pbuf_xor, 0)) {
            return false;
        }
        LFSM_MUTEX_UNLOCK(&active_st->buf_lock);
#endif
        pret_wparam->lbno =
            active_st->xor_lbns | RDDCY_ECCBIT(allocated_page_num + 1);
    } else {
        pret_wparam->lbno = LBNO_PREPADDING_PAGE;
    }

    pbioc->start_lbno = pret_wparam->lbno;
    pbioc->end_lbno = pret_wparam->lbno;
    pbioc->pstripe_wctrl = active_st->wctrl;

    pbioc->write_type = TP_NORMAL_IO;
    //support not exist because ACF_ONGOING is set inside bioc_get
    atomic_set(&(pbioc->active_flag), ACF_ONGOING);

    return true;
}

bool rddcy_gen_bioc(lfsm_dev_t * td, struct lfsm_stripe *cur_stripe,
                    struct swrite_param *pret_wparam, bool renew_stripe)
{
    int32_t ret;
    bool isExtEcc = false;

    ret = 0;

    if (!rddcy_alloc_bioc(td, pret_wparam, cur_stripe, renew_stripe)) {
        goto fail_alloc;
    }

    if (renew_stripe) {
        if (!stripe_is_last_page
            (cur_stripe, hpeu_get_maxcn_data_disk(td->hpeutab.cn_disk))) {
            isExtEcc = true;
        }
        if (!stripe_data_init(cur_stripe, isExtEcc)) {
            goto fail_alloc;
        }
    }

    if (isExtEcc) {
        if ((ret =
             ExtEcc_get_dest_pbno(td, cur_stripe, false, pret_wparam)) < 0) {
            LOGERR("ExtEcc_get_dest_pbno fail (%d)\n", ret);
            goto fail_alloc;
        }
    } else {
        // get_dest_pbno return pbno by pret_wparm->pbno_new
        if ((ret = get_dest_pbno(td, cur_stripe, false, pret_wparam)) < 0) {
            LOGERR("get_dest_pbno fail (%d)\n", ret);
            goto fail_alloc;
        }
    }
    pret_wparam->bio_c->dest_pbn = pret_wparam->bio_c->bio->bi_iter.bi_sector =
        pret_wparam->pbno_new;
    return true;

  fail_alloc:
    if (pret_wparam->bio_c) {
        put_bio_container(td, pret_wparam->bio_c);
    }

    return false;
}

bool rddcy_xor_flash_buff_op(struct lfsm_stripe *cur_stripe,
                             struct bio_vec *page_vec, sector64 lbno)
{

    int32_t i;

#if LFSM_XOR_METHOD == 1 || LFSM_XOR_METHOD == 3
    int8_t *addr;
    int8_t *dest = cur_stripe->wctrl->pbuf_xor;
#endif
    sector64 *xor_lbno;

    xor_lbno = &(cur_stripe->xor_lbns);
    for (i = 0; i < MEM_PER_FLASH_PAGE; i++) {
#if LFSM_XOR_METHOD == 1 || LFSM_XOR_METHOD == 3
        addr = kmap(page_vec->bv_page);
        if (addr == 0) {
            return false;
        }

        xorcalc_xor_blocks(1, PAGE_SIZE, dest + i * PAGE_SIZE, (void **)&addr);
        // inside xor always use private xor routine
        kunmap(page_vec->bv_page);
#endif

#if LFSM_XOR_METHOD == 2 || LFSM_XOR_METHOD == 3
        cur_stripe->wctrl->
            ar_page[atomic_read(&cur_stripe->wctrl->cn_allocated)][i]
            = page_vec->bv_page;
#endif

        page_vec++;
    }

    (*xor_lbno) ^= lbno;

    return true;
}

int32_t rddcy_read_metadata(lfsm_dev_t * td, struct EUProperty *peu,
                            sector64 * ar_lbno)
{
    sector64 pbn;

    pbn = peu->start_pos + SECTORS_PER_SEU - METADATA_SIZE_IN_SECTOR;

    // read from metadata and isn't empty
    if ((devio_read_pages
         (pbn, (int8_t *) ar_lbno, METADATA_SIZE_IN_FLASH_PAGES, NULL))
        && (!metabioc_isempty(ar_lbno))) {
        return true;
    }
    // read from spare_area (note: support buffer size is the same or larger than metadata read size)
    if (!EU_spare_read(td, (int8_t *) ar_lbno, peu->start_pos, FPAGE_PER_SEU)) {
        return false;
    }

    return true;
}

static int32_t _rddcy_recov_alloc_mem(sector64 ** p_metadata,
                                      onDisk_md_t ** buf_metadata,
                                      int32_t count)
{
    int32_t i, metadata_size;

    metadata_size = METADATA_SIZE_IN_FLASH_PAGES * FLASH_PAGE_SIZE;

    for (i = 0; i < lfsmdev.hpeutab.cn_disk; i++) {
        p_metadata[i] = 0;
    }

    *buf_metadata = 0;
    for (i = 0; i < lfsmdev.hpeutab.cn_disk; i++) {
        if (NULL == (p_metadata[i] =
                     track_vmalloc(metadata_size, GFP_KERNEL | __GFP_ZERO,
                                   M_RECOVER))) {
            LOGERR("alloc buffer fail,\n");
            return -ENOMEM;
        }
    }

    if (NULL == (*buf_metadata =
                 track_vmalloc(metadata_size * count, GFP_KERNEL | __GFP_ZERO,
                               M_RECOVER))) {
        LOGERR("alloc buffer fail,\n");
        return -ENOMEM;
    }

    return 0;
}

static int32_t _rddcy_recov_free_mem(sector64 ** p_metadata,
                                     onDisk_md_t ** buf_metadata, int32_t count)
{
    int32_t i, metadata_size;

    metadata_size = METADATA_SIZE_IN_FLASH_PAGES * FLASH_PAGE_SIZE;
    for (i = 0; i < lfsmdev.hpeutab.cn_disk; i++) {
        if (p_metadata[i] != 0) {
            track_vfree(p_metadata[i], metadata_size, M_RECOVER);
        }
    }

    if (*buf_metadata) {
        track_vfree(*buf_metadata, metadata_size * count, M_RECOVER);
    }

    return true;
}

// reasonable valuu range: -1 ~ 8191?  
static bool _rddcy_insert_md2eu(lfsm_dev_t * td, onDisk_md_t * buf_metadata,
                                struct EUProperty *eu_new, int32_t count)
{
    int32_t i;

    if (mdnode_alloc(&eu_new->act_metadata, td->pMdpool) == -ENOMEM) {
        return false;
    }

    for (i = 0; i < count; i++) {
        mdnode_set_lpn(eu_new->act_metadata.mdnode, buf_metadata[i].lbno, i);
    }

    return true;
}

static int32_t _rddcy_update_ppq_and_eubitmap(lfsm_dev_t * td, sector64 lbno,
                                              sector64 pbno,
                                              struct EUProperty *eu_new,
                                              sector64 pbno_old)
{
    struct D_BMT_E dbmta;
    int32_t ret;

    ret = bmt_lookup(td, td->ltop_bmt, lbno, false, 0, &dbmta);

    if (ret == -EACCES) {
        if (dbmta.pbno == pbno_old) {
            reset_bitmap_ppq(pbno_old, pbno, lbno, td);
        }
        // else do nothing since it is invalid data.
    } else if (ret < 0) {
        LOGERR("rddcy update ppq and bitmap unexpect ret %d\n", ret);
        return ret;             // failure
    }

    return 0;
}

// tsr -> task_stripe_rebuild
// user should use sizeof and get to access tsr since the size of tsr is variable due to the dynamic number of cn_data_disk
static int32_t _rddcy_sizeof_tsr(int32_t maxcn_data_disk)
{
    return (sizeof(stsk_stripe_rebuild_t) +
            (sizeof(devio_fn_t) * maxcn_data_disk));
}

// user should use sizeof and get to access tsr since the size of tsr is variable due to the dynamic number of cn_data_disk
static stsk_stripe_rebuild_t *_rddcy_get_tsr(stsk_stripe_rebuild_t * ar_tsr,
                                             int32_t idx,
                                             int32_t maxcn_data_disk)
{
    if (idx < 0 || idx >= CN_REBUILD_BATCH) {
        LOGERR("unexpected index of ar_tsr: %d (expected between 0 and %d)\n",
               idx, CN_REBUILD_BATCH);
        dump_stack();
        return NULL;
    }

    return (stsk_stripe_rebuild_t *)
        ((int8_t *) ar_tsr + (idx * _rddcy_sizeof_tsr(maxcn_data_disk)));
}

static stsk_stripe_rebuild_t *_tsr_alloc_init(int32_t cn_batch,
                                              int32_t maxcn_data_disk)
{
    stsk_stripe_rebuild_t *ar_tsr, *ptsr;
    int32_t i, j;

    if (NULL == (ar_tsr =
                 track_kmalloc(_rddcy_sizeof_tsr(maxcn_data_disk) * cn_batch,
                               GFP_KERNEL, M_RECOVER))) {
        LOGERR("fail to alloc ar_spcb \n");
        return NULL;
    }

    for (i = 0; i < cn_batch; i++) {
        if (NULL == (ptsr = _rddcy_get_tsr(ar_tsr, i, maxcn_data_disk))) {
            return NULL;
        }

        for (j = 0; j < maxcn_data_disk; j++) {
            init_waitqueue_head(&ptsr->arcb_read[j].io_queue);
            atomic_set(&ptsr->arcb_read[j].result, DEVIO_RESULT_WORKING);
            ptsr->arcb_read[j].pBio = NULL;
        }
        init_waitqueue_head(&ptsr->cb_write.io_queue);
        atomic_set(&ptsr->cb_write.result, DEVIO_RESULT_WORKING);
    }

    return ar_tsr;
}

static void _rddcy_remap_fail_disks(int32_t * idx_old, int32_t * idx_new,
                                    int32_t * map, int32_t cn_ssds_per_hpeu)
{
    int32_t i;

    *idx_new = -1;
    for (i = 0; i < cn_ssds_per_hpeu; i++) {
        if (map[i] == idx_old[0]) {
            *idx_new = i;
            break;
        }
    }
}

static int32_t _rddcy_batch_rebuild_1(lfsm_dev_t * td,
                                      struct EUProperty **ar_peu,
                                      sector64 ** p_metadata, int32_t idx_row,
                                      int32_t cn_batch,
                                      struct status_ExtEcc_recov *status_ExtEcc,
                                      struct rddcy_fail_disk *fail_disk,
                                      struct EUProperty *eu_new,
                                      onDisk_md_t * buf_metadata,
                                      bool isMorePage)
{
    struct timeval s, e;
    stsk_stripe_rebuild_t *ptsr;
    void *ar_tsr;               //stsk_stripe_rebuild_t* ar_spcb=NULL;
    int8_t *buf_fpage[(1 << RDDCY_ECC_MARKER_BIT_NUM)], *buf_xor, is_rebuild;
    struct page *dest_page[(1 << RDDCY_ECC_MARKER_BIT_NUM)];
    sector64 pbno;
    int32_t i, ret, start, maxcn_data_disk, cn_rebuild;
    int32_t cn_def_len, *map, idx_fail_new;
    struct status_ExtEcc_recov *act_ExtEcc;

    ret = -ENOMEM;
    ar_tsr = NULL;
    buf_xor = NULL;
    cn_def_len = td->hpeutab.cn_disk;

    maxcn_data_disk = hpeu_get_maxcn_data_disk(td->hpeutab.cn_disk);

    if (NULL == (ar_tsr = _tsr_alloc_init(cn_batch, maxcn_data_disk))) {
        goto l_fail;
    }

    memset(buf_fpage, 0, sizeof(int8_t *) * (1 << RDDCY_ECC_MARKER_BIT_NUM));
    memset(dest_page, 0,
           sizeof(struct page *) * (1 << RDDCY_ECC_MARKER_BIT_NUM));
    if (NULL == (dest_page[0] = track_alloc_page(__GFP_ZERO))) {
        LOGERR("rddcy batch rebuild alloc buffer fail\n");
        goto l_fail;
    }
    buf_fpage[0] = kmap(dest_page[0]);

    if (NULL ==
        (buf_xor = track_vmalloc(FLASH_PAGE_SIZE, GFP_KERNEL, M_RECOVER))) {
        LOGERR("rddcy batch rebuild alloc buffer fail,\n");
        goto l_fail;
    }

    do_gettimeofday(&s);
    status_ExtEcc->s_ExtEcc = 0;
    is_rebuild = 0;
    cn_rebuild = 0;
    for (i = 0; i < cn_batch; i++) {
        if (NULL == (ptsr = _rddcy_get_tsr(ar_tsr, i, maxcn_data_disk))) {
            goto l_fail;
        }

        start = i + idx_row;

        map = rddcy_get_stripe_map(td->stripe_map, start, td->hpeutab.cn_disk);
        _rddcy_remap_fail_disks(fail_disk->idx, &idx_fail_new, map,
                                td->hpeutab.cn_disk);

        if (unlikely(i >= cn_batch - 2 && !isMorePage)) {   // Check if exist Ecc Ext
            if (!stripe_locate(td, ar_peu[0]->start_pos, p_metadata, fail_disk,
                               start, &ptsr->len, td->hpeutab.cn_disk,
                               status_ExtEcc)) {
                ret = -EINVAL;
                goto l_fail;
            }

            if (ptsr->len > cn_def_len) {
                LOGERR("start=%d len=%d when off=%d %p [%p %p %p %p]\n",
                       start, ptsr->len, idx_row, p_metadata, p_metadata[0],
                       p_metadata[1], p_metadata[2], p_metadata[3]);
                ret = -EINVAL;
                goto l_fail;
            }

            if (ptsr->len != cn_def_len) {
                if (idx_fail_new >= ptsr->len - 1 && (status_ExtEcc->s_ExtEcc & 0x0F) == 0) {   // No need to recovery
                    break;
                }
            }
        } else {
            ptsr->len = cn_def_len;
        }

        cn_rebuild++;

        if ((ret =
             stripe_async_read(td, ar_peu, p_metadata, map, start, ptsr->len,
                               idx_fail_new, ptsr, maxcn_data_disk,
                               status_ExtEcc)) < 0) {
            goto l_fail;
        }

        if (ptsr->len == 0 || (status_ExtEcc->s_ExtEcc & 0x0F)) //No nedd to write metadata 
            continue;
        buf_metadata[(i + idx_row)].lbno = ptsr->lpn;
    }

    act_ExtEcc = NULL;
    for (i = 0; i < cn_rebuild; i++) {
        if (NULL == (ptsr = _rddcy_get_tsr(ar_tsr, i, maxcn_data_disk))) {
            goto l_fail;
        }
        if (ptsr->len == 0)     //No nedd to recovery 
            continue;
        pbno =
            eu_new->start_pos + ((i + idx_row) << SECTORS_PER_FLASH_PAGE_ORDER);
        // support log_frontier counting during get addr, and not be rowback if the io fail. Tifa, Weafon, Annan
        eu_new->log_frontier += SECTORS_PER_FLASH_PAGE;

        if (i == cn_batch - 1) {
            if (is_rebuild == 1)
                break;
            act_ExtEcc = status_ExtEcc;
        }
        if ((ret =
             stripe_async_rebuild(td, pbno, ptsr, buf_fpage, buf_xor,
                                  act_ExtEcc)) < 0) {
            goto l_fail;
        }
    }

    for (i = 0; i < cn_rebuild; i++) {
        if (IS_ERR_OR_NULL(ptsr = _rddcy_get_tsr(ar_tsr, i, maxcn_data_disk))) {
            goto l_fail;
        }

        if (ptsr->len == 0)     //No nedd to recovery 
            continue;

//        printk("wait write result %p %d\n",&(ar_spcb[i]),i);
        if (!devio_async_write_bh(&ptsr->cb_write)) {
            LOGERR("rebuild id_row=%d fail\n", i + idx_row);
            ret = -EAGAIN;
            goto l_fail;
        }
    }
    do_gettimeofday(&e);
    ret = cn_rebuild;
    LOGINFO("cn_batch=%d ret=%d cost %llu us\n", cn_batch, ret,
            get_elapsed_time2(&e, &s));

  l_fail:
    if (buf_xor) {
        track_vfree(buf_xor, FLASH_PAGE_SIZE, M_RECOVER);
    }

    if (buf_fpage[0])
        kunmap(dest_page[0]);
    if (dest_page[0])
        track_free_page(dest_page[0]);

    if (ar_tsr) {
        track_kfree(ar_tsr, _rddcy_sizeof_tsr(maxcn_data_disk) * cn_batch,
                    M_RECOVER);
    }

    return ret;
}

static int32_t _rddcy_lock_lpn(lfsm_dev_t * td, sector64 lpn,
                               struct bio_container *pbioc)
{
    int32_t HasConflict;
    bioc_get_init(pbioc, lpn, 1, TP_NORMAL_IO);
    HasConflict = conflict_bioc_login(pbioc, td, 0);
    if (HasConflict) {
        LOGINFO("wait conflict bioc(%p) lpn %lld\n", pbioc, lpn);
    }

    bioc_wait_conflict(pbioc, td, HasConflict);

    return 0;
}

static int32_t _rddcy_unlock_lpn(lfsm_dev_t * td, sector64 lpn,
                                 struct bio_container *pbioc)
{
    conflict_bioc_logout(pbioc, td);
    return 0;
}

static int32_t _rddcy_update_lpn_gut(lfsm_dev_t * td, struct EUProperty *eu_new,
                                     struct EUProperty *eu_old,
                                     onDisk_md_t * buf_metadata,
                                     int32_t cn_scan_fpages,
                                     int32_t * pret_handled_fpages)
{
    struct bio_container *bioc;
    sector64 pbno, pbno_old, lbno;
    int32_t i, ret, cn_ecc, cn_pad;

    cn_ecc = 0;
    cn_pad = 0;
    bioc = kmalloc(sizeof(struct bio_container), GFP_KERNEL);

    for (i = 0; i < cn_scan_fpages; i++) {
        lbno = buf_metadata[i].lbno;
        if (RDDCY_IS_ECCLPN(lbno)) {
            cn_ecc++;
            continue;
        }

        if (lbno == LBNO_PREPADDING_PAGE) {
            cn_pad++;
            continue;
        }

        if (lbno >= td->logical_capacity) {
            LOGERR
                ("buf_metadata[%d] lbno %llx is more than SOFA capacity %llx\n",
                 i, lbno, td->logical_capacity);
            continue;
        }
        _rddcy_lock_lpn(td, lbno, bioc);

        pbno = eu_new->start_pos + (i << SECTORS_PER_FLASH_PAGE_ORDER);
        pbno_old = eu_old->start_pos + (i << SECTORS_PER_FLASH_PAGE_ORDER);
        if ((ret =
             _rddcy_update_ppq_and_eubitmap(td, lbno, pbno, eu_new,
                                            pbno_old)) < 0) {
            LOGINFO
                ("update ppq fail buf_metadata[%d] pbno %llx pbno_old %llx ret=%d\n",
                 i, pbno, pbno_old, ret);
            *pret_handled_fpages = i;
            return ret;
        }

        _rddcy_unlock_lpn(td, lbno, bioc);
    }

    *pret_handled_fpages = cn_scan_fpages;
    LOGINFO("cn_scan_page %d, cn_ecc %d, cn_pad %d\n", cn_scan_fpages, cn_ecc,
            cn_pad);
    kfree(bioc);
    return 0;
}

static int32_t _rddcy_update_lpn(lfsm_dev_t * td, struct EUProperty *eu_new,
                                 struct EUProperty *eu_old,
                                 onDisk_md_t * buf_metadata,
                                 int32_t cn_valid_fpages)
{
    int32_t cn_fpages_finish, cn_fpages_finish_rollback;

    cn_fpages_finish = 0;
    cn_fpages_finish_rollback = 0;
    if (_rddcy_update_lpn_gut(td, eu_new, eu_old, buf_metadata, cn_valid_fpages,
                              &cn_fpages_finish) < 0) {
        if (_rddcy_update_lpn_gut(td, eu_new, eu_old, buf_metadata,
                                  cn_fpages_finish,
                                  &cn_fpages_finish_rollback) < 0) {
            LOGERR("Internal# fail to lpn rollback %d\n",
                   cn_fpages_finish_rollback);
        }

        eu_new->id_hpeu = -1;
        return -EPERM;
    }

    return 0;
}

int32_t rddcy_rebuild_metadata(struct HPEUTab *phpeutab, sector64 start_pos,
                               struct rddcy_fail_disk *fail_disk,
                               sector64 ** p_metadata)
{
    int32_t cn_written_fpages, i, len, j;
    int32_t idx_fail, cn_disk;
    struct status_ExtEcc_recov status_ExtEcc;
    bool isEccP = 0;
    sector64 meta_ExtEccP = 0;

    cn_disk = phpeutab->cn_disk;
    idx_fail = fail_disk->idx[0];

    if ((cn_written_fpages =
         hpeu_get_eu_frontier(p_metadata, idx_fail, cn_disk)) < 0) {
        return -EINVAL;
    }

    LOGINFO("cn_valid_fpage %d\n", cn_written_fpages);

    memset(p_metadata[idx_fail], 0xff,
           METADATA_SIZE_IN_FLASH_PAGES * FLASH_PAGE_SIZE);

    status_ExtEcc.s_ExtEcc = 0;

    if (idx_fail == hpeu_get_maxcn_data_disk(cn_disk))
        isEccP = 1;
    for (i = 0; i < cn_written_fpages; i++) {
        if (i >= cn_written_fpages - 2) {   // Check if exist Ecc Ext
            if (!stripe_locate(phpeutab->td, start_pos, p_metadata, fail_disk,
                               i, &len, cn_disk, &status_ExtEcc)) {
                return -EINVAL;
            }

            if (len != cn_disk) {
                if (idx_fail >= len - 1) {  // No need to recovery
                    cn_written_fpages--;
                    break;
                }
                if ((status_ExtEcc.s_ExtEcc & 0x80) == 0x80) {  // use Ext ecc
                    meta_ExtEccP =
                        phpeutab->td->ExtEcc[status_ExtEcc.ecc_idx].eu_main->
                        act_metadata.mdnode->lpn[status_ExtEcc.idx];
                    len--;
                }
            }
        } else {
            len = cn_disk;
        }

        p_metadata[idx_fail][i] = 0;

        for (j = 0; j < len; j++) {
            if (j == idx_fail) {
                continue;
            }

            p_metadata[idx_fail][i] ^= RDDCY_CLEAR_ECCBIT(p_metadata[j][i]);
        }
        if (meta_ExtEccP != 0) {
            p_metadata[idx_fail][i] ^= meta_ExtEccP;
            meta_ExtEccP = 0;
        }

        if (isEccP) {           // the fpage in the disk is a ECC page
            p_metadata[idx_fail][i] |= RDDCY_ECCBIT(len);
        }
    }

    return cn_written_fpages;
}

// rebuild one hpeu, buf_metadata keeps the metadata of pages in the hpeu
static int32_t _rddcy_rebuild_hpeu_gut(struct HPEUTab *phpeutab,
                                       int32_t id_hpeu,
                                       struct rddcy_fail_disk *fail_disk,
                                       sector64 ** p_metadata,
                                       struct EUProperty **ar_peu,
                                       onDisk_md_t * buf_metadata)
{
    struct timeval s, e;
    struct EUProperty *eu_new[MAX_FAIL_DISK] = { NULL };
    HPEUTabItem_t *pItem;
    lfsm_dev_t *td;
    int32_t ret, cn_written_fpages, idx_start, resi_page, cn_batch;
    int32_t cn_written_tmp[MAX_FAIL_DISK], i, buf_offset, cn_re0;
    onDisk_md_t *target_meta[MAX_FAIL_DISK] = { NULL };
    struct status_ExtEcc_recov status_ExtEcc;
    struct external_ecc *ext_ecc;

    buf_offset = METADATA_SIZE_IN_FLASH_PAGES * FLASH_PAGE_SIZE;
    td = phpeutab->td;
    idx_start = 0;
    status_ExtEcc.new_start_pbno = 0;

    // Ding: Because the size of Type HPEUTabItem is variable,
    // you cannot access ar_item via the below way...
    // if (phpeutab->ar_item[id_hpeu].state==good)
    // new function named HPEU_get_HPEUTabItem is create for you
    if (NULL == (pItem = HPEU_GetTabItem(phpeutab, id_hpeu))) {
        return -EINVAL;
    }

    if (pItem->state == good) {
        LOGWARN("skip id_hpeu %d due to it is healthy\n", id_hpeu);
        return 0;
    }

    do_gettimeofday(&s);

    if ((ret = HPEU_get_eus(phpeutab, id_hpeu, ar_peu)) != phpeutab->cn_disk) {
        LOGWARN("skip id_hpeu %d due to incompleteness "
                "(valid eu count in this hpeu is %d)\n", id_hpeu, ret);
        if (ret < 0) {
            LFSM_FORCE_RUN(return 0);
            return -ENOMEM;
        } else {
            LOGWARN
                ("HPEU %d may only be allocated partially and no need to rescure, "
                 "fore state to good \n", id_hpeu);
            HPEU_set_state(phpeutab, id_hpeu, good);
            return 0;
        }
    }
    //rddcy_read_all_disk_metadata
    if ((ret = HPEU_read_metadata(td, ar_peu, p_metadata, fail_disk)) < 0) {
        return ret;
    }
    if ((cn_written_fpages = hpeu_get_eu_frontier(p_metadata, fail_disk->idx[0],
                                                  phpeutab->cn_disk)) < 0) {
        return -EINVAL;
    }

    LOGINFO("written_page = %d\n", cn_written_fpages);
    do_gettimeofday(&e);
    LOGINFO("Read and analyze metadata spending %llu us\n",
            get_elapsed_time2(&e, &s));

    for (i = 0; i < fail_disk->count; i++) {
        target_meta[i] =
            (onDisk_md_t *) ((char *)buf_metadata + buf_offset * i);
        if (NULL ==
            (eu_new[i] =
             FreeList_delete(gc
                             [HPEU_EU2HPEU_OFF
                              (fail_disk->idx[i], fail_disk->pg_id)], EU_DATA,
                             true))) {
            LOGERR("Fail to get a new eu from gc[%d]\n",
                   HPEU_EU2HPEU_OFF(fail_disk->idx[i], fail_disk->pg_id));
            return -ENOMEM;
        }

        eu_new[i]->temper = ar_peu[fail_disk->idx[i]]->temper;  //to be fixed...
    }

    if (fail_disk->idx[0] == 0 && eu_new[0]->start_pos != ar_peu[0]->start_pos) {   // Start pbno is changed
        LOGINFO("Start pbno of HPEU id %d has changed %llx -> %llx\n", id_hpeu,
                ar_peu[0]->start_pos, eu_new[0]->start_pos);
        status_ExtEcc.new_start_pbno = eu_new[0]->start_pos;
    }

    resi_page = cn_written_fpages;
    memset(buf_metadata, 0xff, buf_offset * fail_disk->count);
    cn_re0 = 0;
    while (resi_page > 0) {
        if (resi_page > (CN_REBUILD_BATCH >> (fail_disk->count - 1))) {
            cn_batch = CN_REBUILD_BATCH >> (fail_disk->count - 1);
        } else {
            cn_batch = resi_page;
        }

        resi_page -= cn_batch;
        switch (fail_disk->count) {
        case 1:
            if ((ret = _rddcy_batch_rebuild_1(td, ar_peu, p_metadata,
                                              idx_start, cn_batch,
                                              &status_ExtEcc, fail_disk,
                                              eu_new[0], buf_metadata,
                                              (resi_page > 0))) < 0) {
                LOGERR("batch stripe rebuild_1 fail : idx_start=%d ret=%d\n",
                       idx_start, ret);
                return ret;
            }
            cn_re0 += (cn_batch - ret);
            break;
        default:
            LOGERR("Wrong fail disk counts %d\n", fail_disk->count);
            return -EINVAL;
        }

        if (status_ExtEcc.s_ExtEcc & 0x80) {
            if (status_ExtEcc.new_start_pbno != 0) {
                ext_ecc = &td->ExtEcc[status_ExtEcc.ecc_idx];
                mdnode_set_lpn(ext_ecc->eu_main->act_metadata.mdnode,
                               status_ExtEcc.start_pbno, status_ExtEcc.idx + 1);
            }
        }
        idx_start += cn_batch;
    }

    do_gettimeofday(&s);
    cn_written_tmp[0] = cn_written_fpages - cn_re0;
    for (i = 0; i < fail_disk->count; i++) {
        if (eu_is_active(ar_peu[fail_disk->idx[i]]) < 0) {
            if (!metabioc_commit(td, target_meta[i], eu_new[i])) {
                return -EAGAIN;
            }
        } else {
            if (!_rddcy_insert_md2eu
                (td, target_meta[i], eu_new[i], cn_written_tmp[0])) {
                return -ENOMEM;
            }
        }
        eu_new[i]->id_hpeu = id_hpeu;   // if eu_new->id_hpeu is not set to current id_hpeu, warining will be generated when update_ppq in next functiomn
        if (_rddcy_update_lpn(td, eu_new[i], ar_peu[fail_disk->idx[i]],
                              target_meta[i], cn_written_tmp[0]) < 0) {
            return -EPERM;
        }

        if (!HPEU_turn_fixed(phpeutab, id_hpeu, fail_disk->idx[i], eu_new[i])) {
            return -EPERM;
        }
    }

    HPEU_set_state(phpeutab, id_hpeu, good);
    /////////// tifa check
    mdnode_hpeu_check_exist(phpeutab, ar_peu, -1);

    do_gettimeofday(&e);
    LOGINFO("Write metadata spending %llu us\n", get_elapsed_time2(&e, &s));

    return 0;
}

int32_t rddcy_search_failslot(diskgrp_t * pgrp)
{
    return rddcy_search_failslot_by_pg_id(pgrp, -1);
}

int32_t rddcy_search_failslot_by_pg_id(struct diskgrp *pgrp,
                                       int32_t select_pg_id)
{
    int32_t i, idx_failslot, count = -1;

    idx_failslot = 0;
    count = 0;
    for (i = 0; i < pgrp->cn_max_drives; i++) {
        if (pgrp->ar_subdev[i].load_from_prev != all_ready) {   //==LFSM_BAD_DISK)
            if (lfsmdev.hpeutab.cn_disk > 0) {
                if (-1 != select_pg_id && PGROUP_ID(i) != select_pg_id)
                    continue;
            }
            idx_failslot |= (i << (count << 3));
            count++;
            if (count == 2)
                break;
            /*
               for(j=0;j<LFSM_DISK_NUM;j++)
               {
               LOGINFO("fail=%d load_from_prev",id_faildisk);
               printk(" %d",td->subdev[j].load_from_prev);
               printk("\n");
               }
             */
        }
    }
    idx_failslot |= (count << 24);
    if (count == 0)
        idx_failslot = -ENODEV;
    return idx_failslot;
}

void rddcy_fail_ids_to_disk(int32_t cn_disk, int32_t id_failslot,
                            struct rddcy_fail_disk *fail_disk)
{
    int32_t disk, i;
    fail_disk->count = 0;
    fail_disk->idx[0] = -1;

    if (id_failslot < 0)
        return;
    for_rddcy_faildrive_get(id_failslot, disk, i) {
        fail_disk->idx[i] = disk % cn_disk;
        if (i == 0) {
            fail_disk->pg_id = PGROUP_ID(disk);
            fail_disk->count = 1;
            break;
        }
    }
}

int32_t rddcy_faildrive_count_get(int32_t id)
{
    int32_t count = 0;
    if (id > 0)
        count = (id >> 24) & 0xff;
    return count;
}

static int32_t _rddcy_reinit_active_eu(lfsm_dev_t * td, int32_t pg_id)
{
    int32_t hpeu_start_disk, i, ret;
    struct lfsm_stripe_w_ctrl *pstripe_wctrl;

    ret = -EIO;
    hpeu_start_disk = pg_id * td->hpeutab.cn_disk;

    for (i = hpeu_start_disk; i < hpeu_start_disk + td->hpeutab.cn_disk; i++) {
        if (gc[i]->free_list_number < TEMP_LAYER_NUM) {
            LOGINFO("free_list_num = %llu < %d, cannot reinit active eu \n",
                    gc[i]->free_list_number, TEMP_LAYER_NUM);
            return -EIO;
        }
    }

    for (i = hpeu_start_disk; i < hpeu_start_disk + td->hpeutab.cn_disk; i++) {
        if ((ret = EU_close_active_eu(td, gc[i], LVP_heap_insert)) < 0) {
            LOGERR("close_active_eu fail ret= %d\n", ret);
            return ret;
        }

        LOGINFO("close_active_eu after rebuild\n");
        if ((ret = active_eu_init(td, gc[i])) < 0) {
            LOGERR("active_eu_init fail , ret= %d\n", ret);
            return ret;
        }
        LOGINFO("active_eu_init after rebuild\n");
    }

    for (i = 0; i < TEMP_LAYER_NUM; i++) {
        atomic_set(&td->ar_pgroup[pg_id].stripe[i].wio_cn, 0);
        atomic_set(&td->ar_pgroup[pg_id].stripe[i].cn_ExtEcc, 0);
        atomic_set(&td->ar_pgroup[pg_id].stripe[i].idx, 0);
        if ((pstripe_wctrl = td->ar_pgroup[pg_id].stripe[i].wctrl) != NULL) {   //free lfsm_stripe_w_ctrl
            LOGINFO("_rddcy_reinit_active_eu free lfsm_stripe_w_ctrl\n");
            if (NULL != pstripe_wctrl->pbuf->pbuf_xor) {
                track_vfree(pstripe_wctrl->pbuf->pbuf_xor, FLASH_PAGE_SIZE,
                            M_OTHER);
            }
            track_vfree(pstripe_wctrl->pbuf, sizeof(lfsm_stripe_buf_t),
                        M_OTHER);
            kmem_cache_free(ppool_wctrl, pstripe_wctrl);

            td->ar_pgroup[pg_id].stripe[i].wctrl = NULL;
            stripe_data_init(&td->ar_pgroup[pg_id].stripe[i], false);
        }
    }

    return 0;
}

int32_t rddcy_rebuild_hpeus_A(struct HPEUTab *hpeutab, diskgrp_t * pgrp,
                              int32_t id_failslot)
{

    struct timeval s, e;
    HPEUTabItem_t *pItem;
    lfsm_dev_t *td;
    int32_t i, ret, pg_id, disk;
    struct rddcy_fail_disk fail_disk;
    //int8_t nm_io_scheduler[MAXLEN_IO_SCHE_NAME]; 

    ret = 0;
    td = hpeutab->td;

    rddcy_fail_ids_to_disk(hpeutab->cn_disk, id_failslot, &fail_disk);
    pg_id = fail_disk.pg_id;

    do_gettimeofday(&s);
/*    strcpy(nm_io_scheduler, "noop");
    for(i = 0; i < hpeutab->cn_disk; i ++) {
        if(pgrp->ar_subdev[i].load_from_prev == all_ready) {
            strcpy(nm_io_scheduler, 
#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 10, 0)
                    pgrp->ar_drives[i].bdev->bd_disk->queue->elevator->elevator_type->elevator_name);
#else
                    pgrp->ar_drives[i].bdev->bd_disk->queue->elevator->type->elevator_name);
#endif
            break;
        }
    }
*/
    LOGINFO("start rebuild pgroup %d fail disk DATA part\n", pg_id);
    for (i = 0; i < fail_disk.count; i++)
        LOGINFO("\tfail disk idx %d\n", fail_disk.idx[i]);

    if ((ret = metalog_grp_load_all(&td->mlogger, td, &fail_disk) < 0)) {
        return ret;
    }

    if (pgroup_disk_all_active(pg_id, pgrp, hpeutab->cn_disk) < 0) {
        return -EPERM;
    }

    for (i = 0; i < hpeutab->cn_total; i++) {
        if (NULL == (pItem = HPEU_GetTabItem(hpeutab, i))) {
            return -EINVAL;
        }

        if (pItem->cn_added == -1) {
            continue;
        }

        if (pItem->state != damege) {
            continue;
        }

        if (HPEU_get_pgroup_id(hpeutab, i) != pg_id) {
            continue;
        }

        if ((ret = rddcy_rebuild_hpeu(hpeutab, i, &fail_disk)) < 0) {
            return ret;
        }
    }

    if ((ret = pgroup_birthday_reassign(td->ar_pgroup, pg_id))
        < 0) {
        return ret;
    }

    if ((ret = _rddcy_reinit_active_eu(td, fail_disk.pg_id)) < 0) {
        LOGERR("Cannot re-init active EU after rescue pgroud id = %d (%d)\n",
               fail_disk.pg_id, ret);
        return ret;
    }
    for_rddcy_faildrive_get(id_failslot, disk, i) {
        metalog_grp_eulog_rescue(&td->mlogger, PGROUP_ID(disk), disk);
    }

    if (UpdateLog_reset_withlock(td) < 0) {
        LOGERR("bmt commit and change uplog fail\n");
        return -EIO;
    }

    for_rddcy_faildrive_get(id_failslot, disk, i) {
        pgrp->ar_subdev[disk].load_from_prev = all_ready;
        td->ar_pgroup[PGROUP_ID(disk)].state = epg_good;
        //set io scheduler to fixed disk
        //nmra elv_iosched_store(pgrp->ar_drives[disk].bdev->bd_disk->queue, nm_io_scheduler, sizeof(nm_io_scheduler));
    }
    if (dedicated_map_logging(td) < 0) {    //log_super
        LOGERR("dedicated map logging fail\n");
        return -EIO;
    }

    do_gettimeofday(&e);
    // for autotesting, don't change without ack Chyi
    LOGINFO("exec rebuild all ret= %d duration=%llu us\n", ret,
            get_elapsed_time2(&e, &s));
#if LFSM_PREERASE == 1
    ersmgr_wakeup_worker(&td->ersmgr);
#endif
    return 0;
}

int32_t rddcy_rebuild_hpeu(struct HPEUTab *hpeutab, int32_t id_hpeu,
                           struct rddcy_fail_disk *fail_disk)
{
    sector64 **p_metadata;
    struct EUProperty **ar_peu;
    onDisk_md_t *buf_metadata;
    int32_t ret, cn_retry;

    ret = 0;
    cn_retry = 0;
    LOGINFO("Start_rebuild id_hpeu %d, fail count %d\n", id_hpeu,
            fail_disk->count);

    if (NULL == (p_metadata = (sector64 **)
                 track_kmalloc(hpeutab->cn_disk * sizeof(sector64 *),
                               GFP_KERNEL, M_RECOVER))) {
        return -ENOMEM;
    }

    if (NULL == (ar_peu = (struct EUProperty **)
                 track_kmalloc(hpeutab->cn_disk * sizeof(struct EUProperty *),
                               GFP_KERNEL, M_RECOVER))) {
        ret = -ENOMEM;
        goto l_free_lbno;
    }

    if ((ret =
         _rddcy_recov_alloc_mem(p_metadata, &buf_metadata, fail_disk->count))) {
        goto l_fail;
    }

    while (cn_retry < LFSM_MIN_RETRY) {
        cn_retry++;

        if ((ret = _rddcy_rebuild_hpeu_gut(hpeutab, id_hpeu, fail_disk,
                                           p_metadata, ar_peu,
                                           buf_metadata)) < 0) {
            if (ret == -EAGAIN) {
                LOGWARN("id_hpeu %d rebuild fail cn_retry=%d, ret=%d\n",
                        id_hpeu, cn_retry, ret);
                continue;
            } else {
                LOGERR("id_hpeu %d rebuild fail cn_retry=%d, ret=%d\n", id_hpeu,
                       cn_retry, ret);
                LFSM_FORCE_RUN(ret = 0);    // do next hpeu when demo
                goto l_fail;
            }
        }
        break;                  //success
    }

    if (ret < 0) {
        LFSM_FORCE_RUN(ret = 0);    // do next hpeu when demo
    }

  l_fail:
    _rddcy_recov_free_mem(p_metadata, &buf_metadata, fail_disk->count);
    track_kfree(ar_peu, hpeutab->cn_disk * sizeof(struct EUProperty *),
                M_RECOVER);
  l_free_lbno:
    track_kfree(p_metadata, hpeutab->cn_disk * sizeof(sector64 *), M_RECOVER);
    return ret;
}

int32_t rddcy_rebuild_hpeus_data(struct HPEUTab *hpeutab, diskgrp_t * pgrp)
{
    //start rebuild
    int32_t id_failslot;
    if ((id_failslot = rddcy_search_failslot(pgrp)) < 0) {
        return -ENODEV;
    }

    LOGINFO(" found fail disk id %d , and reopen successful \n", id_failslot);
    if (rddcy_rebuild_hpeus_A(hpeutab, pgrp, id_failslot) < 0) {
        return -EIO;
    }
    return 0;
}

int32_t rddcy_rebuild_fail_disk(struct HPEUTab *hpeutab, diskgrp_t * pgrp,
                                struct rddcy_fail_disk *fail_disk)
{
    int32_t i, disk, id_failslot;

    for (i = 0; i < fail_disk->count; i++) {
        disk = HPEU_EU2HPEU_OFF(fail_disk->idx[i], fail_disk->pg_id);
        if (diskgrp_isfaildrive(pgrp, disk)) {
            diskgrp_switch(pgrp, disk, 1);
            if (diskgrp_isfaildrive(pgrp, disk)) {
                return -ENOEXEC;
            }
        }
    }

    LOGINFO("wait for pgroup %d cn_gcing ==0\n", fail_disk->pg_id);
    wait_event_interruptible(lfsmdev.worker_queue,
                             atomic_read(&lfsmdev.ar_pgroup[fail_disk->pg_id].
                                         cn_gcing) == 0);

    id_failslot = 0;
    for (i = 0; i < fail_disk->count; i++) {
        disk = HPEU_EU2HPEU_OFF(fail_disk->idx[i], fail_disk->pg_id);
        id_failslot |= (disk << (i << 3));
        LOGINFO
            ("Set disk %d name %s to activated and reopen success, start rebuild\n",
             disk, grp_ssds.ar_drives[disk].nm_dev);

        if (system_eu_rescue(hpeutab->td, disk) < 0) {
            LOGWARN("Rescue system EU fail\n");
            return -EIO;
        }
        LOGINFO("Rescue system EU Success\n");
    }
    id_failslot |= (fail_disk->count << 24);

    if (rddcy_rebuild_hpeus_A(hpeutab, pgrp, id_failslot) < 0) {
        LOGWARN("Rescue DATA EU fail\n");
        return -EIO;
    }
    LOGINFO("Rescue DATA EU Success\n");
    return 0;
}

int32_t rddcy_rebuild_all(struct HPEUTab *hpeutab, diskgrp_t * pgrp)
{
    int32_t id_failslot;
    struct rddcy_fail_disk fail_disk;

    if ((id_failslot = rddcy_search_failslot(pgrp)) < 0)
        return 0;

    rddcy_fail_ids_to_disk(hpeutab->cn_disk, id_failslot, &fail_disk);
    return rddcy_rebuild_fail_disk(hpeutab, pgrp, &fail_disk);
}

static int32_t ExtEcc_eu_init_data(lfsm_dev_t * td, struct EUProperty *eu)
{
    eu->id_hpeu = -2;
    if (mdnode_alloc(&eu->act_metadata, td->pMdpool) == -ENOMEM)
        return -ENOMEM;
    return 0;
}

int32_t ExtEcc_eu_init(lfsm_dev_t * td, int32_t idx, bool fg_insert_old)
{
    struct HListGC *h = NULL;
    int32_t *map_ptr, disk, cn_ssd_for_hepu;
    struct external_ecc *pEcc = &td->ExtEcc[idx];
    struct EUProperty *old_eu;

    if (unlikely(!fg_insert_old)) { // for system initial
        cn_ssd_for_hepu = td->hpeutab.cn_disk;

        map_ptr =
            &td->stripe_map->disk_num[(idx % cn_ssd_for_hepu) *
                                      cn_ssd_for_hepu];
        disk = map_ptr[cn_ssd_for_hepu - 1];
        disk += idx / cn_ssd_for_hepu * cn_ssd_for_hepu;
        h = gc[disk];
        pEcc->eu_main = FreeList_delete(h, EU_EXT_ECC, false);
        if (ExtEcc_eu_init_data(td, pEcc->eu_main) == -ENOMEM) {
            LOGERR("ExtEcc_eu_init alloc memory error\n");
            return -ENOMEM;
        }
    } else {
        old_eu = pEcc->eu_main;
        h = gc[old_eu->disk_num];
        pEcc->eu_main = FreeList_delete(h, EU_EXT_ECC, false);

        if (old_eu->act_metadata.mdnode != NULL) {
            mdnode_free(&old_eu->act_metadata, td->pMdpool);
        }

        FreeList_insert(h, old_eu);
        if (ExtEcc_eu_init_data(td, pEcc->eu_main) == -ENOMEM)
            return -ENOMEM;
        LOGINFO("External Ecc start: %llu disk %d\n",
                pEcc->eu_main->start_pos, pEcc->eu_main->disk_num);
    }

    LOGINFO("BMT External Ecc initalizing..... DONE\n");
    return 0;
}

int32_t ExtEcc_eu_set_mdnode(lfsm_dev_t * td)
{
    struct external_ecc *pEcc;
    struct EUProperty *eu;
    sector64 pbn;
    int32_t idx;

    for (idx = 0; idx < td->cn_ssds; idx++) {
        pEcc = &td->ExtEcc[idx];
        eu = pEcc->eu_main;
        if (ExtEcc_eu_init_data(td, eu) == -ENOMEM)
            return -ENOMEM;
        if (eu->log_frontier > 0) {
            //get metadata from EU
            LOGINFO("EU %p (%llu) read metadata form EU metadata page\n", eu,
                    eu->start_pos);
            pbn = eu->start_pos + SECTORS_PER_SEU - METADATA_SIZE_IN_SECTOR;
            if (devio_read_pages(pbn, (char *)eu->act_metadata.mdnode->lpn,
                                 METADATA_SIZE_IN_FLASH_PAGES, NULL) == false) {
                LOGERR("EU %p (%llu) read metadata fail\n", eu, eu->start_pos);
                return -ENOEXEC;
            }
        }
    }
    return 0;
}

int32_t ExtEcc_eu_set(lfsm_dev_t * td, struct sddmap_ondisk_header *pheader)
{
    struct external_ecc *pEcc;
    struct external_ecc_head *pEcc_head;
    struct EUProperty *eu;
    int32_t idx_pg;

    for (idx_pg = 0; idx_pg < td->cn_ssds; idx_pg++) {
        pEcc = &td->ExtEcc[idx_pg];
        pEcc_head = ddmap_get_ext_ecc(pheader, idx_pg);

        if (IS_ERR_OR_NULL(eu = eu_find(pEcc_head->external_ecc_head)))
            return -ENOEXEC;

        pEcc->eu_main = eu;
        pEcc->eu_main->log_frontier = pEcc_head->external_ecc_frontier;

        if (grp_ssds.ar_subdev[eu->disk_num].load_from_prev != all_ready) {
            pEcc->eu_main->log_frontier = 0;    // Reset Extecc frontier if ExtEcc fail
        }

        if (FreeList_delete_by_pos(td, pEcc->eu_main->start_pos, EU_EXT_ECC) == NULL)   // check for ExtEcc's EU stat
            return -ENOEXEC;
    }
    return 0;
}

void ExtEcc_destroy(lfsm_dev_t * td)
{
    int idx_pg, ret;
    if (IS_ERR_OR_NULL(td->ExtEcc))
        return;
    for (idx_pg = 0; idx_pg < td->cn_ssds; idx_pg++) {
        if (IS_ERR_OR_NULL(td->ExtEcc[idx_pg].eu_main))
            continue;
        if ((ret =
             flush_metadata_of_active_eu(td, NULL,
                                         td->ExtEcc[idx_pg].eu_main)) < 0) {
            LOGERR("flush_metadata_of_active_eu fail ret=%d\n", ret);
            return;
        }
    }
}

int32_t ExtEcc_get_dest_pbno(lfsm_dev_t * td, struct lfsm_stripe *pstripe,
                             bool isValidData, struct swrite_param *pwp)
{
    int32_t ret = 0, ecc_idx;
    struct external_ecc *extEcc;
    struct EUProperty *active_eu;

    LFSM_ASSERT(pwp != NULL);

    ecc_idx =
        ((atomic_read(&pstripe->idx) / STRIPE_CHUNK_SIZE) %
         td->hpeutab.cn_disk) + (pstripe->id_pgroup * td->hpeutab.cn_disk);
    extEcc = &td->ExtEcc[ecc_idx];

    LFSM_MUTEX_LOCK(&extEcc->ecc_lock);
    active_eu = extEcc->eu_main;
    if (active_eu->log_frontier == active_eu->eu_size - METADATA_SIZE_IN_SECTOR) {
        ExtEcc_eu_init(td, ecc_idx, true);
        active_eu = extEcc->eu_main;
    }

    pwp->pbno_new = active_eu->log_frontier + active_eu->start_pos;
    pwp->pbno_meta = 0;
    pwp->id_disk = active_eu->disk_num;

    if (active_eu->act_metadata.mdnode == NULL) {
        LOGERR("ExtEcc disk[%d] mdnode is NULL start %llu log_frontier %d\n",
               active_eu->disk_num, active_eu->start_pos,
               active_eu->log_frontier);
        LFSM_MUTEX_UNLOCK(&extEcc->ecc_lock);

        return -ENOMEM;
    }
    mdnode_set_lpn(active_eu->act_metadata.mdnode, pwp->lbno,
                   active_eu->log_frontier >> SECTORS_PER_FLASH_PAGE_ORDER);
    active_eu->log_frontier += SECTORS_PER_FLASH_PAGE;
    mdnode_set_lpn(active_eu->act_metadata.mdnode, pstripe->start_pbno,
                   active_eu->log_frontier >> SECTORS_PER_FLASH_PAGE_ORDER);
    active_eu->log_frontier += SECTORS_PER_FLASH_PAGE;
    LFSM_MUTEX_UNLOCK(&extEcc->ecc_lock);

    return ret;

}

int32_t ExtEcc_get_stripe_size(struct lfsm_dev_struct *td, sector64 start_pos,
                               int32_t row, sector64 * extecc_pbno,
                               struct rddcy_fail_disk *fail_disk)
{
    sector64 lbno, pbno, start_pbno;
    int32_t i, len;
    struct EUProperty *eu_ExtEcc;
    struct external_ecc *ext_ecc;
    int32_t ecc_idx, cn_ssds_per_hpeu, p_pos;

    cn_ssds_per_hpeu = td->hpeutab.cn_disk;
    ecc_idx = ((row / STRIPE_CHUNK_SIZE) % cn_ssds_per_hpeu) +
        (fail_disk->pg_id * cn_ssds_per_hpeu);
    ext_ecc = &td->ExtEcc[ecc_idx];
    eu_ExtEcc = ext_ecc->eu_main;

    start_pbno = start_pos + (row << SECTORS_PER_FLASH_PAGE_ORDER);

    p_pos = ext_ecc->eu_main->disk_num % cn_ssds_per_hpeu;
    if (p_pos == fail_disk->idx[0]) {
        *extecc_pbno = PBNO_SPECIAL_BAD_PAGE;
        return -1;
    }

    i = (eu_ExtEcc->log_frontier >> SECTORS_PER_FLASH_PAGE_ORDER) - 2;
    lbno = 0;
    len = 0;
    for (; i >= 0; i -= 2) {
        pbno = eu_ExtEcc->act_metadata.mdnode->lpn[i + 1];
        lbno = eu_ExtEcc->act_metadata.mdnode->lpn[i];
        if (pbno == start_pbno) {
            len = RDDCY_GET_STRIPE_SIZE(lbno);
            *extecc_pbno =
                eu_ExtEcc->start_pos + (i << SECTORS_PER_FLASH_PAGE_ORDER);
            break;
        }
    }

    return len;
}

int32_t init_stripe_disk_map(lfsm_dev_t * td, int32_t ssds_per_hpeu)
{
    struct stripe_disk_map *map;
    int32_t i, j, ret = 0, cn_data, src_pos;
    int32_t disk_p, *map_ptr;

    if (NULL ==
        (map =
         track_kmalloc(sizeof(struct stripe_disk_map) +
                       sizeof(int32_t) * ssds_per_hpeu * ssds_per_hpeu,
                       GFP_KERNEL, M_OTHER))) {
        LOGERR("Can not allocate memory for stripe disk map\n");
        ret = -1;
    }

    cn_data = ssds_per_hpeu - 1;
    disk_p = cn_data;
    for (i = 0; i < ssds_per_hpeu; i++) {
        src_pos = 0;
        map_ptr = &map->disk_num[i * ssds_per_hpeu];
        for (j = 0; j < cn_data; j++) {
            if (src_pos == disk_p) {
                src_pos = (src_pos + 1) % ssds_per_hpeu;
            }

            map_ptr[j] = src_pos;
            src_pos = (src_pos + 1) % ssds_per_hpeu;
        }
        map_ptr[j++] = disk_p;
        disk_p = disk_p - 1;
    }

    td->stripe_map = map;
    return ret;
}

void free_stripe_disk_map(lfsm_dev_t * td, int32_t ssds_per_hpeu)
{
    if (td->stripe_map)
        track_kfree(td->stripe_map,
                    sizeof(struct stripe_disk_map) +
                    sizeof(int32_t) * ssds_per_hpeu * ssds_per_hpeu, M_OTHER);
}

static int32_t rddcy_check_data_1(lfsm_dev_t * td, struct EUProperty **ar_peu,
                                  sector64 ** p_metadata, int32_t start,
                                  int32_t len, int8_t ** buf_fpage,
                                  int8_t * buf_eccP,
                                  struct status_ExtEcc_recov *status_ExtEcc)
{
    int32_t i, ret, hpeu_disk_num, cn_diff;
    sector64 pbno;
    sector64 lbno, ecc_lpn;
    devio_fn_t arcb_read[1 << RDDCY_ECC_MARKER_BIT_NUM];
    bool isGetEcc;
    int32_t *map, ecc_idx;

    hpeu_disk_num = td->hpeutab.cn_disk;
    ecc_lpn = 0;
    cn_diff = 0;
    isGetEcc = false;

    map = rddcy_get_stripe_map(td->stripe_map, start, hpeu_disk_num);

    memset(buf_eccP, 0, FLASH_PAGE_SIZE);
    for (i = 0; i < len; i++) {
        if ((map[hpeu_disk_num - 1] == i || i == len - 1)
            && len != hpeu_disk_num && !isGetEcc) {
            ecc_idx =
                ((start / STRIPE_CHUNK_SIZE) % hpeu_disk_num) +
                (PGROUP_ID(ar_peu[0]->disk_num) * hpeu_disk_num);
            lbno =
                td->ExtEcc[ecc_idx].eu_main->act_metadata.mdnode->
                lpn[status_ExtEcc->idx];
            pbno =
                td->ExtEcc[ecc_idx].eu_main->start_pos +
                (status_ExtEcc->idx << SECTORS_PER_FLASH_PAGE_ORDER);
            isGetEcc = true;
        } else {
            pbno =
                ar_peu[i]->start_pos + (start << SECTORS_PER_FLASH_PAGE_ORDER);
            lbno = p_metadata[i][start];
        }
        if (pbno > grp_ssds.capacity) {
            LOGERR("invalid pbno %llx. idx_fpage_in_hpeu=%d ar_peu %p\n", pbno,
                   i, ar_peu);
            return -EFAULT;
        }

        ret = devio_async_read_pages(pbno, 1, &arcb_read[i]);
        if (ret < 0) {
            LOGWARN("wrong read page pbno %llx\n", pbno);
            return ret;
        }

        if (i < len - 1) {
            ecc_lpn ^= RDDCY_CLEAR_ECCBIT(lbno);
        } else {
            if (ecc_lpn != RDDCY_CLEAR_ECCBIT(lbno)) {
                LOGINFO("Check lpn Ecc error start %d len %d ecc %llx %llx\n",
                        start, len, ecc_lpn, lbno);
                cn_diff++;
            }
        }
    }

    for (i = 0; i < len; i++) {
        if (!devio_async_read_pages_bh(1, buf_fpage[i], &arcb_read[i])) {
            LOGWARN("wrong read page len %d i %d, submit reread %llds\n",
                    len, i, (sector64) arcb_read[i].pBio->bi_iter.bi_sector);
            return -EIO;
        }
        if (i < len - 1) {
            xor_block(buf_eccP, buf_fpage[i], FLASH_PAGE_SIZE);
        }
    }

    if (memcmp(buf_eccP, buf_fpage[len - 1], FLASH_PAGE_SIZE) != 0) {
        LOGINFO("Check Data Ecc error start_pbno %llx len %d ECC_P[0] = %x\n",
                ar_peu[0]->start_pos + (start << SECTORS_PER_FLASH_PAGE_ORDER),
                len, buf_eccP[0] & 0xff);
        for (i = 0; i < len; i++) {
            LOGINFO("\tDisk[%d][0] = %x\n", i, buf_fpage[i][0] & 0xff);
        }
        cn_diff++;
    }

    return cn_diff;
}

int32_t rddcy_check_hpeus_data_ecc(struct HPEUTab *hpeutab, diskgrp_t * pgrp)
{
    int32_t i, j, ret, cn_written_fpages, cn_hpeu_disk, len, cn_diff;
    HPEUTabItem_t *pItem;
    lfsm_dev_t *td;
    sector64 **p_metadata;
    struct EUProperty **ar_peu;
    int8_t *buf_fpage[(1 << RDDCY_ECC_MARKER_BIT_NUM)], *buf_eccP;
    struct page *dest_page[(1 << RDDCY_ECC_MARKER_BIT_NUM)];
    struct rddcy_fail_disk fail_disk;
    struct status_ExtEcc_recov status_ExtEcc;

    ret = 0;
    cn_diff = 0;
    td = hpeutab->td;
    cn_hpeu_disk = hpeutab->cn_disk;
    buf_eccP = NULL;

    LOGINFO("rddcy_check_hpeus_data_ecc start\n");
    if (NULL == (p_metadata = (sector64 **)
                 track_kmalloc(cn_hpeu_disk * sizeof(sector64 *), GFP_KERNEL,
                               M_RECOVER))) {
        return -ENOMEM;
    }

    for (i = 0; i < cn_hpeu_disk; i++) {
        p_metadata[i] = 0;
    }

    for (i = 0; i < cn_hpeu_disk; i++) {
        if (NULL == (p_metadata[i] =
                     track_kmalloc(METADATA_SIZE_IN_FLASH_PAGES *
                                   FLASH_PAGE_SIZE, GFP_KERNEL, M_RECOVER))) {
            LOGERR("alloc p_metadata buffer fail,\n");
            ret = -ENOMEM;
            goto l_free_lbno;
        }
    }

    if (NULL == (ar_peu = (struct EUProperty **)
                 track_kmalloc(cn_hpeu_disk * sizeof(struct EUProperty *),
                               GFP_KERNEL, M_RECOVER))) {
        ret = -ENOMEM;
        goto l_free_lbno;
    }

    memset(buf_fpage, 0, sizeof(int8_t *) * (1 << RDDCY_ECC_MARKER_BIT_NUM));
    memset(dest_page, 0,
           sizeof(struct page *) * (1 << RDDCY_ECC_MARKER_BIT_NUM));
    for (i = 0; i < cn_hpeu_disk; i++) {
        if (NULL == (dest_page[i] = track_alloc_page(__GFP_ZERO))) {
            LOGERR("rddcy_check_hpeus_data_ecc alloc buffer fail\n");
            ret = -ENOMEM;
            goto l_fail;
        }
        buf_fpage[i] = kmap(dest_page[i]);
    }

    if (NULL ==
        (buf_eccP = track_kmalloc(FLASH_PAGE_SIZE, GFP_KERNEL, M_RECOVER))) {
        LOGERR("rddcy_check_hpeus_data_ecc alloc Ecc P buffer fail,\n");
        ret = -ENOMEM;
        goto l_fail;
    }

    for (i = 0; i < hpeutab->cn_total; i++) {
        if (NULL == (pItem = HPEU_GetTabItem(hpeutab, i))) {
            ret = -EINVAL;
            goto l_fail;
        }

        if (pItem->cn_added == -1) {
            continue;
        }
        if ((ret = HPEU_get_eus(hpeutab, i, ar_peu)) != cn_hpeu_disk) {
            LOGWARN("skip id_hpeu %d due to incompleteness "
                    "(valid eu count in this hpeu is %d)\n", i, ret);
            continue;
        }
        LOGINFO("Check data for HPEU id = %d\n", i);
        if ((ret = HPEU_read_metadata(td, ar_peu, p_metadata, NULL)) < 0) {
            LOGWARN("HPEU_read_metadata error\n");
            goto l_fail;
        }
        for (j = 0; j < FPAGE_PER_SEU - METADATA_SIZE_IN_FLASH_PAGES; j++) {
            if (SPAREAREA_IS_EMPTY(p_metadata[0][j])) {
                break;
            }
        }
        cn_written_fpages = j;

        LOGINFO("written_page = %d , ar_peu[0]->start_pos %llx\n",
                cn_written_fpages, ar_peu[0]->start_pos);
        len = cn_hpeu_disk;
        status_ExtEcc.s_ExtEcc = 0;

        for (j = 0; j < cn_written_fpages; j++) {
            if (j == cn_written_fpages - 1) {
                fail_disk.idx[0] = -1;
                fail_disk.pg_id = PGROUP_ID(pItem->ar_peu[0]->disk_num);
                if (!stripe_locate
                    (td, ar_peu[0]->start_pos, p_metadata, &fail_disk, j, &len,
                     cn_hpeu_disk, &status_ExtEcc) || len == 0) {
                    LOGWARN("stripe_locate start pbno %llx error\n",
                            ar_peu[0]->start_pos +
                            (j << SECTORS_PER_FLASH_PAGE_ORDER));
                    continue;
                }
            }

            cn_diff +=
                rddcy_check_data_1(td, ar_peu, p_metadata, j, len, buf_fpage,
                                   buf_eccP, &status_ExtEcc);
            if (cn_diff > 0)
                goto l_success;
        }

    }
  l_success:
    ret = cn_diff;
  l_fail:
    track_kfree(ar_peu, cn_hpeu_disk * sizeof(struct EUProperty *), M_RECOVER);

    if (buf_eccP) {
        track_kfree(buf_eccP, FLASH_PAGE_SIZE, M_RECOVER);
    }

    for (i = 0; i < cn_hpeu_disk; i++) {
        if (buf_fpage[i])
            kunmap(dest_page[i]);
        if (dest_page[i])
            track_free_page(dest_page[i]);
    }
  l_free_lbno:
    for (i = 0; i < cn_hpeu_disk; i++) {
        if (p_metadata[i])
            track_kfree(p_metadata[i],
                        METADATA_SIZE_IN_FLASH_PAGES * FLASH_PAGE_SIZE,
                        M_RECOVER);
    }
    track_kfree(p_metadata, cn_hpeu_disk * sizeof(sector64 *), M_RECOVER);

    return ret;
}
