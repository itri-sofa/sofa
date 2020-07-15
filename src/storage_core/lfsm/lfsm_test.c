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
#include "lfsm_test.h"
#include "spare_bmt_ul.h"
#include "bmt.h"
#include "GC.h"
#include "mdnode.h"
#include "EU_create.h"
#include "metabioc.h"
#include "system_metadata/Super_map.h"
#include "ioctl.h"

/*
AN add test function
*/
static int32_t check_valid_bitmap_bmt_new(lfsm_dev_t * td,
                                          struct EUProperty *src_eu,
                                          int32_t idx, sector64 chk_lbn,
                                          sftl_err_msg_t * perr_msg)
{
    sftl_err_item_t err_item;
    struct D_BMT_E dbmta;
    sector64 org_pbn;
    int32_t ret, err_code;      //valid
    bool testbit;

    ret = 1;
    err_code = 0;
    org_pbn = src_eu->start_pos + (idx << SECTORS_PER_FLASH_PAGE_ORDER);
    testbit = test_bit((idx % PAGES_PER_BYTE),
                       (void *)&src_eu->bit_map[idx >> PAGES_PER_BYTE_ORDER]);

    memset(&err_item, 0, sizeof(sftl_err_item_t));

    err_item.id_eu = PBNO_TO_EUID(src_eu->start_pos);
    err_item.org_pbn = org_pbn;
    err_item.testbit = testbit;
    err_item.lpno = chk_lbn;

    if (RDDCY_IS_ECCLPN(chk_lbn) || chk_lbn == LBNO_PREPADDING_PAGE) {
        if (!testbit) {
            goto l_end;
        }

        LOGERR("ERROR: Is Ecc and valid lpn %llu pbn %llu\n", chk_lbn, src_eu->start_pos + (idx << SECTORS_PER_FLASH_PAGE_ORDER));  // Err 1
        err_code = -ECCPAGE_BUT_TESTBIT1;
        goto l_end;

    } else if (chk_lbn >= td->logical_capacity || chk_lbn < 0) {
        LOGERR("ERROR: eu's id_hpeu %d :wrong lpn %llu  %llx in pbn %llu\n", src_eu->id_hpeu, chk_lbn, chk_lbn, org_pbn);   // ERR2
        err_code = -ELPNO_EXCEED_SIZE;
        goto l_end;
    }

    ret = bmt_lookup(td, td->ltop_bmt, chk_lbn, false, 0, &dbmta);

    err_item.bmt_pbn = dbmta.pbno;
    err_item.ret_bmt_lookup = ret;

    if (ret < 0 && ret != -EACCES) {
        LOGERR("bmt_lookup lpn %llu, ret= %d\n", chk_lbn, ret); // Err3
        err_code = -ERET_BMT_LOOKUP;
        goto l_end;
    }

    if (!testbit && dbmta.pbno == org_pbn) {
        LOGERR("lpn %llu org_pbno is %llu match bmt %llu but testbit is one\n",
               chk_lbn, org_pbn, (sector64) dbmta.pbno);
        err_code = -ETESTBIT0;
        goto l_end;
    } else if (testbit && dbmta.pbno != org_pbn) {
        LOGERR
            ("lpn %llu org_pbno is %llu not match bmt %llu but testbit is zero\n",
             chk_lbn, org_pbn, (sector64) dbmta.pbno);
        err_code = -ETESTBIT1;
        goto l_end;
    }

  l_end:
    if (err_code < 0 && perr_msg) {
        err_item.err_code = err_code;
        memcpy(&perr_msg->ar_item[perr_msg->cn_err++], &err_item,
               sizeof(sftl_err_item_t));
    }
    return err_code;
}

/*
check page index is valid or not, if valid do bmt_lookup to check lpn match
return 0: is invalid page, 1: is valid page, -1: is wrong valid page
*/
static int32_t check_valid_bitmap_bmt(lfsm_dev_t * td,
                                      struct EUProperty *src_eu, int32_t idx,
                                      sector64 chk_lbn)
{
    struct D_BMT_E dbmta;
    sector64 org_pbn;
    int32_t ret;                //valid

    ret = 1;
    if (!test_bit((idx % PAGES_PER_BYTE), (void *)
                  &src_eu->bit_map[idx >> PAGES_PER_BYTE_ORDER])) {
        return 0;
    }

    if (RDDCY_IS_ECCLPN(chk_lbn)) {
        printk("ERROR: Is Ecc and valid lpn %llu pbn %llu\n",
               chk_lbn,
               src_eu->start_pos + (idx << SECTORS_PER_FLASH_PAGE_ORDER));
        return -1;
    }

    org_pbn = src_eu->start_pos + (idx << SECTORS_PER_FLASH_PAGE_ORDER);
    if (chk_lbn >= td->logical_capacity || chk_lbn < 0) {
        printk("ERROR: wrong lpn %llu in pbn %llu\n", chk_lbn, org_pbn);
        return -1;
    }

    ret = bmt_lookup(td, td->ltop_bmt, chk_lbn, false, 0, &dbmta);

    if (ret < 0) {
        return -1;
    }

    if (dbmta.pbno != org_pbn) {
        printk("%s lpn %llu org_pbno is %llu not match bmt %llu\n",
               __FUNCTION__, chk_lbn, org_pbn, (sector64) dbmta.pbno);
        ret = -1;
    }

    return ret;
}

int32_t check_hpeu_table(lfsm_dev_t * td, int32_t id_hpeu,
                         sftl_err_msg_t * perr_msg)
{
    struct EUProperty *ar_peu[td->hpeutab.cn_disk];
    sftl_err_item_t err_item;
    struct HPEUTab *phpeutab;
    int32_t cn_alloc, i;

    phpeutab = &td->hpeutab;
    memset(&err_item, 0, sizeof(sftl_err_item_t));

    err_item.id_hpeu = id_hpeu;

    if ((cn_alloc = HPEU_get_eus(phpeutab, id_hpeu, ar_peu)) < 0) {
        goto l_end;
    }

    if (cn_alloc < phpeutab->cn_disk) {
        err_item.cn_eu_inhpeu = cn_alloc;
        err_item.err_code = -EEU_HPEUID;
        goto l_end;
    }

    for (i = 0; i < phpeutab->cn_disk; i++) {
        if (ar_peu[i]->id_hpeu == id_hpeu) {
            continue;
        }

        err_item.err_code = -EEU_HPEUID;
        err_item.id_eu = PBNO_TO_EUID(ar_peu[i]->start_pos);
        goto l_end;
    }

  l_end:
    if (err_item.err_code < 0 && perr_msg) {
        memcpy(&perr_msg->ar_item[perr_msg->cn_err++], &err_item,
               sizeof(sftl_err_item_t));
    }

    return err_item.err_code;
}

int32_t check_eu_mdnode(lfsm_dev_t * td, struct EUProperty *eu,
                        sftl_err_msg_t * perr_msg)
{
    sftl_err_item_t err_item;

    memset(&err_item, 0, sizeof(sftl_err_item_t));
    err_item.id_eu = PBNO_TO_EUID(eu->start_pos);

    if (eu_is_active(eu) >= 0) {
        if (atomic_read(&eu->act_metadata.fg_state) != md_exist) {
            LOGERR("active EU %p id %d active_metadata nonexist\n",
                   eu, PBNO_TO_EUID(eu->start_pos));
            err_item.err_code = -EACTIVE_EU_MDNODE_NONEXIST;
            goto l_end;
        }

        if (mdnode_item_added(eu->act_metadata.mdnode) !=
            eu->log_frontier >> SECTORS_PER_FLASH_PAGE_ORDER) {
            LOGERR("active EU %p id %d mdnode number not equal \n", eu,
                   PBNO_TO_EUID(eu->start_pos));
            err_item.err_code = -EACTIVE_EU_MDNODE_INVALID;
            goto l_end;
        }
    } else {
        if (atomic_read(&eu->act_metadata.fg_state) == md_exist) {
            LOGERR("nonactive EU %p id %d active_metadata exist\n",
                   eu, PBNO_TO_EUID(eu->start_pos));
            err_item.err_code = -ENONACTIVE_EU_MDNODE_EXIST;
            goto l_end;
        }

        if (eu->act_metadata.mdnode) {
            LOGERR("nonactive EU %p id %d active_metadata exist\n",
                   eu, PBNO_TO_EUID(eu->start_pos));
            err_item.err_code = -ENONACTIVE_EU_MDNODE_VALID;
            goto l_end;
        }
    }

  l_end:
    if ((err_item.err_code < 0) && perr_msg) {
        memcpy(&perr_msg->ar_item[perr_msg->cn_err++], &err_item,
               sizeof(sftl_err_item_t));
    }

    return err_item.err_code;
}

int32_t check_eu_bitmap_validcount(lfsm_dev_t * td, struct EUProperty *eu,
                                   sftl_err_msg_t * perr_msg)
{
    sftl_err_item_t err_item;
    int32_t i, cn_valid;

    memset(&err_item, 0, sizeof(sftl_err_item_t));

    cn_valid = 0;
    err_item.id_eu = PBNO_TO_EUID(eu->start_pos);

    for (i = 0; i < DATA_FPAGE_IN_SEU; i++) {
        if (test_bit
            (i % PAGES_PER_BYTE, (void *)(&eu->bit_map[i / PAGES_PER_BYTE]))) {
            cn_valid++;
        }
    }

    if ((cn_valid * SECTORS_PER_FLASH_PAGE) != atomic_read(&eu->eu_valid_num)) {    // sector based
        err_item.err_code = -EEU_VALID_COUNT;
        LOGERR("EU %p valid count incorrect cn_valid:%d  eu_valid_num :%d "
               "stat %d, eu_id%d\n", eu,
               (cn_valid * SECTORS_PER_FLASH_PAGE),
               atomic_read(&eu->eu_valid_num), atomic_read(&eu->state_list),
               PBNO_TO_EUID(eu->start_pos));
        if (perr_msg) {
            memcpy(&perr_msg->ar_item[perr_msg->cn_err++], &err_item,
                   sizeof(sftl_err_item_t));
        }
    }

    return err_item.err_code;
}

int32_t check_eu_idhpeu(lfsm_dev_t * td, struct EUProperty *eu,
                        sftl_err_msg_t * perr_msg)
{
    sftl_err_item_t err_item;
    HPEUTabItem_t *pitem;

    memset(&err_item, 0, sizeof(sftl_err_item_t));

    if (eu->id_hpeu < 0) {
        goto l_end;
    }

    err_item.id_eu = PBNO_TO_EUID(eu->start_pos);
    err_item.id_hpeu = eu->id_hpeu;

    if (IS_ERR_OR_NULL(pitem = HPEU_GetTabItem(&td->hpeutab, eu->id_hpeu))
        || pitem->cn_added < 0) {
        LOGERR("eu_id %d  hpeu_id %d , pItem %p\n", PBNO_TO_EUID(eu->start_pos),
               eu->id_hpeu, pitem);
        err_item.err_code = -EEU_HPEUID;
        goto l_end;
    }

    if (pitem->ar_peu[HPEU_HPEUOFF2EUIDX(eu->disk_num)] != eu) {
        LOGERR("EU(%p)'s id_hpeu(%d) doesn't match the eu(%p) in hpeutab \n",
               eu, eu->id_hpeu,
               pitem->ar_peu[HPEU_HPEUOFF2EUIDX(eu->disk_num)]);
        err_item.err_code = -EEU_HEPUID_NO_MATCH;
        goto l_end;
    }

  l_end:
    if (err_item.err_code < 0 && perr_msg) {
        memcpy(&perr_msg->ar_item[perr_msg->cn_err++], &err_item,
               sizeof(sftl_err_item_t));
    }

    return err_item.err_code;
}

int32_t get_metadata_with_check_new(lfsm_dev_t * td, struct EUProperty *src_eu,
                                    sftl_err_msg_t * perr_msg)
{
    sector64 *md_mem;
    sector64 pbn, ret, cn_checked, checksum;
    int32_t i, cn_valid_lpn;

    ret = 0;
    cn_checked = 0;
    checksum = 0;
    cn_valid_lpn = DATA_FPAGE_IN_SEU;
    md_mem =
        (sector64 *) track_vmalloc(METADATA_SIZE_IN_BYTES, GFP_KERNEL,
                                   M_UNKNOWN);

    //// get metadata ////
    if (atomic_read(&src_eu->state_list) < 0 && eu_is_active(src_eu) < 0) {
        goto l_end;
    }

    if (!IS_ERR_OR_NULL(src_eu->act_metadata.mdnode)) {
        cn_valid_lpn = src_eu->log_frontier >> SECTORS_PER_FLASH_PAGE_ORDER;
        LOGINFO("EU %p (%llu) with EU mdnode valid_page %d \n",
                src_eu, src_eu->start_pos, cn_valid_lpn);
        for (i = 0; i < cn_valid_lpn; i++) {
            md_mem[i] = src_eu->act_metadata.mdnode->lpn[i];
        }
    } else if (!src_eu->fg_metadata_ondisk) {   //get metadata from spare area
        LOGINFO("EU %p (%llu) read metadata from spare area\n", src_eu,
                src_eu->start_pos);
        if (!EU_spare_read
            (td, (int8_t *) md_mem, src_eu->start_pos, FPAGE_PER_SEU)) {
            LOGERR("EU %p (%llu) read spare fail \n", src_eu,
                   src_eu->start_pos);
            if (perr_msg) {
                perr_msg->ar_item[perr_msg->cn_err].err_code = -ENO_META_DATA;
                perr_msg->ar_item[perr_msg->cn_err++].org_pbn =
                    src_eu->start_pos;
            }
            goto l_end;
        }
    } else {                    //get metadata from EU
        LOGINFO("EU %p (%llu) read metadata form EU metadata page\n", src_eu,
                src_eu->start_pos);
        pbn = src_eu->start_pos + SECTORS_PER_SEU - METADATA_SIZE_IN_SECTOR;
        if (!devio_read_pages
            (pbn, (int8_t *) md_mem, METADATA_SIZE_IN_FLASH_PAGES, NULL)) {
            LOGERR("EU %p (%llu) read metadata fail\n", src_eu,
                   src_eu->start_pos);
            if (perr_msg) {
                perr_msg->ar_item[perr_msg->cn_err].err_code = -ENO_META_DATA;
                perr_msg->ar_item[perr_msg->cn_err++].org_pbn =
                    src_eu->start_pos;
            }
            goto l_end;
        }

        if ((checksum = metabioc_CalcChkSumB(md_mem, 0, DATA_FPAGE_IN_SEU + 1)) != 0) { // +1 is include checksum, and xor should be zero
            if (perr_msg) {
                perr_msg->ar_item[perr_msg->cn_err].err_code = -ENO_META_DATA;
                perr_msg->ar_item[perr_msg->cn_err++].org_pbn =
                    src_eu->start_pos;
            }
            LOGERR("Error@%s :eu %lld checksum=%08llx should be 0!\n",
                   __FUNCTION__, src_eu->start_pos, checksum);
            goto l_end;
        }
    }

    for (i = 0; i < cn_valid_lpn; i++) {
        if (check_valid_bitmap_bmt_new(td, src_eu, i, md_mem[i], perr_msg) < 0) {
            ret++;
        } else {
            cn_checked++;
        }
    }

  l_end:
    track_vfree(md_mem, METADATA_SIZE_IN_BYTES, M_UNKNOWN);
    if (cn_checked != 0 || ret != 0) {
        LOGINFO("final incorrect %llu, corr %llu\n", ret, cn_checked);
    }

    return ret;
}

int32_t get_metadata_with_check(lfsm_dev_t * td, struct EUProperty *src_eu,
                                sftl_err_msg_t * perr_msg)
{
    struct page *plpn_page, *md_page[METADATA_SIZE_IN_PAGES] = { NULL };
    sector64 *lpn_list;
    struct HListGC *h;
    sector64 chk_lbn, pbn;
    int32_t ret, err, retry;
    int32_t i, j, k, idx = 0;

    i = 0;
    ret = 0;
    retry = 0;
    plpn_page = track_alloc_page(__GFP_ZERO);
    LFSM_ASSERT(plpn_page != NULL);

    //// get metadata ////
    if (!IS_ERR_OR_NULL(src_eu->act_metadata.mdnode)) {
        for (i = 0; i < src_eu->log_frontier >> SECTORS_PER_FLASH_PAGE_ORDER;
             i++) {
            if (check_valid_bitmap_bmt
                (td, src_eu, i, src_eu->act_metadata.mdnode->lpn[i]) < 0) {
                ret++;
            }
        }
    } else if (!src_eu->fg_metadata_ondisk) {   //get metadata from spare area
        printk("EU %d (%llu) need to read spare area\n",
               (int32_t) ((src_eu->start_pos - eu_log_start) >> EU_BIT_SIZE),
               src_eu->start_pos);
        pbn = src_eu->start_pos;
        for (j = 0; j < METADATA_SIZE_IN_PAGES + 1; j++) {
            lpn_list = (sector64 *) kmap(plpn_page);
            devio_query_lpn((int8_t *) lpn_list + LPN_QUERY_HEADER, pbn,
                            MAX_LPN_ONE_SHOT << sizeof(sector64));

            for (k = 0; k < MAX_LPN_ONE_SHOT; k++) {
                idx = j * MAX_LPN_ONE_SHOT + k;
                if (idx >= (FPAGE_PER_SEU - METADATA_SIZE_IN_FLASH_PAGES)) {
                    break;
                }
                chk_lbn = lpn_list[k + LPN_QUERY_HEADER / sizeof(sector64)];

                if (SPAREAREA_IS_EMPTY(chk_lbn)) {
                    break;
                }
                if (check_valid_bitmap_bmt(td, src_eu, idx, chk_lbn) < 0) {
                    ret++;
                }
            }
            kunmap(plpn_page);
            pbn += MAX_LPN_ONE_SHOT * SECTORS_PER_FLASH_PAGE;   // Tifa: 2^nK safe
        }
    } else {                    //get metadata from EU
        printk("EU %d (%llu) with EU metadata\n", i, src_eu->start_pos);
        h = gc[src_eu->disk_num];

        LFSM_MUTEX_LOCK(&(h->lock_metadata_io_bio));
        while ((err =
                gc_read_eu_metadata(td, h, src_eu->start_pos,
                                    &md_page[0])) != 1) {
            if (err == 0) {
                printk("EU %d (%llu) without metadata\n", i, src_eu->start_pos);
                return ret;
            }

            if (retry++ > 3) {
                printk("ERROR (BAD FAIL): metadata read fail eu %llu\n",
                       src_eu->start_pos);
                return ret;
            }
        }

        for (j = 0; j < METADATA_SIZE_IN_PAGES; j++) {
            if (IS_ERR_OR_NULL(lpn_list = (sector64 *) kmap(md_page[j]))) {
                printk("BAD FAIL: metadata read fail eu %llu\n",
                       src_eu->start_pos);
                break;
            }

            for (k = 0; k < METADATA_ENTRIES_PER_PAGE; k++) {
                idx = j * METADATA_ENTRIES_PER_PAGE + k;
                if (idx >= (FPAGE_PER_SEU - METADATA_SIZE_IN_FLASH_PAGES)) {
                    break;
                }

                if (check_valid_bitmap_bmt(td, src_eu, idx, lpn_list[k]) < 0) {
                    ret++;
                }
            }
        }
        LFSM_MUTEX_UNLOCK(&(h->lock_metadata_io_bio));
    }

    return ret;
}

////// return error number, <0 means fail return, 0 means success
static int32_t check_EUmeta_and_sparelpn(lfsm_dev_t * td, struct EUProperty *eu,
                                         sftl_err_msg_t * perr_msg)
{
    sector64 *lpn_sp, *lpn_md;
    int8_t *buf_spare, *buf_meta;
    int32_t i, ret;

    ret = 0;
    buf_spare = NULL;
    buf_meta = NULL;

    ///////////// boundary check /////////////
    if (PbnoBoundaryCheck(td, eu->start_pos)) {
        printk("Error: eu (%p) addr %llu\n", eu, eu->start_pos);
        return 0;
    }
    ///////////// skip active eu /////////////
    if (eu_is_active(eu) >= 0) {
        printk(" no check EUmetadata, return\n");
        return 0;
    }
    ////////////// memory alloc ////////////
    if (IS_ERR_OR_NULL(buf_spare = (int8_t *)
                       track_kmalloc(SZ_SPAREAREA * FPAGE_PER_SEU, GFP_KERNEL,
                                     M_UNKNOWN))) {
        printk("%s alloc mem failure\n", __FUNCTION__);
        ret = -ENOMEM;
        goto l_end;
    }

    if (IS_ERR_OR_NULL(buf_meta = (int8_t *)
                       track_kmalloc(sizeof(sector64) * FPAGE_PER_SEU,
                                     GFP_KERNEL, M_UNKNOWN))) {
        printk("%s alloc mem failure\n", __FUNCTION__);
        ret = -ENOMEM;
        goto l_end;
    }
    /////////// read spare and metadata ///////////
    if (!EU_spare_read(td, buf_spare, eu->start_pos, FPAGE_PER_SEU)) {
        ret = -EIO;
        goto l_end;
    }

    if (!devio_read_pages(eu->start_pos + DATA_SECTOR_IN_SEU,
                          buf_meta, METADATA_SIZE_IN_FLASH_PAGES, NULL)) {
        ret = -EIO;
        goto l_end;
    }
    ////////// compare spare and metadata ////////////
    lpn_sp = (sector64 *) buf_spare;
    lpn_md = (sector64 *) buf_meta;
    for (i = 0; i < DATA_FPAGE_IN_SEU; i++) {
        if ((lpn_md[i] & MASK_SPAREAREA) != lpn_sp[i]) {
            ret++;
            printk
                ("ERROR eu %llu page %d {meta,lpn}={%llx,%llx} {%llu ,%llu} ret %d\n",
                 eu->start_pos, i, lpn_md[i], lpn_sp[i], lpn_md[i], lpn_sp[i],
                 ret);
        }
    }

  l_end:
    if (buf_spare) {
        track_kfree(buf_spare, SZ_SPAREAREA * FPAGE_PER_SEU, M_UNKNOWN);
    }

    if (buf_meta) {
        track_kfree(buf_meta, sizeof(sector64) * FPAGE_PER_SEU, M_UNKNOWN);
    }
    return ret;
}

static int32_t scan_allEU_for_pfun(lfsm_dev_t * td, fp pfun)
{
    struct HListGC *h;
    struct EUProperty *pEU;
    int32_t ret, i, j;

    ret = 0;

    printk("~~~Start %s ~~~\n", __FUNCTION__);
    metalog_load_all(&td->mlogger, td);
    // scan for all data EU except in free list
    // do bmt_lookup and check pbn if match
    // return or printk non-match (lpn, pbn) pair
    for (i = 0; i < td->cn_ssds; i++) {
        h = gc[i];
        // scan from active
        printk("scan active\n");
        for (j = 0; j < TEMP_LAYER_NUM; j++) {
            ret += pfun(td, h->active_eus[j], NULL);
        }

        LFSM_RMUTEX_LOCK(&h->whole_lock);

        // scan from hlist
        printk("scan hlist %d\n", h->hlist_number);
        list_for_each_entry_reverse(pEU, &h->hlist, list) {
            ret += pfun(td, pEU, NULL);
        }

        printk("scan heap %d\n", h->LVP_Heap_number);
        // scan from heap
        for (j = 0; j < h->LVP_Heap_number; j++) {
            ret += pfun(td, h->LVP_Heap[j], NULL);
        }

        LFSM_RMUTEX_UNLOCK(&h->whole_lock);

    }
    printk("%s Done ~~~\n", __FUNCTION__);
    return ret;
}

int32_t check_allEU_valid_lpn(lfsm_dev_t * td)
{
    return scan_allEU_for_pfun(td, get_metadata_with_check);
}

int32_t check_allEU_valid_lpn_new(lfsm_dev_t * td)
{
    return scan_allEU_for_pfun(td, get_metadata_with_check_new);
}

int32_t check_one_lpn(lfsm_dev_t * td, sector64 lpn, sftl_err_msg_t * perr_msg)
{
    sftl_err_item_t err_item;
    struct D_BMT_E dbmta;
    struct EUProperty *eu;
    int32_t ret;

    ret = 0;
    memset(&err_item, 0, sizeof(sftl_err_item_t));

    ret = bmt_lookup(td, td->ltop_bmt, lpn, false, false, &dbmta);
    if (ret < 0 && ret != -EACCES) {
        LOGERR("bmt_lookup for lpn %llu is %d\n", lpn, ret);
        err_item.err_code = -ERET_BMT_LOOKUP;
        goto l_end;
    }

    if (dbmta.pbno == PBNO_SPECIAL_NEVERWRITE) {
        goto l_end;
    }

    err_item.org_pbn = dbmta.pbno;

    if ((eu = eu_find(dbmta.pbno)) < 0) {
        LOGERR("lpn %llu 's pbno %llu is invalid\n", lpn,
               (sector64) dbmta.pbno);
        err_item.err_code = -EPBNO_WRONG;
        goto l_end;
    }

    if (!test_pbno_bitmap(dbmta.pbno)) {
        LOGERR("lpn %llu 's pbno %llu is zero\n", lpn, (sector64) dbmta.pbno);
        err_item.err_code = -ETESTBIT0;
    }

  l_end:
    if (err_item.err_code < 0 && perr_msg) {
        memcpy(&perr_msg->ar_item[perr_msg->cn_err++], &err_item,
               sizeof(sftl_err_item_t));
    }

    return err_item.err_code;
}

int32_t check_all_lpn(lfsm_dev_t * td)
{
    sector64 lpn, cn_wrong, cn_check;
    int32_t ret;

    cn_wrong = 0;
    cn_check = 0;
    for (lpn = 0; lpn < td->logical_capacity; lpn++) {
        cn_check++;

        if ((ret = check_one_lpn(td, lpn, NULL)) < 0) {
            cn_wrong++;
        }
    }

    LOGINFO("total wrong lpn  %llu,total check %llu \n", cn_wrong, cn_check);
    return 0;
}

int32_t check_allEU_EUmeta_spare(lfsm_dev_t * td)
{
    return scan_allEU_for_pfun(td, check_EUmeta_and_sparelpn);
}

void print_eu_erase_count(struct HListGC *h)
{
    struct EUProperty *eu;
    int32_t i;

    for (i = 0; i < h->LVP_Heap_number; i++) {
        if (h->LVP_Heap[i]->erase_count == EU_UNUSABLE) {
            continue;
        }
        printk("EU %llu (%llu) erase_count %d valid_page %d\n",
               h->LVP_Heap[i]->start_pos >> SECTORS_PER_FLASH_PAGE_ORDER,
               h->LVP_Heap[i]->start_pos, h->LVP_Heap[i]->erase_count,
               atomic_read(&h->LVP_Heap[i]->eu_valid_num));
    }

    list_for_each_entry_reverse(eu, &h->hlist, list) {
        if (eu->erase_count == EU_UNUSABLE) {
            continue;
        }
        printk("EU %llu (%llu) erase_count %d valid_page %d\n",
               eu->start_pos >> SECTORS_PER_FLASH_PAGE_ORDER,
               eu->start_pos, eu->erase_count, atomic_read(&eu->eu_valid_num));
    }

    list_for_each_entry(eu, &h->free_list, list) {
        if (eu->erase_count == EU_UNUSABLE) {
            continue;
        }
        printk("EU %llu (%llu) erase_count %d valid_page %d\n",
               eu->start_pos >> SECTORS_PER_FLASH_PAGE_ORDER,
               eu->start_pos, eu->erase_count, atomic_read(&eu->eu_valid_num));
    }

    for (i = 0; i < h->total_eus; i++) {
        printk("eu[%d]= %d\n", i,
               pbno_eu_map[h->disk_num * h->total_eus + i]->erase_count);
    }
}

/*
Author: Tifa
    To compare active metadata list with spare area lpn
*/
void check_mdlist_and_sparelpn(lfsm_dev_t * td, struct EUProperty *eu,
                               struct bio *mbio)
{
    struct page *lpn_page[METADATA_SIZE_IN_PAGES + 1] = { NULL };
    sector64 *lpn_list, *meta_list, pbn;
    int32_t i, j, pidx_md, pidx_lpn;

    pbn = eu->start_pos;
    j = 0;

    for (i = 0; i < METADATA_SIZE_IN_PAGES + 1; i++) {
        lpn_page[i] = track_alloc_page(__GFP_ZERO); /*Page in which LPN query will return LPNs */
        LFSM_ASSERT(lpn_page[i]);
        build_lpn_query(td, pbn, lpn_page[i]);  /*LPNs are returned in lpn_page */
        pbn += MAX_LPN_ONE_SHOT * SECTORS_PER_FLASH_PAGE;   // Tifa: 2^nK safe
    }

    if (!IS_ERR_OR_NULL(mbio)) {
        pidx_md = 0;
        pidx_lpn = 0;
        meta_list = (sector64 *) kmap(mbio->bi_io_vec[pidx_md].bv_page);
        lpn_list = (sector64 *) kmap(lpn_page[pidx_lpn]);
        for (i = 0; i < eu->log_frontier >> SECTORS_PER_FLASH_PAGE_ORDER; i++) {
            if ((i >= MAX_LPN_ONE_SHOT) && (i % MAX_LPN_ONE_SHOT) == 0) {
                kunmap(lpn_page[pidx_lpn]);
                pidx_lpn++;
                lpn_list = (sector64 *) kmap(lpn_page[pidx_lpn]);
            }

            if ((i >= METADATA_ENTRIES_PER_PAGE)
                && (i % METADATA_ENTRIES_PER_PAGE) == 0) {
                kunmap(mbio->bi_io_vec[pidx_md].bv_page);
                pidx_md++;
                meta_list = (sector64 *) kmap(mbio->bi_io_vec[pidx_md].bv_page);
            }

            tf_printk("%d {meta,lpn}={%llu,%llu}", i,
                      meta_list[i % METADATA_ENTRIES_PER_PAGE],
                      lpn_list[(i % MAX_LPN_ONE_SHOT) +
                               LPN_QUERY_HEADER / sizeof(sector64)]);
            if (meta_list[i % METADATA_ENTRIES_PER_PAGE] !=
                lpn_list[(i % MAX_LPN_ONE_SHOT) +
                         LPN_QUERY_HEADER / sizeof(sector64)]) {
                tf_printk("... ERROR!!\n");
            } else {
                tf_printk("\n");
            }
        }
        kunmap(lpn_page[pidx_lpn]);
        kunmap(mbio->bi_io_vec[pidx_md].bv_page);
    } else {
        i = 0;
        lpn_list = (sector64 *) kmap(lpn_page[i]);
        for (i = 0; i < eu->log_frontier >> SECTORS_PER_FLASH_PAGE_ORDER; i++) {
            tf_printk("{meta,lpn}={%llu, %llu} {%llx, %llx}",
                      eu->act_metadata.mdnode->lpn[i],
                      lpn_list[(j + LPN_QUERY_HEADER / sizeof(sector64))],
                      eu->act_metadata.mdnode->lpn[i],
                      lpn_list[(j + LPN_QUERY_HEADER / sizeof(sector64))]);
            if (eu->act_metadata.mdnode->lpn[i] !=
                lpn_list[(j + LPN_QUERY_HEADER / sizeof(sector64))]) {
                tf_printk("... ERROR!!\n");
            } else {
                tf_printk("\n");
            }
            j++;

            if (j >= MAX_LPN_ONE_SHOT) {
                kunmap(lpn_page[i]);
                i++;
                lpn_list = (sector64 *) kmap(lpn_page[i]);
                j = 0;
            }
        }
        kunmap(lpn_page[i]);
    }

    for (i = 0; i < METADATA_SIZE_IN_PAGES + 1; i++) {
        track_free_page(lpn_page[i]);
    }
}

static int32_t check_one_eu_state(lfsm_dev_t * td, struct HListGC *h,
                                  struct EUProperty *eu, int32_t * ar_eu_marker,
                                  int32_t state_check,
                                  sftl_err_msg_t * perr_msg)
{
    sftl_err_item_t err_item;
    int32_t cn_total_eu;

    cn_total_eu = h->total_eus;
    if (eu->erase_count == EU_UNUSABLE) {
        return 0;
    }

    memset(&err_item, 0, sizeof(err_item));

    err_item.id_eu = PBNO_TO_EUID(eu->start_pos);
    err_item.state_eu = atomic_read(&eu->state_list);

    //printk(" id_eu %d  , disk_id %d\n",err_item.id_eu, eu->disk_num);

    if (eu->temper >= TEMP_LAYER_NUM || eu->temper < 0) {
        err_item.err_code = -ETEMPER;
        LOGERR("eu_id %d temper %d incorrect \n", err_item.id_eu, eu->temper);
        goto l_end;
    }

    if (atomic_read(&eu->state_list) != state_check) {
        err_item.err_code = -ESTATE;
        LOGERR("eu_id %d state %d incorrect \n", err_item.id_eu,
               atomic_read(&eu->state_list));
        goto l_end;

    }
    //printk(" set idx %d , number %d  \n", err_item.id_eu % cn_total_eu,
    //                ar_eu_marker[err_item.id_eu % cn_total_eu]);

    if ((++ar_eu_marker[err_item.id_eu % cn_total_eu]) >= 2) {
        err_item.err_code = -EEU_DUPLICATE;
        LOGERR("eu %d state %d incorrect \n", err_item.id_eu,
               atomic_read(&eu->state_list));
        goto l_end;
    }

  l_end:
    if (perr_msg && err_item.err_code < 0) {
        memcpy(&perr_msg->ar_item[perr_msg->cn_err++], &err_item,
               sizeof(sftl_err_item_t));
    }

    return err_item.err_code;
}

int32_t check_disk_eu_state(lfsm_dev_t * td, int32_t id_disk,
                            sftl_err_msg_t * perr_msg)
{
    sftl_err_item_t err_item;
    struct HListGC *h;
    struct EUProperty *eu;
    int32_t *ar_eu_marker;
    int32_t i, id_eu, cn_eu_free, cn_eu_hlist;

    cn_eu_free = 0;
    cn_eu_hlist = 0;

    h = gc[id_disk];
    ar_eu_marker = track_vmalloc(h->total_eus * sizeof(int32_t),
                                 GFP_KERNEL | __GFP_ZERO, M_UNKNOWN);
    memset(ar_eu_marker, 0, h->total_eus * sizeof(int32_t));
    memset(&err_item, 0, sizeof(sftl_err_item_t));
    err_item.id_disk = h->disk_num;

    //atomic_t state_list;        /* 2 if in falselist. 1 if in hlist. 0 if in heap */
    // transver free_list

    list_for_each_entry(eu, &h->free_list, list) {
        cn_eu_free++;
        check_one_eu_state(td, h, eu, ar_eu_marker, -1, perr_msg);
    };

    // Hlist
    list_for_each_entry(eu, &h->hlist, list) {
        cn_eu_hlist++;
        check_one_eu_state(td, h, eu, ar_eu_marker, 1, perr_msg);
    };
    // LVP
    for (i = 0; i < h->LVP_Heap_number; i++) {
        check_one_eu_state(td, h, h->LVP_Heap[i], ar_eu_marker, 0, perr_msg);
    }

    //active eu
    for (i = 0; i < h->NumberOfRegions; i++) {
        check_one_eu_state(td, h, h->active_eus[i], ar_eu_marker, -1, perr_msg);
    }

    if (cn_eu_free != h->free_list_number) {
        err_item.err_code = -EFREE_LIST_NUMBER_MISMATCH;
        LOGERR("disk %d , cn_eu_free %d, h->free_list_nuber %lld \n",
               id_disk, cn_eu_free, h->free_list_number);
        goto l_end;
    }

    if (cn_eu_hlist != h->hlist_number) {
        err_item.err_code = -EHLIST_NUMBER_MISMATCH;
        LOGERR("disk %d , cn_eu_hlist %d, h->hlist_nuber %d \n",
               id_disk, cn_eu_hlist, h->hlist_number);
        goto l_end;
    }

    for (i = 0; i < h->total_eus; i++) {
        if (ar_eu_marker[i] == 0) {
            id_eu = (h->total_eus * h->disk_num) + i;
            if (pbno_eu_map[id_eu]->usage != EU_FREE && pbno_eu_map[id_eu]->usage != EU_DATA) { // SYSTEM EU
                continue;
            }

            err_item.err_code = -EEU_LOST;  //
            err_item.id_eu = id_eu;
            LOGERR("disk %d , eu_id%d , erase_count %d \n", h->disk_num, id_eu,
                   pbno_eu_map[id_eu]->erase_count);
            goto l_end;
        }
    }

  l_end:
    if (perr_msg && err_item.err_code < 0) {
        memcpy(&perr_msg->ar_item[perr_msg->cn_err++], &err_item,
               sizeof(sftl_err_item_t));
    }

    track_vfree((void *)ar_eu_marker, (h->total_eus * sizeof(int32_t)),
                M_UNKNOWN);
    printk("return\n");
    return err_item.err_code;

}

static int32_t check_eu_frontier_id_group(lfsm_dev_t * td, int32_t temper,
                                          sftl_err_msg_t * perr_msg)
{
    struct HListGC *h_first, *h, *h_prev;
    int32_t id_group, id_disk, offset_prev, offset_first;

    for (id_group = 0; id_group < td->cn_pgroup; id_group++) {
        h_first = gc[HPEU_EU2HPEU_OFF(0, id_group)];

        for (id_disk = 1; id_disk < td->hpeutab.cn_disk; id_disk++) {
            h = gc[HPEU_EU2HPEU_OFF(id_disk, id_group)];
            h_prev = gc[HPEU_EU2HPEU_OFF(id_disk - 1, id_group)];

            offset_first = h_first->active_eus[temper]->log_frontier -
                h->active_eus[temper]->log_frontier;

            offset_prev = h_prev->active_eus[temper]->log_frontier -
                h->active_eus[temper]->log_frontier;

            if ((offset_first == 0 || offset_first == SECTORS_PER_FLASH_PAGE) &&
                (offset_prev == 0 || offset_prev == SECTORS_PER_FLASH_PAGE)) {
                continue;
            }

            perr_msg->ar_item[perr_msg->cn_err].id_disk = id_disk;
            perr_msg->ar_item[perr_msg->cn_err++].id_group = id_group;
            LOGERR
                ("active EU frontier offset_first:%d, offset_prev:%d, id_disk:%d    \n",
                 offset_first, offset_prev, id_disk);
        }
    }
    return perr_msg->cn_err;
}

static int32_t check_whole_hpage_unwrite(int8_t * buf)
{
    sector64 *pdata;
    int32_t i;

    pdata = (sector64 *) buf;
    for (i = 0; i < HARD_PAGE_SIZE / sizeof(sector64); i++) {
        if (pdata[i] != TAG_UNWRITE_HPAGE) {
            printk(" %llu %d %d\n", pdata[i], TAG_UNWRITE_HPAGE, i);
            return -i;
        }
    }
    return 0;
}

int32_t check_active_eu_frontier(lfsm_dev_t * td, int32_t useless,
                                 sftl_err_msg_t * perr_msg)
{
    int32_t temper;

    for (temper = 0; temper < TEMP_LAYER_NUM; temper++) {
        check_eu_frontier_id_group(td, temper, perr_msg);
    }

    return perr_msg->cn_err;
}

static int32_t check_supermap(lfsm_dev_t * td, sftl_err_msg_t * perr_msg)
{
    super_map_entry_t *rec;
    int8_t *buffer;
    int32_t id_disk;

    if (IS_ERR_OR_NULL(buffer =
                       track_kmalloc(2 * HARD_PAGE_SIZE,
                                     GFP_KERNEL | __GFP_ZERO, M_UNKNOWN))) {
        LOGERR("Cannot alloc memory %d \n", 2 * HARD_PAGE_SIZE);
        return -ENOMEM;
    }

    for (id_disk = 0; id_disk < td->cn_ssds; id_disk++) {
        // read current frontier
        perr_msg->ar_item[perr_msg->cn_err].id_disk = id_disk;

        if (!devio_raw_rw_page
            (grp_ssds.ar_subdev[id_disk].super_frontier - SECTORS_PER_HARD_PAGE,
             grp_ssds.ar_drives[id_disk].bdev, buffer, 2, REQ_OP_READ)) {
            LOGERR("Cannot read disk_id %d supermap sz%d \n", id_disk,
                   2 * HARD_PAGE_SIZE);
            perr_msg->ar_item[perr_msg->cn_err++].err_code =
                -ESUPERMAP_READ_FAIL;
            goto l_end;
        }

        rec = (super_map_entry_t *) buffer;
        if (rec->logic_disk_id != id_disk
            || strncmp(rec->magicword, TAG_SUPERMAP, 8) != 0) {
            LOGERR("supermap content wrong rec->logic_disk_id %d, id_disk %d\n",
                   rec->logic_disk_id, id_disk);
            perr_msg->ar_item[perr_msg->cn_err++].err_code = -ESPM_DISK_FAIL;
            goto l_end;
        }

        if (check_whole_hpage_unwrite(buffer + HARD_PAGE_SIZE) < 0) {
            LOGERR("supermap content wrong rec->logic_disk_id %d, id_disk %d\n",
                   rec->logic_disk_id, id_disk);
            perr_msg->ar_item[perr_msg->cn_err++].err_code = -ESPM_NONEMPTY;
            hexdump(buffer + HARD_PAGE_SIZE, 4096);
            goto l_end;
        }
        LOGINFO("check disk(%d) supermap ok\n", id_disk);
    }

    return 0;

  l_end:
    track_kfree(buffer, 2 * HARD_PAGE_SIZE, M_UNKNOWN);
    return (perr_msg->cn_err);
}

int32_t check_sys_table(lfsm_dev_t * td, int32_t useless,
                        sftl_err_msg_t * perr_msg)
{
    check_supermap(td, perr_msg);   // err item in perr_msg

    ioctl_check_ddmap_updatelog_bmt(td, perr_msg, NULL);    // err item in perr_msg

    return perr_msg->cn_err;
}

int32_t check_freelist_bioc(lfsm_dev_t * td, int32_t id_bioc,
                            sftl_err_msg_t * perr_msg)
{
    struct bio_container *pbioc;

    pbioc = gar_pbioc[id_bioc];
    if (!bioc_list_search(td, &td->datalog_free_list[0], pbioc)) {
        LOGERR("Missing bioc(%p) lpn %llu (0x%llx)\n", pbioc, pbioc->start_lbno,
               pbioc->start_lbno);
        perr_msg->ar_item[perr_msg->cn_err++].id_bioc = id_bioc;
    }

    return perr_msg->cn_err;
}

int32_t check_freelist_bioc_simple(lfsm_dev_t * td, int32_t useless,
                                   sftl_err_msg_t * perr_msg)
{
    struct bio_container *pbioc_cur, *pbioc_next;
    int32_t count;
    // use par_bioext as marker
    //spin_lock_irqsave(&td->datalog_list_spinlock,flag);

    if (IS_ERR_OR_NULL(perr_msg)) {
        return -EFAULT;
    }

    count = 0;
    list_for_each_entry_safe(pbioc_cur, pbioc_next, &td->datalog_free_list[0],
                             list) {
        count++;
        if (!IS_ERR_OR_NULL(pbioc_cur->par_bioext)) {
            LOGERR("bioc(%p) par_biocext is not null , count %d\n", pbioc_cur,
                   count);
            perr_msg->ar_item[perr_msg->cn_err++].pbioc = pbioc_cur;
            goto l_end;
        }
        pbioc_cur->par_bioext = (void *)1;
    }
    //spin_unlock_irqrestore(&td->datalog_list_spinlock,flag);

    LOGINFO("total_bioc_count: count:%d, capacity:%llu  %u\n",
            count, td->freedata_bio_capacity,
            atomic_read(&td->len_datalog_free_list));

    if (count != td->freedata_bio_capacity) {
        LOGINFO("total_bioc_count is wrong: count:%d, capacity:%llu \n",
                count, td->freedata_bio_capacity);
        perr_msg->ar_item[perr_msg->cn_err++].cn_bioc = count;
    }

  l_end:
    //spin_lock_irqsave(&td->datalog_list_spinlock,flag);

    // put par_bioext value back
    list_for_each_entry_safe(pbioc_cur, pbioc_next, &td->datalog_free_list[0],
                             list) {
        pbioc_cur->par_bioext = NULL;
    }
    //spin_unlock_irqrestore(&td->datalog_list_spinlock,flag);

    return perr_msg->cn_err;
}

int32_t check_systable_hpeu(lfsm_dev_t * td, int32_t useless,
                            sftl_err_msg_t * perr_msg)
{
    sftl_err_item_t err_item;
    sector64 log_frontier;
    struct SPssLog *pssl;
    struct EUProperty **eu;
    int8_t *buf;
    int32_t cn_byte;

    pssl = &td->hpeutab.pssl;
    cn_byte = LFSM_CEIL(pssl->sz_alloc, HARD_PAGE_SIZE);
    eu = pssl->eu;
    memset(&err_item, 0, sizeof(err_item));

    if (IS_ERR_OR_NULL(buf = track_kmalloc(cn_byte, GFP_KERNEL, M_OTHER))) {
        LOGERR("cannot alloc memory\n");
        return -ENOMEM;
    }

    if ((log_frontier = eu[0]->log_frontier) != eu[1]->log_frontier) {
        LOGERR("hpeu table EU log-frontier didn't match %d  %d \n",
               eu[0]->log_frontier, eu[1]->log_frontier);
        err_item.id_eu = PBNO_TO_EUID(eu[0]->start_pos);
        err_item.err_code = -ESYS_HPEU_TABLE;
        goto l_end;
    }

    if (PssLog_Get_Comp(pssl, buf, log_frontier, true) < 0) {
        LOGERR("hpeu table EU content didn't match %llu  %llu\n",
               eu[0]->start_pos, eu[1]->start_pos);
        err_item.err_code = -ESYS_HPEU_TABLE;
        goto l_end;
    }

  l_end:
    if (perr_msg && err_item.err_code < 0) {
        memcpy(&perr_msg->ar_item[perr_msg->cn_err++], &err_item,
               sizeof(sftl_err_item_t));
    }

    track_kfree(buf, cn_byte, M_OTHER);
    return err_item.err_code;
}
