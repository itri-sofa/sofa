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
/*
** This file constitues of all the functions related to BMT
** Creating BMT, bitmap and frontiers
** Lookup, signature sector and per-page-queue update
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
#include "bmt_commit.h"
#include "io_manager/io_common.h"
#include "bmt_ppq.h"
#include "bmt_ppdm.h"
#include "bmt.h"
#include "EU_access.h"
#include "EU_create.h"
#include "GC.h"
#include "system_metadata/Dedicated_map.h"
#include "system_metadata/Super_map.h"
#include "special_cmd.h"
#include "erase_count.h"
#include "devio.h"
#include "spare_bmt_ul.h"
#include "hpeu.h"
#include "hpeu_recov.h"
#include "io_manager/io_write.h"

static int32_t lfsm_load_ECT(lfsm_dev_t * td, sddmap_disk_hder_t * prec)
{
    struct HListGC *h;
    erase_head_t *perasemap;
    int32_t i;

    for (i = 0; i < td->cn_ssds; i++) {
        perasemap = ddmap_get_erase_map(prec, i);
        if (perasemap->disk_num < 0 || perasemap->disk_num >= td->cn_ssds) {
            LOGERR("err: disk num is out of bound\n");
            return -EFAULT;
        }

        if (perasemap->disk_num != i) {
            LOGERR("err: disk_num[%d]=%d is not matched!\n", i,
                   perasemap->disk_num);
            return -EFAULT;
        }

        h = gc[i];
        if (IS_ERR_OR_NULL(h->ECT.eu_curr =
                           FreeList_delete_by_pos(td, perasemap->start_pos,
                                                  EU_ECT))) {
            LOGERR("err: h->ECT.eu_curr<0 fail disknum %d\n", i);
            return -EFAULT;
        }

        h->ECT.eu_curr->log_frontier = perasemap->frontier;
//        LOGINFO("erase_map[%d]=%llus frontier %ds\n",i,perasemap->start_pos,h->ECT.eu_curr->log_frontier);
    }

    return ECT_load_all(td);
}

// Tifa:
//   if out off boundary, and then assert
//   return 0 is in boundary, else is out off boundary
int32_t PbnoBoundaryCheck(lfsm_dev_t * td, sector64 pbno_over_disk)
{
    sector64 pbno_in_disk;
    int32_t dev_id, ret;

    ret = 0;

    pbno_lfsm2bdev(pbno_over_disk, &pbno_in_disk, &dev_id);

    do {
        if (pbno_over_disk < td->start_sector) {
            LOGWARN("pbno_over_disk: %llu lower-bound of whole disk\n",
                    pbno_over_disk);
            ret = -EFAULT;
            break;
        }

        if (pbno_over_disk >= grp_ssds.capacity) {
            LOGWARN("pbno_over_disk: %llu over-bound of whole disk\n",
                    pbno_over_disk);
            ret = -EFAULT;
            break;
        }

        if (pbno_in_disk < grp_ssds.ar_subdev[dev_id].log_start) {
            LOGWARN("In disk%d pbno %llu lower-bound\n", dev_id, pbno_in_disk);
            ret = -EFAULT;
            break;
        }

        if (pbno_in_disk >= grp_ssds.cn_sectors_per_dev) {
            LOGWARN("In disk%d pbno %llu over-bound\n", dev_id, pbno_in_disk);
            ret = -EFAULT;
            break;
        }
    } while (0);

    return ret;
}

static int32_t bmt_lookup_adv_test(lfsm_dev_t * td, sector64 pbno)
{
    int32_t id_dev;
    if (PbnoBoundaryCheck(td, pbno)) {
        return -EFAULT;
    }
//to FIXME: for bad disk return
    if (!pbno_lfsm2bdev_A(pbno, 0, &id_dev, &grp_ssds)) {
        return -ENOEXEC;
    }

    if (grp_ssds.ar_subdev[id_dev].load_from_prev != all_ready) {   //==LFSM_BAD_DISK)
        switch (HPEU_get_state(&td->hpeutab, pbno)) {
        case good:
            break;
        case damege:
            return -EACCES;
        default:
            return -EINVAL;
        }
    }

    if ((pbno % SECTORS_PER_FLASH_PAGE) != 0) {
        LOGERR("not aligned ret_pbn = %llu\n", pbno);
        return -EFAULT;
    }

    return 0;
}

static int32_t rebuild_bitmap(lfsm_dev_t * td, struct EUProperty **pbno_eu_map)
{
    struct D_BMT_E *bmt_rec;
    struct EUProperty *eu;
    MD_L2P_table_t *bmt;
    int32_t i, j, ret;

    bmt = td->ltop_bmt;
    bmt_rec =
        (struct D_BMT_E *)track_kmalloc(FLASH_PAGE_SIZE, __GFP_ZERO,
                                        M_BMT_CACHE);

    for (i = 0; i < bmt->cn_ppq; i++) {
        //read page_bmt from bmt eu
        memset(bmt_rec, 0, FLASH_PAGE_SIZE);

        if (bmt->per_page_queue[i].pDbmtaUnit) {    // means the data is newer in mem
            for (j = 0; j < DBMTE_NUM; j++) {
                bmt_rec[j].pbno = (sector64)
                    (bmt->per_page_queue[i].pDbmtaUnit->BK_dbmtE[j].pbno);
            }
        } else {
            if (bmt->per_page_queue[i].addr_ondisk == PAGEBMT_NON_ONDISK) {
                continue;
            }

            if (PPDM_BMT_load(td, i, (int8_t *) bmt_rec) < 0) {
                ret = -EIO;
                LOGERR("devio_read_pages fail (ret=%d)\n", ret);
                goto l_fail;
            }
        }

        //rebuild the bitmap
        for (j = 0; j < FLASH_PAGE_SIZE / sizeof(struct D_BMT_E); j++) {
            if (bmt_rec[j].pbno == PBNO_SPECIAL_NEVERWRITE) {
                continue;
            }

            if ((ret = bmt_lookup_adv_test(td, bmt_rec[j].pbno)) < 0) {
                if (ret != -EACCES) {
                    LOGERR
                        ("(%d) idx:%d, ret_pbno:0x%08llx capacity:%llu log_start:%llu bmt_pos %llu\n",
                         ret, j, (sector64) bmt_rec[j].pbno, grp_ssds.capacity,
                         td->start_sector,
                         bmt->per_page_queue[i].
                         addr_ondisk << SECTORS_PER_FLASH_PAGE_ORDER);
                    goto l_fail;
                }
                continue;
            }
            //printk("(pbno,lbno)=(%llu, %lu)\n",bmt_rec[j].pbno,i*(PAGE_SIZE/sizeof(struct D_BMT_E))+j);

            if (IS_ERR_OR_NULL(eu = eu_find(bmt_rec[j].pbno))) {
                LOGERR("pbno_eu_map eu=NULL\n");
                ret = -EINVAL;
                goto l_fail;
            }

            set_bit(((bmt_rec[j].pbno - eu->start_pos - DUMMY_SIZE_IN_SECTOR)
                     >> SECTORS_PER_FLASH_PAGE_ORDER) % PAGES_PER_BYTE,
                    (unsigned long *)&eu->
                    bit_map[(bmt_rec[j].pbno - eu->start_pos -
                             DUMMY_SIZE_IN_SECTOR) >>
                            SECTORS_PER_FLASH_PAGE_ORDER >>
                            PAGES_PER_BYTE_ORDER]);
            atomic_add(SECTORS_PER_FLASH_PAGE, &eu->eu_valid_num);
            atomic64_inc(&td->ar_pgroup[PGROUP_ID(eu->disk_num)].cn_valid_page);
        }
    }
    ret = 0;

  l_fail:
    track_kfree(bmt_rec, FLASH_PAGE_SIZE, M_BMT_CACHE);
    return ret;
}

// for debug shown in procfs
int32_t bmt_dump(MD_L2P_table_t * bmt, int8_t * buf)
{
    int32_t a, b, len;

    a = atomic64_read(&bmt->cn_lookup_query);
    b = atomic_read(&bmt->cn_lookup_missing);
    len = sprintf(buf, "bmt lookup: total= %d missing= %d\n", a, b);
    return len;
}

static bool bmt_disk_reload(lfsm_dev_t * td, MD_L2P_table_t * bmt,
                            sector64 idx_ppq)
{
    int8_t *buf;
    int32_t cn_records;
    bool ret;

    if (IS_ERR_OR_NULL
        (buf = track_kmalloc(FLASH_PAGE_SIZE, GFP_KERNEL, M_OTHER))) {
        return false;
    }

    if (PPDM_BMT_load(td, idx_ppq, buf) < 0) {
        goto l_end;
    }

    cn_records = FLASH_PAGE_SIZE / sizeof(struct D_BMT_E);
    PPDM_BMT_cache_insert_nonpending(td, bmt, (struct D_BMT_E *)buf,
                                     cn_records, idx_ppq * cn_records);
    ret = true;

  l_end:
    track_kfree(buf, FLASH_PAGE_SIZE, M_OTHER);
    return ret;
}

/*
** This is the main handler for the BMT lookup
** For a given LBN, check is first done in the BMT cache
** If cache misses, we have to read from disk
** If a sector is brought in during on-disk BMT read, we update the BMT cache with the extra new records
** 2011.10 modify sector-based unit of read_small_disk_io_temp into page-based unit
**
** @td : lfsm_dev_t object
** @bmt : BMT object
** @lbn : LBN for which PBN is sought (unit = page)
** @to_pin_ppq : True if the ppq's dbmta will be used again soon, e.g. the lookup is requested by a write cmd (get_dest_pbno)
** Returns the PBN in sector
*/

int32_t ppq_alloc_reload(MD_L2P_item_t * ppq, lfsm_dev_t * td,
                         MD_L2P_table_t * bmt, int32_t idx_ppq)
{
    if (!IS_ERR_OR_NULL(ppq->pDbmtaUnit)) {
        LOGERR("Internal# ppq[%d] pDbmtaUnit should be zero\n", idx_ppq);
        return -EPERM;
    }

    ppq->pDbmtaUnit = dbmtaE_BK_alloc(&td->dbmtapool);
    if (IS_ERR_OR_NULL(ppq->pDbmtaUnit)) {
        return -EAGAIN;
    }

    if (!bmt_disk_reload(td, bmt, idx_ppq)) {
        return -ENXIO;
    }

    return 0;
}

static int32_t ppq_reload_and_query(lfsm_dev_t * td, MD_L2P_table_t * bmt,
                                    MD_L2P_item_t * ppq,
                                    struct D_BMT_E *pret_dbmta, int32_t idx_ppq,
                                    int32_t idx_dbmta)
{
    int32_t ret;

    if ((ret = ppq_alloc_reload(ppq, td, bmt, idx_ppq)) < 0) {
        return ret;
    }

    if ((pret_dbmta->pbno = ppq->pDbmtaUnit->BK_dbmtE[idx_dbmta].pbno) != PBNO_SPECIAL_NEVERWRITE) {    //ret_pbno==0 means never write
        if (PbnoBoundaryCheck(td, pret_dbmta->pbno)) {
            return -EFAULT;
        }
    }

    return 0;
}

// Weafon said (20120718):
// The basic function of bmt_lookup is to return ppn for a lpn.
// However, since the function also guarantee to load back the dbmtas from disk when on-disk dbmtas exist,
// Tifa and Annan called the function to ensure the on-mem dbmtas must be allocated and loaded if existing.
// In fact, It should be OK since PPDM_BMT_cache_lookup should not cost too much computing while new function may be too similar to old one.
int32_t bmt_lookup(lfsm_dev_t * td, MD_L2P_table_t * bmt, sector64 lpn,
                   bool incr_acc_count, int32_t cn_pin_ppq,
                   struct D_BMT_E *pret_dbmta)
{
    MD_L2P_item_t *ppq;
    uint64_t _q_no, _rem, _pos;
    int32_t ret, record_num_per_page;
    uint64_t fake_pbn;

    memset(pret_dbmta, 0, sizeof(struct D_BMT_E));
    if (lpn >= td->logical_capacity) {
        LOGERR("Internal lpn %llx\n", lpn);
        return -EPERM;
    }

    record_num_per_page = FLASH_PAGE_SIZE / sizeof(struct D_BMT_E);
    ret = 0;
    _pos = lpn;
    _rem = do_div(_pos, record_num_per_page);
    _q_no = _pos;
    //printk("%s : lbn=%lld qno=%d pin=%d\n",__FUNCTION__,lbn,(int)_q_no, cn_pin_ppq);
    if (_q_no >= bmt->cn_ppq) {
        LOGERR("Internal q_no %llu > %d\n", _q_no, bmt->cn_ppq);
        return -EPERM;
    }

  l_restart:
    ppq = &bmt->per_page_queue[_q_no];
    LFSM_MUTEX_LOCK(&ppq->ppq_lock);    //TODO: we should use rw-lock
    /* Look in cache first */
    PPDM_BMT_cache_lookup(bmt, lpn, incr_acc_count, pret_dbmta);

    //TODO: we should no need check this at release version
    if (pret_dbmta->pbno % SECTORS_PER_FLASH_PAGE &&
        pret_dbmta->pbno != PBNO_SPECIAL_NOT_IN_CACHE
        && pret_dbmta->fake_pbno == 0) {
        LOGERR("PPDM: not aligned for lbn %llu ret_pbn = %llu\n", lpn,
               (sector64) pret_dbmta->pbno);
    }

    if (pret_dbmta->pbno == PBNO_SPECIAL_NOT_IN_CACHE) {
        atomic_inc(&bmt->cn_lookup_missing);
        switch (ret =
                ppq_reload_and_query(td, bmt, ppq, pret_dbmta, _q_no, _rem)) {
        case -EAGAIN:
            LFSM_MUTEX_UNLOCK(&(ppq->ppq_lock));
            //TODO: FSS-108: we should let gcwrite thread (sflush)
            if (IS_GC_THREAD(current->pid)) {
                PPDM_BMT_dbmta_return(td, bmt,
                                      (atomic_read(&td->dbmtapool.dbmtE_bk_used)
                                       + DBMTAPOOL_ALLOC_HIGHNUM -
                                       td->dbmtapool.max_bk_size));
                goto l_restart;
            }
            wake_up_interruptible(&td->worker_queue);
            lfsm_wait_event_interruptible(td->gc_conflict_queue,
                                          atomic_read(&td->dbmtapool.
                                                      dbmtE_bk_used) <
                                          td->dbmtapool.max_bk_size);
            goto l_restart;
        case 0:
            break;
        default:
            LFSM_MUTEX_UNLOCK(&(ppq->ppq_lock));
            return ret;
        }
    }
    // we inc the counter to prevent its dbmta freed due to insufficient dbmta.
    // The counter will be dec in PPDM_BMT_cache_insert_one_pending
    // Notably, the counter is inc even dbmta is null for now.
    if (cn_pin_ppq > 0) {
//        printk("%s:%d null ppq(%d)->cn_bioc_pins (%d)++\n",__FUNCTION__,__LINE__,(int)_q_no,ppq->cn_bioc_pins);
        ppq->cn_bioc_pins += cn_pin_ppq;
    }
//    printk("%s:%d wf_ppqlock: ppq[%llu] unlock %p\n",__FILE__,__LINE__,
//        _q_no,&(bmt->per_page_queue[_q_no].ppq_lock));
    LFSM_MUTEX_UNLOCK(&(ppq->ppq_lock));
    atomic64_inc(&bmt->cn_lookup_query);

    if (pret_dbmta->fake_pbno) {
        fake_pbn = (uint64_t) pret_dbmta->pbno;
        if ((ret = bmt_lookup(td, bmt, fake_pbn, false, 1, pret_dbmta)) < 0) {
            LOGERR("Fail to bmt lookup for lpn: %lld\n", lpn);
            return ret;
        }
        PPDM_BMT_cache_remove_pin(bmt, fake_pbn, 1);    //only remove fake pbn pin, first ppq pin will be rmoved in inser_one_pending
    }

    switch (pret_dbmta->pbno) {
    case PBNO_SPECIAL_NOT_IN_CACHE:
    case PBNO_SPECIAL_BAD_PAGE:
    case PBNO_SPECIAL_NEVERWRITE:
        pret_dbmta->pbno = PBNO_SPECIAL_NEVERWRITE;
        break;
    default:
        if ((ret = bmt_lookup_adv_test(td, pret_dbmta->pbno)) < 0) {
            if (ret != -EACCES) {
                LOGERR("(%d) lpn:%llu ret_pbno:0x%08llx capacity:%llu "
                       "log_start:%llu bmt_pos %llu \n",
                       ret, lpn, (sector64) pret_dbmta->pbno,
                       grp_ssds.capacity, td->start_sector,
                       _pos << SECTORS_PER_FLASH_PAGE_ORDER);
            }

            return ret;
        }
        break;
    }

    return 0;
}

static bool is_data_write(struct bio_container *pBioc)
{
    if (pBioc->start_lbno == LBNO_PREPADDING_PAGE) {
        return false;
    }

    if (RDDCY_IS_ECCLPN(pBioc->start_lbno)) {
        return false;
    }

    if ((pBioc->dest_pbn & MAPPING_TO_HDD_BIT) > 0) {
        return false;
    }

    return true;
}

/*
 * Description: 20170307-lego
 *     I use a compile flag EU_BITMAP_REFACTOR_PERF in this function.
 *     (See JIRA FSS-104 and FSS-64)
 *     Original idea is refactor for improving performance by saving 1 spinlock and 1 bit operation.
 *     But when aplly this version to test sofa over iSCSI performance, we only get 830 ~ 890 KIOPS.
 *     If restore to original way, we can get avg 960 ~ 980 KIOPS at 40 vcores platform.
 *     It is a tricky point that improve performance but sofa over iSCSI performance degrade.
 *
 */
static int32_t set_clear_PBN_bitmap_validcount(sector64 pbno, bool set_bitmap)
{
    struct EUProperty *e;
    void *addr;
    int32_t ret, nr;

    ret = 0;
    e = pbno_eu_map[(pbno - eu_log_start) >> EU_BIT_SIZE];

    nr = ((pbno - e->start_pos - DUMMY_SIZE_IN_SECTOR) >> SECTORS_PER_FLASH_PAGE_ORDER) % PAGES_PER_BYTE;   // bit location

    addr = (void *)&e->bit_map[(pbno - e->start_pos - DUMMY_SIZE_IN_SECTOR) >> SECTORS_PER_FLASH_PAGE_ORDER >> PAGES_PER_BYTE_ORDER];   // bit address

    LFSM_ASSERT(pbno >= e->start_pos);
    LFSM_ASSERT(pbno - e->start_pos + METADATA_SIZE_IN_SECTOR -
                DUMMY_SIZE_IN_SECTOR <= e->eu_size);

#ifndef EU_BITMAP_REFACTOR_PERF
    spin_lock(&e->lock_bitmap);

    if (test_bit(nr, addr)) {
        if (!set_bitmap) {
            LFSM_ASSERT2(atomic_read(&e->eu_valid_num) > 0,
                         "eu(%p) pbno %llu id_hpeu %d\n", e, pbno, e->id_hpeu);

            //if(atomic_read(&e->eu_valid_num) == 0)
            //LOGWARN("eu(%p) pbno %llu id_hpeu %d\n",e, pbno, e->id_hpeu);
            __clear_bit(nr, addr);
            atomic_sub(SECTORS_PER_FLASH_PAGE, &e->eu_valid_num);
            atomic64_dec(&lfsmdev.ar_pgroup[PGROUP_ID(e->disk_num)].
                         cn_valid_page);
            atomic64_dec(&lfsmdev.ltop_bmt->cn_valid_page_ssd);
        } else {
            // original bit =1, and wait to set bit --> assert
            // LFSM_ASSERT2(false, "%lld sector has been set to one\n",pbno);
            LOGERR("%lld sector has been set to one\n", pbno);
            //lfsm_write_pause();
            //ret = -EPERM;
        }
    } else {
        if (set_bitmap) {
            __set_bit(nr, addr);
            atomic_add(SECTORS_PER_FLASH_PAGE, &e->eu_valid_num);
            atomic64_inc(&lfsmdev.ar_pgroup[PGROUP_ID(e->disk_num)].
                         cn_valid_page);
            atomic64_inc(&lfsmdev.ltop_bmt->cn_valid_page_ssd);
        } else {
            //LFSM_ASSERT2(false, "%lld sector has been set to zero\n",pbno);
            LOGERR("%lld sector has been set to zero\n", pbno);
            //lfsm_write_pause();
            //ret = -ENOENT;
            //LOGWARN("eu(%p) %llu pbno %llu id_hpeu %d is zero\n",e,e->start_pos, pbno, e->id_hpeu);
        }
    }

    spin_unlock(&e->lock_bitmap);
#else

    if (set_bitmap) {
        if (unlikely(test_and_set_bit(nr, addr))) {
            // original bit =1, and wait to set bit --> assert
            // LFSM_ASSERT2(false, "%lld sector has been set to one\n",pbno);
            LOGERR("%lld sector has been set to one\n", pbno);
            //lfsm_write_pause();
            //ret = -EPERM;
        } else {
            atomic_add(SECTORS_PER_FLASH_PAGE, &e->eu_valid_num);
            atomic64_inc(&lfsmdev.ar_pgroup[PGROUP_ID(e->disk_num)].
                         cn_valid_page);
            atomic64_inc(&lfsmdev.ltop_bmt->cn_valid_page_ssd);
        }
    } else {
        if (likely(test_and_clear_bit(nr, addr))) {
            atomic_sub(SECTORS_PER_FLASH_PAGE, &e->eu_valid_num);
            atomic64_dec(&lfsmdev.ar_pgroup[PGROUP_ID(e->disk_num)].
                         cn_valid_page);
            atomic64_dec(&lfsmdev.ltop_bmt->cn_valid_page_ssd);
        } else {
            //LFSM_ASSERT2(false, "%lld sector has been set to zero\n",pbno);
            LOGERR("%lld sector has been set to zero\n", pbno);
            //lfsm_write_pause();
            //ret = -ENOENT;
            //LOGWARN("eu(%p) %llu pbno %llu id_hpeu %d is zero\n",e,e->start_pos, pbno, e->id_hpeu);
        }
    }
#endif

    return ret;
}

static int32_t set_PBN_bitmap_validcount(sector64 pbno)
{
    return set_clear_PBN_bitmap_validcount(pbno, true);
}

int32_t clear_PBN_bitmap_validcount(sector64 pbno)
{
    return set_clear_PBN_bitmap_validcount(pbno, false);
}

int32_t update_ppq_and_bitmap(lfsm_dev_t * td, struct bio_container *pBioc)
{
    sector64 pbn_source, pbn_dest, lpn;
    int32_t i, cn;

    cn = bioc_cn_fpage(pBioc);
    for (i = 0; i < cn; i++) {
        pbn_dest = bioc_get_psn(pBioc, i);
        pbn_source = bioc_get_sourcepsn(pBioc, i);
        lpn = pBioc->start_lbno + i;
        if (!is_data_write(pBioc) || pBioc->status != BLK_STS_OK) {
            if (pBioc->status != BLK_STS_OK) {
                LOGINFO("handle %p page %d, status != BLK_STS_OK\n", pBioc, i);
            }

            if (HPEU_dec_unfinish(&td->hpeutab, pbn_dest) < 0) {
                return -EACCES;
            }
            continue;
        }

        if (set_PBN_bitmap_validcount(pbn_dest) < 0) {
            //LFSM_ASSERT2(0, "set pbno valid %p tp %d lpn %lld %d pbn %lld->%lld\n",pBioc,pBioc->write_type,lpn,i,pbn_source,pbn_dest);   // AN: 2013/10/14 come from previos eu_alloc_page
            LOGWARN("fail to set %p tp %d lpn %lld %d pbn %lld->%lld\n",
                    pBioc, pBioc->write_type, lpn, i, pbn_source, pbn_dest);
        }

        if (HPEU_dec_unfinish(&td->hpeutab, pbn_dest) < 0) {
            return -EACCES;
        }

        PPDM_BMT_cache_insert_one_pending(td, lpn, pbn_dest, 1, true, 0, false);

        diskgrp_cleanlog(&grp_ssds, eu_find(pbn_dest)->disk_num);   // clean err counter since new io complete successfully

//        printk("BIT PPQ: {src,dest} = {%llu, %llu} vn %d\n",pbn_source, pbn_dest, atomic_read(&eu_find(pbn_dest)->eu_valid_num));
        if (pBioc->write_type != TP_GC_IO) {
            if (update_old_eu(NULL, 1, &pbn_source) < 0) {
                //LFSM_ASSERT2(0, "pbioc %p lpn %lld %d pbn %lld->%lld\n",pBioc,lpn,i,pbn_source,pbn_dest);
                LOGWARN("pbioc %p lpn %lld %d pbn %lld->%lld\n",
                        pBioc, lpn, i, pbn_source, pbn_dest);
            }
        } else {
            if (clear_PBN_bitmap_validcount(pbn_source) < 0) {
                LOGWARN("pbioc %p lpn %lld pbn %lld->%lld\n", pBioc, lpn,
                        pbn_source, pbn_dest);
            }
        }

        if (IS_ERR_OR_NULL(pBioc->par_bioext)) {
            break;
        }
    }

    return 0;
}

//the latest verson adds the RELOAD mechanism by using bmt_lookup while in GC thread
int32_t per_page_queue_update(lfsm_dev_t * td, sector64 * lbno, int32_t size,
                              sector64 * pbno, enum EDBMTA_OP_DIRTYBIT op_dirty)
{
    int32_t i, st_index, conti_size;

    st_index = 0;
    conti_size = 0;
    for (i = 0; i < size; i++) {
        if (conti_size == 0) {
            st_index = i;
            conti_size++;
        }

        if ((i + 1) != size && (lbno[i + 1] == lbno[i] + 1)
            && ((lbno[i] % 512) != 511)) {
            conti_size++;
            continue;
        }
//        printk("per_page_queue_update: lbn=%llu pbn=%llu\n",lbno[st_index], pbno[st_index]);
        PPDM_BMT_cache_insert_one_pending(td, lbno[st_index], pbno[st_index],
                                          conti_size, true, op_dirty, false);
        conti_size = 0;
    }

    return 0;
}

static void calc_bitmap_location(sector64 pbno, void **ret_addr,
                                 int32_t * ret_nr)
{
    struct EUProperty *e;

    e = pbno_eu_map[(pbno - eu_log_start) >> EU_BIT_SIZE];
    *ret_nr = ((pbno - e->start_pos - DUMMY_SIZE_IN_SECTOR) >> SECTORS_PER_FLASH_PAGE_ORDER) % PAGES_PER_BYTE;  // bit location

    *ret_addr = (void *)&e->bit_map[(pbno - e->start_pos - DUMMY_SIZE_IN_SECTOR)
                                    >> SECTORS_PER_FLASH_PAGE_ORDER >> PAGES_PER_BYTE_ORDER];   // bit address
}

int32_t test_pbno_bitmap(sector64 pbno)
{
    void *addr;
    int32_t nr;

    calc_bitmap_location(pbno, &addr, &nr);

    return test_bit(nr, addr);
}

void reset_bitmap_ppq(sector64 pbno_clear, sector64 pbno_set, sector64 lpn,
                      lfsm_dev_t * td)
{
    set_PBN_bitmap_validcount(pbno_set);
    PPDM_BMT_cache_insert_one_pending(td, lpn, pbno_set, 1, false, DIRTYBIT_SET,
                                      false);
    clear_PBN_bitmap_validcount(pbno_clear);
}

// For each logical EU of BMT, sync the contents between its main eu and backup eu
bool BMT_rescue(lfsm_dev_t * td, MD_L2P_table_t * bmt)
{
    int32_t diskid_main_eu, diskid_bkup_eu, cn_page, cn_total, cn_round;
    bool ret, isAppend;

    if (lfsm_readonly == 1) {
        return true;
    }

    isAppend = false;
    cn_round = 0;
    ret = true;
    LFSM_MUTEX_LOCK(&bmt->cmt_lock);

    // AN 2012:1022 rescue BMT*gc, only need to make sure not a band EU
    bmt->BMT_EU_backup_gc = EU_EnsureUsable(bmt->BMT_EU_backup_gc);

    bmt->BMT_EU_gc = EU_EnsureUsable(bmt->BMT_EU_gc);

    if (bmt->pagebmt_cn_used >= (FPAGE_PER_SEU * bmt->BMT_num_EUs)) {
        LOGERR
            ("bmt->pagebmt_cn_used : %d, (FPAGE_PER_SEU*bmt->BMT_num_EUs) %ld\n",
             bmt->pagebmt_cn_used, (FPAGE_PER_SEU * bmt->BMT_num_EUs));
        goto l_end;
    }

    cn_total = bmt->cn_ppq + bmt_cn_fpage_valid_bitmap(bmt);    // must search all page in ppq table

    while (cn_total > 0) {
        diskid_main_eu = bmt->BMT_EU[cn_round]->disk_num;
        diskid_bkup_eu = bmt->BMT_EU_backup[cn_round]->disk_num;

        cn_page = (cn_total > FPAGE_PER_SEU) ? FPAGE_PER_SEU : cn_total;

        if (!DEV_SYS_READY(grp_ssds, diskid_main_eu)
            || !DEV_SYS_READY(grp_ssds, diskid_bkup_eu)) {

#if FPAGE_PER_SEU%8 != 0
#error "AN say that : the AR_PAGEBMT is changed to bitmap, the EU_rescue cannot support FPAGE_PER_SEU is not divide by 8\n"
#endif
            if (!(ret =
                  EU_rescue(td, &bmt->BMT_EU[cn_round],
                            &bmt->BMT_EU_backup[cn_round], cn_page, isAppend,
                            &(bmt->
                              AR_PAGEBMT_isValidSegment[(cn_round *
                                                         FPAGE_PER_SEU) /
                                                        8])))) {
                goto l_end;
            }
        }
        cn_total -= cn_page;
        cn_round++;
    }

  l_end:

    LFSM_MUTEX_UNLOCK(&bmt->cmt_lock);

    return ret;
}

int32_t bmt_cn_fpage_valid_bitmap(MD_L2P_table_t * bmt)
{
    return (LFSM_CEIL_DIV
            (LFSM_CEIL_DIV(bmt->cn_ppq * FACTOR_PAGEBMT2PPQ, 8),
             FLASH_PAGE_SIZE));
}

int32_t bmt_sz_ar_ValidSegment(MD_L2P_table_t * bmt)
{
    return (LFSM_MAX(((bmt->BMT_num_EUs * FPAGE_PER_SEU) / 8),
                     (bmt_cn_fpage_valid_bitmap(bmt) * FLASH_PAGE_SIZE)));
    // AN: this data struct is used both for copy and commit into bmt table bitmap, so we must choose the larger size
}

/*
 * Description : generate bitmap
 * return true : gen fail
 *        false: gen OK
 */
bool generate_bitmap_and_validnum(lfsm_dev_t * td,
                                  struct EUProperty **pbno_eu_map)
{
    bool ret;

    ret = false;
    switch (td->prev_state) {
    case sys_loaded:
        LOGINFO("Last remove lfsm is fail. Recovery SOFA\n");

        if (metalog_load_all(&td->mlogger, td) < 0) {
            LOGERR("gen bitmap fail load metalog fail\n");
            ret = true;
            break;
        }

        if (!HPEUrecov_crash_recovery(td)) {
            LOGERR("both updatelog infomation missing, can't recovery\n");
            mdnode_free_all(td);
            ret = true;
            break;
        }

        mdnode_free_all(td);

        // Annan?: postpone the commit until free list
        //if(update_ondisk_BMT(td,td->ltop_bmt,1)==false) //Reuse =1 as free_list is not yet available
        //    goto l_fail;
/*
        // FIXME@weafon : should integrate into HPEU_load and should keep the next in ddmap.
        HPEU_change_next(&td->hpeutab);
        HPEU_sync_all_eus(&td->hpeutab);
        if(HPEU_save(&td->hpeutab))
            goto l_fail;
*/
        //if(ECT_commit_all(td,true)==false)
        //    goto l_fail;

        if (rebuild_bitmap(td, pbno_eu_map) < 0) {
            ret = true;
            LOGERR("rebuild bitmap fail prev state %d\n", td->prev_state);
            break;
        }

        break;
    case sys_unloaded:
        LOGINFO("Last remove lfsm is success. Rebuild bitmap \n");
        HPEU_sync_all_eus(&td->hpeutab);
        if (rebuild_bitmap(td, pbno_eu_map) < 0) {
            ret = true;
            LOGERR("rebuild bitmap fail prev state %d\n", td->prev_state);
            break;
        }

        break;
    case sys_fresh:
        LOGINFO("Do nothing for bitmap when fresh disk lfsm\n");
        break;
    default:
        LOGERR("gen bitmap fail unknown system prev state %d\n",
               td->prev_state);
        ret = true;
        break;
    }

    return ret;
}

/*
** This function is used to construct/re-construct the freemap and get frontier information
** Called during the initialization of the driver
** It reads the signature sector and decides the status of the previous driver usage
** If fresh disk:
**     Assigns all zeros to freemap and new active eus
** If already used and driver was properly unloaded previously:
**     Reads entire BMT to get the bitmap. Reads signature sector to get the previous frontier information
** If already used and system crashed during previous usage:
**    Do crash recovery. Frontier information will be lost. Thats fine. Use new active eus
**
** @td : lfsm_dev_t object
** @h : GC object
** @i : subdevice ID
** @bmt : BMT object
**
** Returns void
*/
int32_t BMT_setup_for_scale_up(lfsm_dev_t * td, MD_L2P_table_t * bmt,
                               MD_L2P_table_t * bmt_orig,
                               int32_t BMT_num_EUs_new,
                               int32_t BMT_num_EUs_orig, int32_t cn_ssd_orig)
{
    struct HListGC *h;
    struct EUProperty *retEU;
    int32_t i;

    bmt->pagebmt_cn_used = bmt_orig->pagebmt_cn_used;
    bmt->fg_gceu_dirty = bmt_orig->fg_gceu_dirty;
    atomic64_set(&bmt->cn_lookup_query,
                 atomic64_read(&bmt_orig->cn_lookup_query));
    atomic64_set(&bmt->cn_valid_page_ssd,
                 atomic64_read(&bmt_orig->cn_valid_page_ssd));
    atomic64_set(&bmt->cn_dirty_page_ssd,
                 atomic64_read(&bmt_orig->cn_dirty_page_ssd));
    atomic_set(&bmt->cn_lookup_missing,
               atomic_read(&bmt_orig->cn_lookup_missing));

    //memcpy(bmt->AR_PAGEBMT_isValidSegment, bmt_orig->AR_PAGEBMT_isValidSegment, bmt_sz_ar_ValidSegment(bmt_orig));

    memcpy(bmt->BMT_EU, bmt_orig->BMT_EU,
           BMT_num_EUs_orig * sizeof(struct EUProperty *));
    memcpy(bmt->BMT_EU_backup, bmt_orig->BMT_EU_backup,
           BMT_num_EUs_orig * sizeof(struct EUProperty *));

    for (i = 0; i < BMT_num_EUs_orig; i++) {
        //bmt->BMT_EU[i]=bmt_orig->BMT_EU[i];
        //bmt->BMT_EU_backup[i] = bmt_orig->BMT_EU_backup[i];
        LOGINFO("%dth main: %llu\n", i, bmt->BMT_EU[i]->start_pos);
        LOGINFO("%dth bkup: %llu\n", i, bmt->BMT_EU_backup[i]->start_pos);
    }

    for (i = BMT_num_EUs_orig; i < BMT_num_EUs_new; i++) {
        h = gc[get_next_disk()];
        retEU = FreeList_delete(h, EU_BMT, false);
        bmt->BMT_EU[i] = retEU;
        LOGINFO("%dth main: %llu, disk num %d\n", i, bmt->BMT_EU[i]->start_pos,
                h->disk_num);
    }

    for (i = BMT_num_EUs_orig; i < BMT_num_EUs_new; i++) {
        h = gc[(bmt->BMT_EU[i]->disk_num + (td->cn_ssds / 2)) % td->cn_ssds];   //Tifa: just a offset(half disk num) to disperse the BMT_EU
        retEU = FreeList_delete(h, EU_BMT, false);
        bmt->BMT_EU_backup[i] = retEU;
        LOGINFO("%dth bkup: %llu, disk num %d\n\n",
                i, bmt->BMT_EU_backup[i]->start_pos, h->disk_num);
    }

    bmt->BMT_EU_gc = bmt_orig->BMT_EU_gc;
    bmt->BMT_EU_backup_gc = bmt_orig->BMT_EU_backup_gc;
    LOGINFO("cn_ppq_new=%d, cn_ppq_orig=%d\n", bmt->cn_ppq, bmt_orig->cn_ppq);

    for (i = 0; i < bmt_orig->cn_ppq; i++) {
        if (bmt_orig->per_page_queue[i].pDbmtaUnit != NULL) {
            bmt->per_page_queue[i].pDbmtaUnit = dbmtaE_BK_alloc(&td->dbmtapool);
            memcpy(bmt->per_page_queue[i].pDbmtaUnit,
                   bmt_orig->per_page_queue[i].pDbmtaUnit,
                   sizeof(dbmtE_bucket_t));

            dbmtaE_BK_free(&td->dbmtapool,
                           bmt_orig->per_page_queue[i].pDbmtaUnit);
            bmt_orig->per_page_queue[i].pDbmtaUnit = NULL;
            mutex_destroy(&(bmt_orig->per_page_queue[i].ppq_lock));
        }

        if (bmt_orig->per_page_queue[i].addr_ondisk != PAGEBMT_NON_ONDISK) {
            //bmt->per_page_queue[i].pDbmtaUnit = bmt_orig->per_page_queue[i].pDbmtaUnit
            bmt->per_page_queue[i].addr_ondisk =
                bmt_orig->per_page_queue[i].addr_ondisk;
            bmt->per_page_queue[i].ondisk_logi_page =
                bmt_orig->per_page_queue[i].ondisk_logi_page;
            bmt->per_page_queue[i].ondisk_logi_eu =
                bmt_orig->per_page_queue[i].ondisk_logi_eu;
        }
        bmt->per_page_queue[i].fg_pending =
            bmt_orig->per_page_queue[i].fg_pending;
    }

    LOGINFO("ERASE map at :\n");
    for (i = cn_ssd_orig; i < td->cn_ssds; i++) {   /* iterate over all disks */
        h = gc[i];
        h->ECT.eu_curr = FreeList_delete(h, EU_ECT, false);
        h->ECT.eu_next = FreeList_delete(h, EU_ECT, false);
        atomic_set(&h->ECT.fg_next_dirty, 0);
        LOGINFO("Disk %d Erase EU at %llu\n", i, h->ECT.eu_curr->start_pos);
    }

    return 0;
}

static int32_t BMT_fresh_setup(lfsm_dev_t * td, MD_L2P_table_t * bmt,
                               int32_t BMT_num_EUs)
{
    struct HListGC *h;
    struct EUProperty *retEU;
    int32_t i;

    /* round robin get EU from free list of HGCList free list */
    LOGINFO("BMT and backup map at :\n");
    for (i = 0; i < BMT_num_EUs; i++) {
        h = gc[get_next_disk()];
        retEU = FreeList_delete(h, EU_BMT, false);
        bmt->BMT_EU[i] = retEU;
        LOGINFO("%dth main: %llu\n", i, bmt->BMT_EU[i]->start_pos);
    }

    for (i = 0; i < BMT_num_EUs; i++) {
        h = gc[(bmt->BMT_EU[i]->disk_num + (td->cn_ssds / 2)) % td->cn_ssds];   //Tifa: just a offset(half disk num) to disperse the BMT_EU
        retEU = FreeList_delete(h, EU_BMT, false);
        bmt->BMT_EU_backup[i] = retEU;
        LOGINFO("%dth bkup: %llu\n", i, bmt->BMT_EU_backup[i]->start_pos);
    }

    h = gc[bmt->BMT_EU[0]->disk_num];
    bmt->BMT_EU_gc = FreeList_delete(h, EU_BMT, false);
    h = gc[bmt->BMT_EU_backup[0]->disk_num];
    bmt->BMT_EU_backup_gc = FreeList_delete(h, EU_BMT, false);

    LOGINFO("GC BMT at %llu %llu :\n", bmt->BMT_EU_gc->start_pos,
            bmt->BMT_EU_backup_gc->start_pos);
    HPEU_alloc_log_EU(&td->hpeutab, get_next_disk(), get_next_disk());
    LOGINFO("ERASE map at :\n");
    for (i = 0; i < td->cn_ssds; i++) { /* iterate over all disks */
        h = gc[i];
        h->ECT.eu_curr = FreeList_delete(h, EU_ECT, false);
        h->ECT.eu_next = FreeList_delete(h, EU_ECT, false);
        atomic_set(&h->ECT.fg_next_dirty, 0);
        LOGINFO("Disk %d Erase EU at %llu\n", i, h->ECT.eu_curr->start_pos);
    }
    return 0;
}

/*
 * Description: generate BMT table
 *
 * return: true:  generate fail
 *         false: generate ok
 */
bool generate_BMT(lfsm_dev_t * td, int32_t BMT_num_EUs, diskgrp_t * pgrp)
{
    sddmap_disk_hder_t *prec_ddmap;
    MD_L2P_table_t *bmt;
    sector64 pos;
    int32_t i, cn_hpage;

    bmt = td->ltop_bmt;

    if (dedicated_map_init(td)) {
        LOGERR("fail gen BMT dedicated_map_init fail\n");
        return true;
    }

    /** Size in pages **/
    td->UpdateLog.size = FPAGE_PER_SEU << SECTORS_PER_FLASH_PAGE_ORDER;
    if (UpdateLog_init(&td->UpdateLog)) {
        LOGERR("fail gen BMT update log init fail\n");
        return true;
    }

    if (metalog_init(&td->mlogger, gc, td) < 0) {
        return true;
    }

    if (BMT_num_EUs > MAX_BMT_EUS) {
        LOGERR("MAX_BMT_EUS %d < BMT_num_EUs %d EUs\n", MAX_BMT_EUS,
               BMT_num_EUs);
        return true;
    }

    LOGINFO("Need BMT_num_EUs %d EUs\n", BMT_num_EUs);

    if (td->prev_state == sys_fresh) {
        BMT_fresh_setup(td, bmt, BMT_num_EUs);
        return false;
    }
    // ------------------- NOT A FRESH DISK -----------------------
    cn_hpage =
        LFSM_CEIL_DIV(ddmap_get_ondisk_size(td->cn_ssds), HARD_PAGE_SIZE);
    if (IS_ERR_OR_NULL(prec_ddmap = (sddmap_disk_hder_t *)
                       track_kmalloc(cn_hpage * HARD_PAGE_SIZE, GFP_KERNEL,
                                     M_OTHER))) {
        LOGERR("Memory allocation failed for store_rec\n");
        return true;
    }

    LOGINFO("Alloc mem %p for loading ddmap\n", prec_ddmap);
    if (super_map_set_dedicated_map(td, pgrp->ar_subdev) < 0) {
        LOGERR("super_map_set_dedicated_map\n");
        goto l_fail;
    }

    if (dedicated_map_get_last_entry(&td->DeMap, prec_ddmap, td)) {
        LOGERR("dedicated_map_get_last_entry fail\n");
        goto l_fail;
    }

    if (prec_ddmap->signature != sys_loaded
        && prec_ddmap->signature != sys_unloaded) {
        LOGERR("Unexpected signature=%x %p\n", prec_ddmap->signature,
               prec_ddmap);
        goto l_fail;
    }

    td->prev_state = prec_ddmap->signature;
    dedicated_map_set_dev_state(prec_ddmap, pgrp);

    if (IS_ERR_OR_NULL(FreeList_delete_by_pos(td, td->DeMap.eu_main->start_pos, EU_DEDICATEMAP))) { // check again for dedicate map's EU stat
        goto l_fail;
    }

    if (IS_ERR_OR_NULL(FreeList_delete_by_pos(td, td->DeMap.eu_backup->start_pos, EU_DEDICATEMAP))) {   // check again for dedicate map's EU stat
        goto l_fail;
    }

    LOGINFO("After loading latest ddmap record.....\n");
    ddmap_header_dump(prec_ddmap);

    if (UpdateLog_eu_set(&td->UpdateLog, td, prec_ddmap) < 0) {
        goto l_fail;
    }

    LOGINFO("Got UpdateLog main/backup pos (%llus / %llus) from ddmap\n",
            td->UpdateLog.eu_main->start_pos,
            td->UpdateLog.eu_backup->start_pos);
//    printk("eu_log_start =%llu\n",eu_log_start);

    if (ExtEcc_eu_set(td, prec_ddmap) < 0)
        goto l_fail;
    for (i = 0; i < td->cn_ssds; i++) {
        LOGINFO("Got ExtEcc main[%d] pos (%llus) log_frontier(%d) from ddmap\n",
                i, td->ExtEcc[i].eu_main->start_pos,
                td->ExtEcc[i].eu_main->log_frontier);
    }

    if (lfsm_load_ECT(td, prec_ddmap) < 0) {
        goto l_fail;
    }

    for (i = 0; i < BMT_num_EUs; i++) {
        pos = ddmap_get_BMT_map(prec_ddmap, i);
//        LOGINFO("Set the pos of main BMT[%d] : %llds (EU %lld)\n",
//            i,pos,pos>>SECTORS_PER_SEU_ORDER);
        if (IS_ERR_OR_NULL
            (bmt->BMT_EU[i] = FreeList_delete_by_pos(td, pos, EU_BMT))) {
            LOGERR("err: bmt->BMT_EU[%d]\n", i);
            goto l_fail;
        }
    }

    // Recover the backup BMT EUs
    for (i = 0; i < BMT_num_EUs; i++) {
        pos = ddmap_get_BMT_redundant_map(prec_ddmap, i, 1);
//        LOGINFO("Set the pos of backup BMT[%d] : %llds (EU %lld)\n",
//            i,pos,pos>>SECTORS_PER_SEU_ORDER);
        if (IS_ERR_OR_NULL
            (bmt->BMT_EU_backup[i] = FreeList_delete_by_pos(td, pos, EU_BMT))) {
            LOGERR("bmt->BMT_EU_backup[%d]<0\n", i);
            goto l_fail;
        }
    }

    // Recover the BMT EU gc
    LOGINFO("Alloc BMT EU for gc usage...\n");
    if (IS_ERR_OR_NULL
        (bmt->BMT_EU_gc =
         FreeList_delete_by_pos(td, prec_ddmap->BMT_map_gc, EU_BMT))) {
        LOGERR("err: bmt->BMT_EU_gc<0 fail\n");
        goto l_fail;
    }

    if (IS_ERR_OR_NULL(bmt->BMT_EU_backup_gc =
                       FreeList_delete_by_pos(td,
                                              prec_ddmap->BMT_redundant_map_gc,
                                              EU_BMT))) {
        LOGERR("err: bmt->BMT_EU_redundant_gc<0 fail\n");
        goto l_fail;
    }

    if (!PPDM_PAGEBMT_rebuild(td, bmt)) {
        LOGERR("PPDM_PAGEBMT_rebuild failure\n");
        goto l_fail;
    }
    // load HPEUTab and remove both EU from FreeList
    if (!HPEU_load
        (&td->hpeutab, prec_ddmap->HPEU_map, prec_ddmap->HPEU_redundant_map,
         prec_ddmap->HPEU_idx_next)) {
        goto l_fail;
    }

    if (metalog_eulog_set_all(td->mlogger.par_eulog,
                              ddmap_get_eu_mlogger(prec_ddmap), td->cn_ssds,
                              td) < 0) {
        goto l_fail;
    }

    td->logical_capacity = prec_ddmap->logical_capacity;
    track_kfree(prec_ddmap, cn_hpage * HARD_PAGE_SIZE, M_OTHER);
    return false;

  l_fail:
    track_kfree(prec_ddmap, cn_hpage * HARD_PAGE_SIZE, M_OTHER);
    return true;
}

static bool _alloc_BMT_EU(MD_L2P_table_t * bmt)
{
    bool ret;

    ret = true;

    do {
        bmt->BMT_EU = (struct EUProperty **)
            track_kmalloc(bmt->BMT_num_EUs * sizeof(struct EUProperty *),
                          GFP_KERNEL, M_BMT);
        if (IS_ERR_OR_NULL(bmt->BMT_EU)) {
            LOGINERR("fail alloc mem to EU of BMT %p\n", bmt->BMT_EU);
            break;
        }

        /* Redundant copy of BMT EUs */
        bmt->BMT_EU_backup = (struct EUProperty **)
            track_kmalloc(bmt->BMT_num_EUs * sizeof(struct EUProperty *),
                          GFP_KERNEL, M_BMT);
        if (IS_ERR_OR_NULL(bmt->BMT_EU_backup)) {
            LOGINERR("fail alloc mem to backup EU of BMT\n");
            break;
        }

        ret = false;
    } while (0);

    return ret;
}

/*
 * Description: To allocate, create and initialize all the data structures
 *     related to the BMT
 *     Called during the initialization of the driver
 * @bmt : the reference pointer of the created sorted array object
 *
 * return true : create fail
 *        false: create ok
 */
bool create_BMT(MD_L2P_table_t ** bmt, sector64 capacity_in_fpage)  // in flash page or said required records
{
    uint64_t record_num_per_flash_page;
    int32_t retry, cn_fpage_valid_bitmap, cn_fpage_bmt, cn_fpage_needed, i;

    if (IS_ERR_OR_NULL(bmt)) {
        LOGERR("bmt is NULL\n");
        return true;
    }

    *bmt =
        (MD_L2P_table_t *) track_kmalloc(sizeof(MD_L2P_table_t),
                                         GFP_KERNEL | __GFP_ZERO, M_BMT);
    if (IS_ERR_OR_NULL(*bmt)) {
        LOGERR("fail alloc memory for bmt\n");
        return true;
    }

    (*bmt)->fg_gceu_dirty = false;
    mutex_init(&((*bmt)->cmt_lock));

    // bmt->external_size = how many pages are required to store all records (the required number of pages is aligned to SEU)
    record_num_per_flash_page = FLASH_PAGE_SIZE / sizeof(struct D_BMT_E);
    (*bmt)->cn_ppq =
        LFSM_CEIL_DIV(capacity_in_fpage, record_num_per_flash_page);

    cn_fpage_bmt = (*bmt)->cn_ppq * FACTOR_PAGEBMT2PPQ;
    LOGINFO("BMT size : %d fpages for storage space %llu fpages\n",
            (*bmt)->cn_ppq, capacity_in_fpage);

    /* each ppq entry has 1 bit, # of fpage need = cn_ppq/8/4096 */
    cn_fpage_valid_bitmap = bmt_cn_fpage_valid_bitmap(*bmt);    // ==0 is TF_FW_FREE ==0
    cn_fpage_needed = cn_fpage_bmt + cn_fpage_valid_bitmap;

    (*bmt)->BMT_num_EUs = LFSM_CEIL_DIV(cn_fpage_needed, FPAGE_PER_SEU);

    if (_alloc_BMT_EU(*bmt)) {
        LOGINFO("Fail alloc EU to bmt num of EU %d\n", (*bmt)->BMT_num_EUs);
        return true;
    }

    LOGINFO("create BMT: external size %llu cn_ppq %d "
            "fpage bmt %d fpage bitmap %d EUs for bmt %d\n",
            capacity_in_fpage, (*bmt)->cn_ppq,
            cn_fpage_bmt, cn_fpage_valid_bitmap, (*bmt)->BMT_num_EUs);

    LOGINFO("Allocing AR_PAGEBMT_isValidSegment size %d byte\n",
            bmt_sz_ar_ValidSegment(*bmt));
    (*bmt)->AR_PAGEBMT_isValidSegment =
        track_kmalloc(bmt_sz_ar_ValidSegment(*bmt), GFP_KERNEL | __GFP_ZERO,
                      M_BMT);
    if ((*bmt)->AR_PAGEBMT_isValidSegment == 0) {
        LOGERR("Error in alloc AR_PAGEBMT_isValidSegment\n");
        return true;
    }

    LOGINFO("Allocating per page queues.........  %d, size=%lu\n",
            (*bmt)->cn_ppq, sizeof(MD_L2P_table_t) * (*bmt)->cn_ppq);
    retry = 0;
    while (IS_ERR_OR_NULL((*bmt)->per_page_queue = (MD_L2P_item_t *)
                          track_vmalloc(sizeof(MD_L2P_item_t) * (*bmt)->cn_ppq,
                                        GFP_KERNEL | __GFP_ZERO, M_BMT))) {
        LOGERR("Allocating bmt ppq failure(retry=%d) size %ld \n",
               retry, sizeof(MD_L2P_item_t) * (*bmt)->cn_ppq);
        schedule();
        if ((retry++) > LFSM_MAX_RETRY) {
            return true;
        }
    }

    for (i = 0; i < (*bmt)->cn_ppq; i++) {
        memset((*bmt)->per_page_queue[i].conflict.bvec, 0, SZ_BVEC);
        INIT_LIST_HEAD(&(*bmt)->per_page_queue[i].conflict.confl_list);
        spin_lock_init(&(*bmt)->per_page_queue[i].conflict.confl_lock);
    }

    LOGINFO("DONE\n");
    if (!PPDM_BMT_cache_init(*bmt)) {
        return true;
    }

    LOGINFO("DONE\n");
    atomic64_set(&((*bmt)->cn_lookup_query), 0);
    atomic_set(&((*bmt)->cn_lookup_missing), 0);

    return false;
}

void BMT_destroy(MD_L2P_table_t * bmt)
{
    if (IS_ERR_OR_NULL(bmt)) {
        return;
    }
#if DYNAMIC_PPQ_LOCK_KEY==1
    if (bmt->ar_key) {
        track_kfree(bmt->ar_key, bmt->cn_ppq * sizeof(struct lock_class_key),
                    M_UNKNOWN);
    }
#endif

    if (bmt->BMT_EU) {
        track_kfree(bmt->BMT_EU,
                    lfsmdev.ltop_bmt->BMT_num_EUs * sizeof(struct EUProperty *),
                    M_BMT);
    }

    if (bmt->BMT_EU_backup) {
        track_kfree(bmt->BMT_EU_backup,
                    lfsmdev.ltop_bmt->BMT_num_EUs * sizeof(struct EUProperty *),
                    M_BMT);
    }

    if (bmt->per_page_queue) {
        track_vfree(bmt->per_page_queue, sizeof(MD_L2P_item_t) * (bmt->cn_ppq),
                    M_BMT);
    }

    if (bmt->AR_PAGEBMT_isValidSegment) {
        track_kfree(bmt->AR_PAGEBMT_isValidSegment, bmt_sz_ar_ValidSegment(bmt),
                    M_BMT);
    }

    if (bmt) {
        track_kfree(bmt, sizeof(MD_L2P_table_t), M_BMT);
    }
}
