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
#include "hpeu.h"
#include "hpeu_recov.h"
#include "devio.h"
#include "io_manager/io_common.h"
#include "spare_bmt_ul.h"
#include "bmt_ppdm.h"
#include "bmt.h"

static bool HPEUrecov_isvalid_id_hpeu(int32_t ul_idx_page,
                                      int32_t erase_idx_page)
{
    return (ul_idx_page > erase_idx_page);
}

static bool ulgcrec_update_firstmark(HPEUTab_t * hpeutab,
                                     BMT_update_record_t * ul_entry)
{
    sector64 pos_eu, birthday;
    struct EUProperty *eu;
    seuinfo_t *peuinfo;
    int32_t temp, id_pgroup, idx_eu, id_hpeu;
    bool ret;

    ret = true;
    for (temp = 0; temp < TEMP_LAYER_NUM; temp++) {
        for (id_pgroup = 0; id_pgroup < hpeutab->td->cn_pgroup; id_pgroup++) {  //ding: can use td in hpeutab?
            for (idx_eu = 0; idx_eu < hpeutab->cn_disk; idx_eu++) {
                peuinfo = gculrec_get_euinfo(&ul_entry->gc_record,
                                             HPEU_EU2HPEU_OFF(idx_eu,
                                                              id_pgroup), temp);
                id_hpeu = peuinfo->id_hpeu;
                pos_eu = peuinfo->eu_start;
                birthday = peuinfo->birthday;

                if ((ret =
                     HPEU_force_update(hpeutab, id_hpeu, pos_eu,
                                       birthday)) == false) {
                    return false;
                }

                if (!IS_ERR_OR_NULL(eu = eu_find(pos_eu))) {
                    eu->id_hpeu = id_hpeu;
                } else {
                    return false;
                }
            }
        }
    }

    return ret;
}

static bool HPEUrecov_rebuild_hpeutab(int8_t * pBuf, HPEUTab_t * hpeutab,
                                      int32_t len, int32_t idx)
{
    struct EUProperty *eu;
    BMT_update_record_t *ul_entry;
    sector64 pos_eu;
    int32_t i, id_hpeu;
    bool ret;

    ret = true;

    for (i = 0; i < len; i++) {
        ul_entry = (BMT_update_record_t *) (pBuf + (i * HARD_PAGE_SIZE));
        if (ul_entry->signature == UL_NORMAL_MARK) {
            id_hpeu = ul_entry->n_record.id_hpeu;
            pos_eu = ul_entry->n_record.eu_start;
            if (!
                (ret =
                 HPEU_force_update(hpeutab, id_hpeu, pos_eu,
                                   ul_entry->n_record.birthday))) {
                break;
            }
            if (!IS_ERR_OR_NULL(eu = eu_find(ul_entry->n_record.eu_start))) {
                eu->id_hpeu = id_hpeu;
            } else {
                return false;
            }
        } else if (ul_entry->signature == UL_HPEU_ERASE_MARK) {
            id_hpeu = ul_entry->id_hpeu;
            if (!(ret = HPEU_free(hpeutab, id_hpeu))) {
                break;
            }
        } else if ((idx + i) == 0 && (ul_entry->signature == UL_GC_MARK)) {
            ulgcrec_update_firstmark(hpeutab, ul_entry);
        }
    }

    return ret;
}

// true mean finish this round; false mean end of EU
static bool HPEUrecov_get_ermark(int8_t * pBuf, int32_t * ar_hpeu_ermark,
                                 int32_t ul_idx_pg, int32_t len)
{
    BMT_update_record_t *entry;
    int32_t i, idx_eu, idx_hpeu;

    for (i = 0; i < len; i++) {
        if ((ul_idx_pg + i) >= HPAGE_PER_SEU) {
            return false;
        }

        entry = (BMT_update_record_t *) (pBuf + (i * HARD_PAGE_SIZE));
        if (entry->signature == UL_EMPTY_MARK) {
            rec_printk("REC: id_hpeu %d signature is EMPTY, return false\n",
                       entry->id_hpeu);
            return false;
        }

        if (entry->signature == UL_ERASE_MARK) {
            idx_eu = (entry->n_record.eu_start >> SECTORS_PER_SEU_ORDER);
            pbno_eu_map[idx_eu]->erase_count++;
        } else if (entry->signature == UL_HPEU_ERASE_MARK) {
            idx_hpeu = entry->id_hpeu;
            rec_printk("REC: erase id_hpeu %d id_pg %d\n", entry->id_hpeu,
                       ul_idx_pg + i);
            if (idx_hpeu == -1) {
                continue;
            }

            if (ar_hpeu_ermark[idx_hpeu] < ul_idx_pg + i) {
                ar_hpeu_ermark[idx_hpeu] = ul_idx_pg + i;   //log ul page id
            }
        } else if (entry->signature == UL_NORMAL_MARK) {
            idx_eu = (entry->n_record.eu_start >> SECTORS_PER_SEU_ORDER);
            rec_printk
                ("REC: used id_hpeu %d temper %d pos %llu id_eu %d id_pg %d\n",
                 entry->n_record.id_hpeu, entry->n_record.temperature,
                 entry->n_record.eu_start, idx_eu, ul_idx_pg + i);
        }
    }

    return true;
}

static bool HPEUrecov_ul_prescan(lfsm_dev_t * td, int32_t * ar_ret)
{
    sector64 st_pos, pos_main, pos_backup;
    int8_t *pBuf;
    int32_t idx_st_hpage, i;
    bool ret;

    ret = false;

    pos_main = DEV_SYS_READY(grp_ssds, td->UpdateLog.eu_main->disk_num) ?
        td->UpdateLog.eu_main->start_pos : -1;
    pos_backup = DEV_SYS_READY(grp_ssds, td->UpdateLog.eu_backup->disk_num) ?
        td->UpdateLog.eu_backup->start_pos : -1;

    if (IS_ERR_OR_NULL(pBuf = (int8_t *)
                       track_kmalloc(HARD_PAGE_SIZE * UL_OPT_RD_HPAGE,
                                     __GFP_ZERO, M_UNKNOWN))) {
        LOGERR(" alloc page fail in page\n");
        goto l_end;
    }

    idx_st_hpage = 0;

    for (i = idx_st_hpage; i < HPAGE_PER_SEU; i += UL_OPT_RD_HPAGE) {
        st_pos = pos_main;
        if ((st_pos == -1) ||
            devio_read_hpages(st_pos + (i << SECTORS_PER_HARD_PAGE_ORDER), pBuf,
                              UL_OPT_RD_HPAGE) == false) {
            st_pos = pos_backup;
            if ((st_pos == -1)
                || devio_read_hpages(st_pos +
                                     (i << SECTORS_PER_HARD_PAGE_ORDER), pBuf,
                                     UL_OPT_RD_HPAGE) == false) {
                LOGERR(" fail to read updatelog can't recovery\n");
                goto l_end;
            }
        }

        if (!HPEUrecov_rebuild_hpeutab(pBuf, &td->hpeutab, UL_OPT_RD_HPAGE, i)) {
            LOGERR(" fail to rebuild hpeutab can't recovery\n");
            goto l_end;
        }

        if (!HPEUrecov_get_ermark(pBuf, ar_ret, i, UL_OPT_RD_HPAGE)) {
            break;
        }
    }
    ret = true;

  l_end:
    if (pBuf) {
        track_kfree(pBuf, HARD_PAGE_SIZE * UL_OPT_RD_HPAGE, M_UNKNOWN);
    }
    return ret;
}

static bool HPEUrecov_entry_alloc_insert(int32_t id_hpeu,
                                         struct list_head *hpeu_list)
{
    HPEUrecov_entry_t *rec_entry;

    if (IS_ERR_OR_NULL(rec_entry = (HPEUrecov_entry_t *)
                       track_kmalloc(sizeof(HPEUrecov_entry_t), GFP_KERNEL,
                                     M_RECOVER))) {
        LOGERR(" can't alloc recov_entry\n");
        return false;
    }
    rec_entry->id_hpeu = id_hpeu;
    list_add_tail(&rec_entry->link, hpeu_list);
    return true;
}

static int32_t ulgcrec_get_eu_id_hpeu(GC_UL_record_t * prec, int32_t temp,
                                      int32_t id_pgroup, int32_t idx_eu)
{
    int32_t id_disk;
    id_disk = HPEU_EU2HPEU_OFF(idx_eu, id_pgroup);
    return gculrec_get_euinfo(prec, id_disk, temp)->id_hpeu;
}

static int32_t ulgcrec_get_eu_frontier(GC_UL_record_t * prec, int32_t temp,
                                       int32_t id_pgroup, int32_t idx_eu)
{
    int32_t id_disk;

    id_disk = HPEU_EU2HPEU_OFF(idx_eu, id_pgroup);
    return gculrec_get_euinfo(prec, id_disk, temp)->frontier;
}

static bool ulgcrec_is_complete_hpeu(GC_UL_record_t * prec, int32_t temp,
                                     int32_t id_pgroup,
                                     int32_t cn_ssds_per_hpeu)
{
    int32_t idx_eu, id_hpeu;

    id_hpeu = ulgcrec_get_eu_id_hpeu(prec, temp, id_pgroup, 0);
    for (idx_eu = 1; idx_eu < cn_ssds_per_hpeu; idx_eu++) {
        if (id_hpeu != ulgcrec_get_eu_id_hpeu(prec, temp, id_pgroup, idx_eu)) {
            return false;
        }
    }

    return true;
}

// Note
// (1) must be a complete hpeu
// (2) f_max must be the multiple of SECTORS_PER_HARD_PAGE
static int32_t ulgcrec_get_hpeu_frontier(GC_UL_record_t * prec, int32_t temp,
                                         int32_t id_pgroup,
                                         int32_t cn_ssds_per_hpeu)
{
    int32_t idx_eu, f_max, f;

    f_max = ulgcrec_get_eu_frontier(prec, temp, id_pgroup, 0);

    if (f_max == 0) {
        return 0;
    }
    // weafon: try to find the edge of hpeu
    // where X represents the frontier of hpeu
    //            ____________
    // ___________|X

    for (idx_eu = 1; idx_eu < cn_ssds_per_hpeu; idx_eu++) {
        f = ulgcrec_get_eu_frontier(prec, temp, id_pgroup, idx_eu);
        if (f < f_max) {
            break;
        }
    }

    return HPEU_EU2HPEU_FRONTIER(idx_eu - 1, f_max);
}

static int32_t HPEUrecov_entry_get_id(HPEUrecov_param_t * param, bool isTail)
{
    HPEUrecov_entry_t *entry;
    if (isTail) {
        entry = (HPEUrecov_entry_t *) param->lst_id_hpeu.prev;
    } else {
        entry = (HPEUrecov_entry_t *) param->lst_id_hpeu.next;
    }

    return entry->id_hpeu;
}

static bool HPEUrecov_entry_isequal_id(HPEUrecov_param_t * param,
                                       int32_t id_hpeu, bool isTail)
{
    return (id_hpeu == HPEUrecov_entry_get_id(param, isTail));
}

static int32_t HPEUrecov_setup_entry_GC(lfsm_dev_t * td,
                                        BMT_update_record_t * ul_entry,
                                        int32_t * ar_erase_idx,
                                        int32_t ul_idx_page,
                                        HPEUrecov_param_t **
                                        ar_hpeu_recov_param)
{
    HPEUrecov_param_t *param;
    int32_t temp, id_pgroup, hpeu_end_page, id_hpeu;
    bool fg_complete;

    hpeu_end_page = -1;
    id_hpeu = -1;
    fg_complete = true;

    for (temp = 0; temp < TEMP_LAYER_NUM; temp++) {
        for (id_pgroup = 0; id_pgroup < td->cn_pgroup; id_pgroup++) {
            param = &ar_hpeu_recov_param[id_pgroup][temp];

            //// select idx=0 as default id_hpeu
            id_hpeu =
                ulgcrec_get_eu_id_hpeu(&ul_entry->gc_record, temp, id_pgroup,
                                       0);

            if (!HPEUrecov_isvalid_id_hpeu(ul_idx_page, ar_erase_idx[id_hpeu])) {   //this hpeu have been erase and invalid, set idx_end
                param->idx_end = FPAGE_PER_SEU * td->hpeutab.cn_disk;   // the tail of the last eu in the list
                //printk("id_hpeu is invalid %d\n",id_hpeu);
                continue;
            }
            //// calculate end_page when id_hepu complete
            // an imcomplete hpeu means part of its eu have not been allocated.
            // Since we alloc all eu for a new hpeu before we write anything to the hpeu, a incomplete hpeu must not host any data page
            if ((fg_complete =
                 ulgcrec_is_complete_hpeu(&ul_entry->gc_record, temp,
                                          id_pgroup, td->hpeutab.cn_disk))) {
                hpeu_end_page = ulgcrec_get_hpeu_frontier(&ul_entry->gc_record,
                                                          temp, id_pgroup,
                                                          td->hpeutab.
                                                          cn_disk) >>
                    SECTORS_PER_FLASH_PAGE_ORDER;
                param->idx_end = hpeu_end_page;
            } else {            //this id_hpeu is incomplete ,and set the insert id_hpeu idx_end 0
                param->idx_end = 0;
            }

            if (list_empty(&param->lst_id_hpeu)) {  // support this id_hpeu is first shown, insert the id_hpeu to recov_list
                if (!HPEUrecov_entry_alloc_insert(id_hpeu, &param->lst_id_hpeu)) {
                    return -ENOMEM;
                }

                param->idx_start = 0;
                if (ul_idx_page == 0) {
                    LOGINFO("gc is first log {%d, %d}\n", param->idx_start,
                            param->idx_end);
                    param->idx_start = hpeu_end_page;
                }
            } else if (!HPEUrecov_entry_isequal_id(param, id_hpeu, true)) { //chk last lst_id_hpeu is equal
                if (!fg_complete) {
                    param->idx_end = FPAGE_PER_SEU * td->hpeutab.cn_disk;
                    rec_printk
                        ("REC: LOG(%x) prev-id_pheu temp %d end_page %d\n",
                         ul_entry->signature, temp, param->idx_end);
                } else {        // fg_complete == true
                    LOGERR(" the last id_hpeu %d isn't equal this id_hepu %d\n",
                           HPEUrecov_entry_get_id(param, true), id_hpeu);
                    return -EINVAL;
                }
            }

            rec_printk
                ("REC: LOG(%x) id_pheu %d temp %d st_page %d end_page %d\n",
                 ul_entry->signature, id_hpeu, temp, param->idx_start,
                 param->idx_end);
        }                       // for id_pgroup
    }                           //for temp

    return 0;
}

static int32_t HPEUrecov_setup_entry_EMPTY(lfsm_dev_t * td,
                                           HPEUrecov_param_t **
                                           ar_hpeu_recov_param)
{
    HPEUrecov_param_t *param;
    int32_t temp, id_group;

    for (id_group = 0; id_group < td->cn_pgroup; id_group++) {
        for (temp = 0; temp < TEMP_LAYER_NUM; temp++) {
            param = &ar_hpeu_recov_param[id_group][temp];

            if (list_empty(&param->lst_id_hpeu)) {
                continue;
            }
            param->idx_end = FPAGE_PER_SEU * td->hpeutab.cn_disk;
        }
    }

    return 0;
}

static bool HPEUrecov_rebuild_bmt(int32_t id_hpeu, struct EUProperty **ar_seu,
                                  sector64 ** ar_lpn, lfsm_dev_t * td,
                                  int32_t idx_start, int32_t idx_end,
                                  int32_t temperature)
{
    sector64 lpn, pbn;
    struct D_BMT_E dbmta;
    int32_t id_disk, id_pg, st_pg, st_drive, end_pg, end_drive, look_ahead;

    st_pg = idx_start / td->hpeutab.cn_disk;
    st_drive = idx_start % td->hpeutab.cn_disk; // st drive
    end_pg = idx_end / td->hpeutab.cn_disk;
    end_drive = idx_end % td->hpeutab.cn_disk;  // next end drive
    look_ahead = 1;
    //printk("%s: id_hpeu %d {%d,%d} start: pg %d drive %d, end: pg %d drive %d\n",
    //        __FUNCTION__,id_hpeu,idx_start,idx_end, st_pg, st_drive, end_pg, end_drive);
    for (id_pg = st_pg; id_pg < (FPAGE_PER_SEU - METADATA_SIZE_IN_FLASH_PAGES);
         id_pg++) {
        for (id_disk = 0; id_disk < td->hpeutab.cn_disk; id_disk++) {
            if (IS_ERR_OR_NULL(ar_seu[id_disk])) {
                LOGERR("id_hpeu %d ar_eu[%d] is NULL\n", id_hpeu, id_disk);
                return false;
            }

            ar_seu[id_disk]->temper = temperature;

            if ((id_pg == st_pg) && (id_disk < st_drive)) {
                //printk("id_hpeu %d id_disk %d st_pg %d id %d\n",id_hpeu, id_disk, st_pg, id_pg);
                continue;
            }

            if ((id_pg >= end_pg) && (id_disk >= end_drive)) {
                //printk("id_hpeu %d id_disk %d end_pg %d id %d\n",id_hpeu, id_disk, end_pg, id_pg);
                goto l_end;
            }

            lpn = ar_lpn[id_disk][id_pg];
            pbn =
                ar_seu[id_disk]->start_pos +
                (id_pg << SECTORS_PER_FLASH_PAGE_ORDER);
            //printk("%s: hpeu %d lpn %llx pbn %llu id_disk %d\n",__FUNCTION__, id_hpeu,lpn, pbn, id_disk);
            if (SPAREAREA_IS_EMPTY(lpn)) {
                if (look_ahead == 0) {
                    if (id_pg < (FPAGE_PER_SEU - METADATA_SIZE_IN_FLASH_PAGES)) {
                        LOGINFO
                            ("meta information is only cn_page %d, lpn= %llx, please check\n",
                             id_pg, lpn);
                    }
                    goto l_end;
                }
                look_ahead--;
                continue;
            }

            if (RDDCY_IS_ECCLPN(lpn)) {
                continue;
            }

            if (lpn == LBNO_PREPADDING_PAGE) {
                continue;
            }

            if (bmt_lookup(td, td->ltop_bmt, lpn, true, 1, &dbmta) < 0) {
                LOGERR("hpeu %d lpn 0x%llx pbn 0x%llu\n", id_hpeu, lpn, pbn);
                return false;
            }

            if ((dbmta.pbno == PBNO_SPECIAL_NEVERWRITE) ||
                (temperature != TEMP_LAYER_NUM - 1) ||
                (!eu_same_temperature(dbmta.pbno, pbn)) ||
                (dbmta.pending_state != DBMTE_PENDING) ||
                (HPEU_is_younger(&td->hpeutab, dbmta.pbno, pbn, lpn) == 1)) {
//                LOGINFO("set lbno %llu -> %llu [birthday %llx ft %lld pg %d] old_pbn (birthday: %llx) %llu pend %d id_hpeu %d\n",lpn, pbn,
//                    HPEU_get_birthday(&td->hpeutab, eu_find(pbn)), HPEU_get_frontier(pbn),PGROUP_ID(eu_find(pbn)->disk_num),
//                    HPEU_get_birthday(&td->hpeutab, eu_find(dbmta.pbno)), (sector64)dbmta.pbno, dbmta.pending_state,eu_find(dbmta.pbno)->id_hpeu);
                PPDM_BMT_cache_insert_one_pending(td, lpn, pbn, 1, true,
                                                  DIRTYBIT_SET, false);
            }
        }
    }

  l_end:
    return true;
}

// rebuild bmt according to the metadata of a specific hpeu (id_hpeu)
// where metadata is got from metadata of eu, or spare area or xor from other eu
static int32_t HPEUrecov_rebuild_hpeu(lfsm_dev_t * td, int32_t id_hpeu,
                                      int32_t idx_start, int32_t idx_end,
                                      int32_t temperature)
{
    sector64 **ar_lpn;
    struct EUProperty **ar_seu;
    int32_t i, ret, retry;

    ret = -1;
    retry = 0;
    if (IS_ERR_OR_NULL(ar_lpn = (sector64 **)
                       track_kmalloc(td->hpeutab.cn_disk * sizeof(sector64 *),
                                     GFP_KERNEL | __GFP_ZERO, M_RECOVER))) {
        ret = -ENOMEM;
        goto l_free_lpn;
    }

    if (IS_ERR_OR_NULL(ar_seu = (struct EUProperty **)
                       track_kmalloc(td->hpeutab.cn_disk *
                                     sizeof(struct EUProperty *), GFP_KERNEL,
                                     M_RECOVER))) {
        ret = -ENOMEM;
        goto l_free_seu;
    }

    if (idx_start == idx_end) {
        LOGINFO("idx_start=idx_end %d this id_hpeu %d no need rebuilding\n",
                idx_start, id_hpeu);
        ret = 0;
        goto l_free_seu;
    }
/////////// get ar_seu
    if ((ret =
         HPEU_get_eus(&td->hpeutab, id_hpeu, ar_seu)) != td->hpeutab.cn_disk) {
        if (ret < 0) {
            LOGERR("This id_hpeu %d ar_eu is empty\n", id_hpeu);
            ret = -ENOEXEC;
            goto l_free_seu;
        } else {
            LOGWARN("id_hpeu %d incompleteness (%d), no need rebuild\n",
                    id_hpeu, ret);
            if (!HPEU_free(&td->hpeutab, id_hpeu)) {
                ret = -ENOEXEC;
            } else {
                ret = 0;
            }

            goto l_free_seu;
        }
        //return true;
    }
//////////// alloc metadata lpn buffer
    //printk("AA: id_hpeu %d", id_hpeu);

    for (i = 0; i < td->hpeutab.cn_disk; i++) {
        retry = 0;
        //printk("(%d) %llu, ", i, ar_seu[i]->start_pos);
        while (IS_ERR_OR_NULL(ar_lpn[i] =
                              track_kmalloc(METADATA_SIZE_IN_FLASH_PAGES *
                                            FLASH_PAGE_SIZE, GFP_KERNEL,
                                            M_RECOVER))) {
            printk("%s: alloc buffer[%d] fail, retry %d\n", __FUNCTION__, i,
                   retry);
            if (retry++ > 3) {
                ret = -ENOMEM;
                goto l_fail;
            }
        }
        memset(ar_lpn[i], TAG_METADATA_EMPTYBYTE,
               METADATA_SIZE_IN_FLASH_PAGES * FLASH_PAGE_SIZE);
    }
    //printk("\n");
    if ((ret = HPEU_metadata_smartread(td, id_hpeu, ar_seu, ar_lpn)) <= 0) {
        goto l_fail;
    }

    if ((ret = HPEU_metadata_commit(td, ar_seu, ar_lpn)) < 0) {
        goto l_fail;
    }

    if (!HPEUrecov_rebuild_bmt
        (id_hpeu, ar_seu, ar_lpn, td, idx_start, idx_end, temperature)) {
        LOGERR("Cannot rebuild bmt\n");
        ret = -ENOEXEC;
        goto l_fail;
    }

    ret = 0;

  l_fail:
    for (i = 0; i < td->hpeutab.cn_disk; i++) {
        if (ar_lpn[i]) {
            track_kfree(ar_lpn[i],
                        METADATA_SIZE_IN_FLASH_PAGES * FLASH_PAGE_SIZE,
                        M_RECOVER);
        }
    }

  l_free_seu:
    if (ar_seu) {
        track_kfree(ar_seu, td->hpeutab.cn_disk * sizeof(struct EUProperty *),
                    M_RECOVER);
    }

  l_free_lpn:
    if (ar_lpn) {
        track_kfree(ar_lpn, td->hpeutab.cn_disk * sizeof(sector64 *),
                    M_RECOVER);
    }

    return ret;
}

static bool HPEUrecov_rebuild_perchunk(lfsm_dev_t * td,
                                       HPEUrecov_param_t * param,
                                       int32_t temperature)
{
    HPEUrecov_entry_t *rec_entry, *rec_next;
    HPEUrecov_entry_t *first, *end;
    struct list_head *phpeu_list;
    bool ret;

    phpeu_list = &param->lst_id_hpeu;
    ret = true;

    if ((param->idx_start == -1) || (param->idx_end == -1)) {
        LOGERR("rebuild entry fail {%d,%d}\n", param->idx_start,
               param->idx_end);
        return false;
    }

    first = (HPEUrecov_entry_t *) phpeu_list->next;
    end = (HPEUrecov_entry_t *) phpeu_list->prev;
    list_for_each_entry_safe(rec_entry, rec_next, phpeu_list, link) {
        if (HPEUrecov_rebuild_hpeu(td, rec_entry->id_hpeu,
                                   (rec_entry == first) ? param->idx_start : 0,
                                   (rec_entry ==
                                    end) ? param->idx_end : FPAGE_PER_SEU *
                                   td->hpeutab.cn_disk, temperature) < 0) {
            ret = false;
            LOGERR(" rebuild entry id_hepu %d \n", rec_entry->id_hpeu);
            break;
        }

        if ((rec_entry != end) || ((rec_entry == end) &&
                                   (param->idx_end ==
                                    FPAGE_PER_SEU * td->hpeutab.cn_disk))) {
            list_del_init(&rec_entry->link);
            track_kfree(rec_entry, sizeof(HPEUrecov_entry_t), M_RECOVER);
        }
    }

    if (list_empty(phpeu_list)) {
        param->idx_start = -1;
    } else {
        // after the above for loop, the phpeu_list list contain one entry at most.
        param->idx_start = param->idx_end;
    }

    param->idx_end = -1;
    return ret;
}

static bool HPEUrecov_rebuild_perchunk_all(lfsm_dev_t * td,
                                           HPEUrecov_param_t **
                                           ar_hpeu_recov_param)
{
    int32_t i, j;
    bool ret;

    ret = true;
    for (i = 0; i < TEMP_LAYER_NUM; i++) {
        for (j = 0; j < td->cn_pgroup; j++) {
            if (list_empty(&ar_hpeu_recov_param[j][i].lst_id_hpeu)) {
                continue;
            }

            if (!HPEUrecov_rebuild_perchunk(td, &ar_hpeu_recov_param[j][i], i)) {
                return false;
            }
        }
    }

    return ret;
}

static int32_t HPEUrecov_setup_entry_UL(BMT_update_record_t * ul_entry,
                                        int32_t * ar_erase_idx,
                                        int32_t ul_idx_page,
                                        HPEUrecov_param_t **
                                        ar_hpeu_recov_param)
{
    HPEUrecov_param_t *param;
    int32_t id_hpeu, temp, id_disk;

    id_hpeu = ul_entry->n_record.id_hpeu;
    temp = ul_entry->n_record.temperature;
    id_disk = ul_entry->n_record.disk_num;
    param = &ar_hpeu_recov_param[HPEU_HPEUOFF2EUOFF(id_disk)][temp];

    if (!HPEUrecov_isvalid_id_hpeu(ul_idx_page, ar_erase_idx[id_hpeu])) {
        return 0;
    }

    if (list_empty(&param->lst_id_hpeu)) {
        param->idx_start = 0;   // the id_hpeu is inserted below
    } else if (HPEUrecov_entry_isequal_id(param, id_hpeu, true)) {  //support this id_hpeu has been inserted
        return 0;               // only one hpeu will be in allocation for one temperature.
    }

    if (!HPEUrecov_entry_alloc_insert(id_hpeu, &param->lst_id_hpeu)) {
        return -ENOMEM;
    }

    rec_printk("RECOV: LOG(%x) id_pheu %d temp %d pos %llu\n",
               ul_entry->signature, id_hpeu, temp, ul_entry->n_record.eu_start);
    return 0;
}

// there are three kinds of updatelog marker: UL_NORMAL_MARK(seu used), UL_GC_MARK(gc), EMPTY_SIGNATURE(finish end)
// return 0 is setup ok, -1 is setup fail, 1 is setup ok and the end of logging mark
static int32_t HPEUrecov_setup_entry(lfsm_dev_t * td,
                                     BMT_update_record_t * ul_entry,
                                     int32_t * ar_idx_erase, int32_t ul_idx_pg,
                                     HPEUrecov_param_t ** ar_hpeu_recov_param)
{
    if (ul_entry->signature == UL_NORMAL_MARK) {
        return HPEUrecov_setup_entry_UL(ul_entry, ar_idx_erase, ul_idx_pg,
                                        ar_hpeu_recov_param);
    } else if (ul_entry->signature == UL_GC_MARK) {
        if (HPEUrecov_setup_entry_GC(td, ul_entry, ar_idx_erase, ul_idx_pg,
                                     ar_hpeu_recov_param) < 0) {
            return -1;
        }

        if (!HPEUrecov_rebuild_perchunk_all(td, ar_hpeu_recov_param)) {
            return -1;
        }
    } else if (ul_entry->signature == UL_EMPTY_MARK) {  //end of UL
        if (HPEUrecov_setup_entry_EMPTY(td, ar_hpeu_recov_param) < 0) {
            return -1;
        }

        if (!HPEUrecov_rebuild_perchunk_all(td, ar_hpeu_recov_param)) {
            return -1;
        }
        return 1;
    }

    return 0;
}

static void HPEUrecov_free_entry(HPEUrecov_param_t ** ar_hpeu_recov_param,
                                 int32_t cn_group, int32_t cn_temp_layer)
{
    HPEUrecov_entry_t *rec_entrey, *rec_next;
    int32_t i, j;

    for (i = 0; i < cn_group; i++) {
        for (j = 0; j < cn_temp_layer; j++) {
            list_for_each_entry_safe(rec_entrey, rec_next,
                                     &ar_hpeu_recov_param[i][j].lst_id_hpeu,
                                     link) {
                list_del_init(&rec_entrey->link);
                track_kfree(rec_entrey, sizeof(HPEUrecov_entry_t), M_RECOVER);
            }
        }
    }
}

static int32_t HPEUrecov_param_init(HPEUrecov_param_t ** ar_hpeu_recov_param,
                                    int32_t cn_pgroup, int32_t cn_temp_layer)
{
    int32_t i, j;

    for (i = 0; i < cn_pgroup; i++) {
        for (j = 0; j < cn_temp_layer; j++) {
            INIT_LIST_HEAD(&ar_hpeu_recov_param[i][j].lst_id_hpeu);
            ar_hpeu_recov_param[i][j].idx_start = -1;
            ar_hpeu_recov_param[i][j].idx_end = -1;
        }
    }

    return 0;
}

static int32_t HPEUrecov_scan_and_rebuild(lfsm_dev_t * td,
                                          int32_t * ar_erasemark)
{
    HPEUrecov_param_t **ar_hpeu_recov_param;    //[LFSM_PGROUP_NUM][TEMP_LAYER_NUM];
    sector64 pos_main, pos_backup, st_pos;
    int8_t *buf;
    int32_t i, idx_st_hpage, ret;

    ret = -1;

    pos_main = DEV_SYS_READY(grp_ssds, td->UpdateLog.eu_main->disk_num) ?
        td->UpdateLog.eu_main->start_pos : -1;
    pos_backup = DEV_SYS_READY(grp_ssds, td->UpdateLog.eu_backup->disk_num) ?
        td->UpdateLog.eu_backup->start_pos : -1;

    ar_hpeu_recov_param = (HPEUrecov_param_t **)
        track_kmalloc(td->cn_pgroup * sizeof(HPEUrecov_param_t *),
                      GFP_KERNEL | __GFP_ZERO, M_RECOVER);
    if (IS_ERR_OR_NULL(ar_hpeu_recov_param)) {
        return -ENOMEM;
    }

    for (i = 0; i < td->cn_pgroup; i++) {
        ar_hpeu_recov_param[i] = (HPEUrecov_param_t *)
            track_kmalloc(TEMP_LAYER_NUM * sizeof(HPEUrecov_param_t),
                          GFP_KERNEL, M_RECOVER);
        if (IS_ERR_OR_NULL(ar_hpeu_recov_param[i])) {
            goto l_param_fail;
        }
    }

    if (HPEUrecov_param_init(ar_hpeu_recov_param, td->cn_pgroup, TEMP_LAYER_NUM)
        < 0) {
        return -EINVAL;
    }

    if (IS_ERR_OR_NULL(buf = (int8_t *)
                       track_kmalloc(HARD_PAGE_SIZE, __GFP_ZERO, M_RECOVER))) {
        LOGERR("alloc buffer fail\n");
        return -ENOMEM;
    }

    idx_st_hpage = 0;

    for (i = idx_st_hpage; i < HPAGE_PER_SEU; i++) {
        // read updatelog
        st_pos = pos_main;
        if ((st_pos == -1) ||
            !devio_read_hpages(st_pos + (i << SECTORS_PER_HARD_PAGE_ORDER), buf,
                               1)) {
            st_pos = pos_backup;
            if ((st_pos == -1) ||
                !devio_read_hpages(st_pos + (i << SECTORS_PER_HARD_PAGE_ORDER),
                                   buf, 1)) {
                LOGERR(" devio_read_pages fail\n");
                goto l_buf_fail;
            }
        }
        // setup recovery entry
        ret = HPEUrecov_setup_entry(td, (BMT_update_record_t *) buf,
                                    ar_erasemark, i, ar_hpeu_recov_param);
        if (ret < 0) {          // failure
            HPEUrecov_free_entry(ar_hpeu_recov_param, td->cn_pgroup,
                                 TEMP_LAYER_NUM);
            LOGERR("can't recovery idx %d \n", i);
            goto l_buf_fail;
        } else if (ret == 1) {  // the end of the update log
            td->UpdateLog.eu_main->log_frontier =
                (i + 1) << SECTORS_PER_HARD_PAGE_ORDER;
            td->UpdateLog.eu_backup->log_frontier =
                (i + 1) << SECTORS_PER_HARD_PAGE_ORDER;

            break;
        }
        // else ret == 0 implies continue
    }

    ret = 0;

  l_buf_fail:
    track_kfree(buf, HARD_PAGE_SIZE, M_RECOVER);
  l_param_fail:
    for (i = 0; i < td->cn_pgroup; i++) {
        if (ar_hpeu_recov_param[i]) {
            track_kfree(ar_hpeu_recov_param[i],
                        TEMP_LAYER_NUM * sizeof(HPEUrecov_param_t), M_RECOVER);
        }
    }

    track_kfree(ar_hpeu_recov_param,
                td->cn_pgroup * sizeof(HPEUrecov_param_t *), M_RECOVER);

    return ret;
}

bool HPEUrecov_crash_recovery(lfsm_dev_t * td)
{
    struct HPEUTab *hpeutb;
    int32_t *ar_hpeu_ermark;
    bool ret;

    ret = false;
    hpeutb = &td->hpeutab;
    LOGINFO("ulog recovery start: {%llu, %llu}\n",
            td->UpdateLog.eu_main->start_pos,
            td->UpdateLog.eu_backup->start_pos);
    if (IS_ERR_OR_NULL(ar_hpeu_ermark = (int32_t *)
                       track_kmalloc(sizeof(int32_t) * hpeutb->cn_total,
                                     GFP_KERNEL, M_RECOVER))) {
        LOGERR(" Fail to allocate erase log array\n");
        goto l_ermark_fail;
    }
    memset(ar_hpeu_ermark, -1, sizeof(int32_t) * hpeutb->cn_total);
    // IOScan update log (first time, up to down)
    if (!HPEUrecov_ul_prescan(td, ar_hpeu_ermark)) {
        goto l_ermark_fail;
    }
    // IOScan update log (second time, up to down)
    if (HPEUrecov_scan_and_rebuild(td, ar_hpeu_ermark) < 0) {
        goto l_ermark_fail;
    }

    ret = true;

  l_ermark_fail:
    track_kfree(ar_hpeu_ermark, sizeof(int32_t) * hpeutb->cn_total, M_RECOVER);
    return ret;
}
