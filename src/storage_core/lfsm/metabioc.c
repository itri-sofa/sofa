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
#include "io_manager/biocbh.h"
#include "nextprepare.h"
#include "diskgrp.h"
#include "system_metadata/Dedicated_map.h"
#include "lfsm_core.h"
#include "GC.h"
#include "io_manager/io_common.h"
#include "bmt.h"
#include "metabioc.h"
#include "EU_access.h"
#include "io_manager/io_write.h"
#include "spare_bmt_ul.h"
#include "mdnode.h"

static int32_t metabioc_set_spare(struct bio_container *bio_ct, sector64 lbn)
{
    sector64 ar_lbn[METADATA_SIZE_IN_PAGES];
    int32_t i;

    for (i = 0; i < METADATA_SIZE_IN_PAGES; i++) {
        ar_lbn[i] = TAG_SPAREAREA_EMPTYPAGE;
    }
    //ar_lbn[0]=lbn;
    return update_bio_spare_area(bio_ct, ar_lbn, METADATA_SIZE_IN_PAGES);
}

static sector64 metabioc_CalcChkSumA(onDisk_md_t * pMetaPage, sector64 checksum,
                                     int32_t num)
{
    int32_t i;
    for (i = 0; i < num; i++) {
        checksum ^= (pMetaPage[i].lbno & (~LFSM_ALLOC_PAGE_MASK));
    }
    return checksum;
}

sector64 metabioc_CalcChkSumB(sector64 * ar_lbno, sector64 checksum,
                              int32_t num)
{
    int32_t i;
    for (i = 0; i < num; i++) {
        checksum ^= (ar_lbno[i] & (~LFSM_ALLOC_PAGE_MASK));
    }
    return checksum;
}

struct bio_container *metabioc_build(lfsm_dev_t * td, struct HListGC *h,
                                     sector64 pbno)
{
    struct EUProperty *eu;
    struct page *tmp_page;
    onDisk_md_t *pMetaPage;
    struct bio_container *pbioc;
    HPEUTabItem_t *pItem;
    sector64 checksum;
    int32_t i, cn_round;

    checksum = 0;

    if (pbno == 0) {
        return NULL;
    }

    if (NULL ==
        (pbioc =
         bioc_alloc(td, METADATA_SIZE_IN_PAGES, METADATA_SIZE_IN_PAGES))) {
        return NULL;
    }

    if (atomic_read(&pbioc->conflict_pages) != 0) {
        LOGWARN("new alloced bioc %p, but with a conflict counter= %d\n",
                pbioc, atomic_read(&pbioc->conflict_pages));
        return NULL;
    }

    init_bio_container(pbioc, NULL, NULL, end_bio_write, REQ_OP_WRITE, pbno,
                       METADATA_SIZE_IN_PAGES);
    eu = pbno_eu_map[(pbno - eu_log_start) >> EU_BIT_SIZE];
    metabioc_printk("%s build page start_pos=%llu pbno=%llu\n", __FUNCTION__,
                    eu->start_pos, pbno);
    for (i = 0; i < METADATA_SIZE_IN_PAGES; i++) {  //16
        tmp_page = pbioc->bio->bi_io_vec[i].bv_page;
        if (NULL == ((pMetaPage = kmap(tmp_page)))) {
            LOGERR("page is NULL!\n");
            goto l_fail;
        }
        metabioc_build_page(eu, pMetaPage, i == (METADATA_SIZE_IN_PAGES - 1),
                            i * METADATA_ENTRIES_PER_PAGE);

        cn_round = (i == (METADATA_SIZE_IN_PAGES - 1)) ?
            METADATA_ENTRIES_PER_PAGE -
            METADATA_SIZE_IN_FLASH_PAGES : METADATA_ENTRIES_PER_PAGE;
        checksum = metabioc_CalcChkSumA(pMetaPage, checksum, cn_round);
        if (i == METADATA_SIZE_IN_PAGES - 1) {
            pMetaPage[METADATA_ENTRIES_PER_PAGE -
                      METADATA_SIZE_IN_FLASH_PAGES].lbno = checksum;
            pMetaPage[METADATA_ENTRIES_PER_PAGE - 1].lbno =
                TAG_METADATA_WRITTEN;

            if (eu->id_hpeu == -2) {    // Ext Ecc
                pMetaPage[METADATA_ENTRIES_PER_PAGE - 2].lbno = eu->id_hpeu;
                pMetaPage[METADATA_ENTRIES_PER_PAGE - 3].lbno = 0;
            } else {
                if (NULL ==
                    (pItem = HPEU_GetTabItem(&td->hpeutab, eu->id_hpeu))) {
                    break;
                }
                pMetaPage[METADATA_ENTRIES_PER_PAGE - 2].lbno = eu->id_hpeu;
                pMetaPage[METADATA_ENTRIES_PER_PAGE - 3].lbno = pItem->birthday;
            }
        }
        kunmap(tmp_page);
    }

    if (metabioc_set_spare(pbioc, checksum) < 0) {
        goto l_fail;
    }

    return pbioc;

  l_fail:
    if (pbioc) {
        free_bio_container(pbioc, METADATA_SIZE_IN_PAGES);
        pbioc = NULL;
    }

    return (struct bio_container *)-EIO;
}

/*
** To form a page consisting of all the metadata information stored in the active_metadata list of the EU
** 
** @h : Garbage Collection structure
** @eu : The active EU to which the metadata is to be written
** @page : The page corresponding to the metadata information
**
** Returns void
*/
void metabioc_build_page(struct EUProperty *eu, onDisk_md_t * buf_lpn,
                         bool isTail, int32_t idx)
{
    memcpy(buf_lpn, &eu->act_metadata.mdnode->lpn[idx], PAGE_SIZE);

    if (isTail) {
        memset(&buf_lpn
               [METADATA_ENTRIES_PER_PAGE - METADATA_SIZE_IN_FLASH_PAGES],
               TAG_METADATA_EMPTYBYTE,
               METADATA_SIZE_IN_FLASH_PAGES * sizeof(onDisk_md_t));
    }
}

int32_t metabioc_save_and_free(lfsm_dev_t * td,
                               struct bio_container *metadata_bio_c,
                               bool isActive)
{
    struct EUProperty *eu_dest;
    int32_t ret, wait_ret, cnt;

    eu_dest = NULL;
    ret = 0;
    if (lfsm_readonly == 1) {
        goto func_end;
    }

    if (NULL == (eu_dest = eu_find(metadata_bio_c->bio->bi_iter.bi_sector))) {
        LOGERR("Fail to flush metadata for eu's metadata at %llx \n",
               (sector64) metadata_bio_c->bio->bi_iter.bi_sector);
        goto func_end;
    }

    metadata_bio_c->io_queue_fin = 0;

    if (my_make_request(metadata_bio_c->bio) < 0) {
        ret = 0;                // metadata write fail, we didn't handle it because metadata can be read from spare area
        LOGWARN("metadtata_bio_pos=%lld (un-shifted) w_fail\n",
                (sector64) metadata_bio_c->bio->bi_iter.bi_sector);
        goto func_end;
    }
    //err_handle_printk("wait_meta_data_bio_w,data_bio_c->bio->bi_sect %llu fin=%d\n",metadata_bio_c->bio->bi_iter.bi_sector, metadata_bio_c->io_queue_fin);
    //lfsm_wait_event_interruptible(metadata_bio_c->io_queue,
    //metadata_bio_c->io_queue_fin != 0);
    cnt = 0;
    while (true) {
        cnt++;
        wait_ret = wait_event_interruptible_timeout(metadata_bio_c->io_queue,
                                                    metadata_bio_c->
                                                    io_queue_fin != 0,
                                                    LFSM_WAIT_EVEN_TIMEOUT);
        if (wait_ret <= 0) {
            LOGWARN("metabioc IO no response %lld after seconds %d\n",
                    (sector64) metadata_bio_c->bio->bi_iter.bi_sector,
                    LFSM_WAIT_EVEN_TIMEOUT_SEC * cnt);
        } else {
            break;
        }
    }

    // printk("blocking wait: got a metabio %p %llu %llu\n",
    // metadata_bio_c,metadata_bio_c->start_lbno,metadata_bio_c->end_lbno);

    // eu =pbno_eu_map[(data_bio_c->bio->bi_iter.bi_sector - eu_log_start)>>EU_BIT_SIZE]; // move to the start of this function
    if (metadata_bio_c->status != BLK_STS_OK) {
        LOGWARN("metadtata_bio_pos=%lld (shifted) w_fail\n",
                (sector64) metadata_bio_c->bio->bi_iter.bi_sector);
        if (err_write_bioc_normal_path_handle(td, metadata_bio_c, true) < 0) {
            //0 : OK, continue , -1 : no ok, redo get dest_pbno_and finish_write,active eu is moved to falselist
            ret = 0;            // no need rewrite
            goto func_end;
        }
    }

  func_end:
    // An-nan, Tifa, Weafon 2011/5/20: we move LVP_heap_insert from get_dest_pbno and gc_get_dest_pbno to here
    // since GC may pick up a EU which metadata is not written completely if the EU is inserted by get_dest_pbno or gc_get_dest_pbno
    // Tifa, Weafon 2012/9/4: we move LVP_heap_insert to change_active_eu
    eu_dest->fg_metadata_ondisk = true; // indicate the metadata is committed no matter success or fail
    mdnode_free(&eu_dest->act_metadata, td->pMdpool);
    metadata_bio_c->io_queue_fin = 0;

    if (metadata_bio_c) {
        free_bio_container(metadata_bio_c, METADATA_SIZE_IN_PAGES);
        metadata_bio_c = NULL;
    }

    return ret;
}

static void metabioc_update_checksum(onDisk_md_t * buf_metadata)
{
    sector64 checksum;

    checksum = metabioc_CalcChkSumA(buf_metadata, 0, DATA_FPAGE_IN_SEU);
    buf_metadata[FPAGE_PER_SEU - METADATA_SIZE_IN_FLASH_PAGES].lbno = checksum;
    buf_metadata[FPAGE_PER_SEU - 1].lbno = TAG_METADATA_WRITTEN;
}

//calculate checksum before write metadata
bool metabioc_commit(lfsm_dev_t * td, onDisk_md_t * buf_metadata,
                     struct EUProperty *eu_new)
{
    sector64 ar_sparedata[METADATA_SIZE_IN_FLASH_PAGES];
    sector64 pbno_meta;
    int32_t i;

    pbno_meta = eu_new->start_pos + (SECTORS_PER_SEU - METADATA_SIZE_IN_SECTOR);
    for (i = 0; i < METADATA_SIZE_IN_FLASH_PAGES; i++) {
        ar_sparedata[i] = TAG_SPAREAREA_EMPTYPAGE;
    }

    metabioc_update_checksum(buf_metadata);
    ar_sparedata[0] =
        buf_metadata[FPAGE_PER_SEU - METADATA_SIZE_IN_FLASH_PAGES].lbno;

    if (!devio_write_pages(pbno_meta, (int8_t *) buf_metadata, ar_sparedata,
                           METADATA_SIZE_IN_HARD_PAGES)) {
        return false;
    }

    eu_new->fg_metadata_ondisk = true;  // indicate the metadata is correct write done and finish
    return true;
}

void metabioc_asyncload(lfsm_dev_t * td, struct EUProperty *peu,
                        devio_fn_t * pcb)
{
    sector64 pbn;
    int32_t ret;

    pbn = peu->start_pos + SECTORS_PER_SEU - METADATA_SIZE_IN_SECTOR;
    if ((ret = devio_async_read_pages(pbn, METADATA_SIZE_IN_FLASH_PAGES, pcb)) < 0) {   // read from metadata and isn't empty
        atomic_set(&pcb->result, DEVIO_RESULT_FAILURE); // as read fail;
        metabioc_printk("Warning@%s: read metadata pbn=%llu fail\n", pbn,
                        __FUNCTION__);
    }
}

// return true: load successfully
// return false: fail to load metadata or metadata is empty
bool metabioc_asyncload_bh(lfsm_dev_t * td, struct EUProperty *ar_peu,
                           sector64 * ar_lbno, devio_fn_t * pcb)
{
    sector64 checksum, pbn;

    pbn = ar_peu->start_pos + SECTORS_PER_SEU - METADATA_SIZE_IN_SECTOR;
    if (!devio_async_read_pages_bh
        (METADATA_SIZE_IN_FLASH_PAGES, (int8_t *) ar_lbno, pcb)) {
        if (!devio_reread_pages_after_fail(pbn, (int8_t *) ar_lbno, METADATA_SIZE_IN_FLASH_PAGES)) {    // do special query in this function
            return false;       // read metadata from spare area
        }
    }

    if (!metabioc_isempty(ar_lbno)) {
        return false;
    }

    ar_peu->fg_metadata_ondisk = true;
    if ((checksum = metabioc_CalcChkSumB(ar_lbno, 0, DATA_FPAGE_IN_SEU + 1)) != 0) {    // +1 is include checksum, and xor should be zero
        LOGERR("eu %lld checksum=%08llx should be 0!\n", ar_peu->start_pos,
               checksum);
        return false;
    }
    return true;
}

bool metabioc_isempty(sector64 * ar_lbno)
{
    if (ar_lbno[FPAGE_PER_SEU - 1] != TAG_METADATA_WRITTEN) {
        metabioc_printk("Info@%s: metadata ar_lbno %llu is empty\n",
                        __FUNCTION__, ar_lbno[FPAGE_PER_SEU - 1]);
        return false;
    }
    return true;
}
