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
#include "GC.h"
#include "hpeu_gc.h"
#include "io_manager/io_common.h"
#include "spare_bmt_ul.h"
#include "bmt.h"
#include "stripe.h"
#include "pgroup.h"

static void HPEUgc_validpg_free(HPEUgc_item_t * hpeu_item)
{
    gc_item_t *gc_item;
    int32_t i;

    for (i = 0; i < lfsmdev.hpeutab.cn_disk; i++) {
        gc_item = &hpeu_item->ar_gc_item[i];
        gc_addr_info_free(&gc_item->gc_addr, gc_item->cn_valid_pg);
    }
}

static bool HPEUgc_validpg_alloc(HPEUgc_item_t * hpeu_item)
{
    gc_item_t *gc_item;
    int32_t i;

    for (i = 0; i < lfsmdev.hpeutab.cn_disk; i++) {
        gc_item = &hpeu_item->ar_gc_item[i];
        if (atomic_read(&gc_item->eu->eu_valid_num) == 0) {
            continue;
        }

        if (!gc_alloc_mapping_valid(gc_item)) {
            goto l_fail;
        }
    }
    return true;

  l_fail:
    HPEUgc_validpg_free(hpeu_item);
    return false;
}

static void HPEUgc_waitup_queue(lfsm_dev_t * td)
{
    int32_t i;

    for (i = 0; i < td->cn_ssds; i++) {
        if (gc[i]->free_list_number > 0) {
            wake_up_interruptible(&td->gc_conflict_queue);
        }
    }
}

static bool HPEUgc_bioc_skip(gc_item_t * gc_item, lfsm_dev_t * td,
                             int32_t cn_valid)
{
    gcadd_info_t *gc_addr;
    int32_t i, pg_idx, pg_addr;

    for (i = 0; i < cn_valid; i++) {
        gc_addr = &gc_item->gc_addr;
        pg_idx =
            ((gc_addr->old_pbn[i] -
              gc_item->eu->start_pos) >> SECTORS_PER_FLASH_PAGE_ORDER);
        pg_addr = (gc_addr->old_pbn[i] >> SECTORS_PER_FLASH_PAGE_ORDER);

        if (test_bit(pg_addr % PAGES_PER_BYTE,
                     (void *)&gc_item->eu->
                     bit_map[pg_idx >> PAGES_PER_BYTE_ORDER])) {
            continue;
        }
        // the lpn have been invalid
        LOGINFO("This addr %llu lpn %llu is invalid \n", gc_addr->old_pbn[i],
                gc_addr->lbn[i]);
        post_rw_check_waitlist(td, gc_item->ar_pbioc[i], 0);
        put_bio_container(td, gc_item->ar_pbioc[i]);
        gc_item->ar_pbioc[i] = NULL;
    }

    return true;
}

static bool HPEUgc_bioc_get(HPEUgc_item_t * hpeu_item, lfsm_dev_t * td,
                            int32_t * ar_valid)
{
    gc_item_t *gc_item;
    int32_t i;

    for (i = 0; i < td->hpeutab.cn_disk; i++) {
        gc_item = &hpeu_item->ar_gc_item[i];
        if (NULL == (gc_item->ar_pbioc)) {
            continue;
        }
        if (!gc_get_bioc_init(td, gc_item, ar_valid[i])) {
            return false;
        }
    }

    for (i = 0; i < td->hpeutab.cn_disk; i++) {
        gc_item = &hpeu_item->ar_gc_item[i];
        HPEUgc_bioc_skip(gc_item, td, ar_valid[i]);
    }
    return true;
}

static void HPEUgc_arpbioc_free(HPEUTab_t * phpeutab, HPEUgc_item_t * hpeu_item,
                                int32_t * ar_valid)
{
    gc_item_t *gc_item;
    int32_t i;

    for (i = 0; i < phpeutab->cn_disk; i++) {
        gc_item = &hpeu_item->ar_gc_item[i];
        if (gc_item->ar_pbioc) {
            track_kfree(gc_item->ar_pbioc,
                        ar_valid[i] * sizeof(struct bio_container *), M_GC);
        }
    }
}

static bool HPEUgc_arpbioc_alloc(HPEUTab_t * phpeutab,
                                 HPEUgc_item_t * hpeu_item, int32_t * ar_valid)
{
    gc_item_t *gc_item;
    int32_t i;

    for (i = 0; i < phpeutab->cn_disk; i++) {
        gc_item = &hpeu_item->ar_gc_item[i];
        if (gc_item->cn_valid_pg == 0) {
            continue;
        }

        if (NULL == (gc_item->ar_pbioc = (struct bio_container **)
                     track_kmalloc(ar_valid[i] * sizeof(struct bio_container *),
                                   GFP_KERNEL, M_GC))) {
            goto l_fail;
        }

        memset(&gc_item->ar_pbioc[0], 0,
               ar_valid[i] * sizeof(struct bio_container *));
    }

    return true;

  l_fail:
    HPEUgc_arpbioc_free(phpeutab, hpeu_item, ar_valid);

    return false;
}

static bool HPEUgc_process_lpninfo(HPEUgc_item_t * hpeu_item, lfsm_dev_t * td,
                                   int32_t * ar_valid)
{
    gc_item_t *gc_item;
    int32_t i;

    for (i = 0; i < td->hpeutab.cn_disk; i++) {
        gc_item = &hpeu_item->ar_gc_item[i];
        if (gc_item->cn_valid_pg == 0) {
            continue;
        }

        if (!gc_process_lpninfo(td, gc_item, &ar_valid[i])) {
            return false;
        }
    }
    return true;
}

static int32_t HPEU_sizeof_GCentry(HPEUTab_t * hpeutab)
{
    return (sizeof(HPEUgc_entry_t) + (sizeof(gc_item_t) * hpeutab->cn_disk));
}

bool HPEUgc_entry_alloc_insert(struct list_head *list_to_gc,
                               struct EUProperty *pick_eu)
{
    HPEUgc_entry_t *entry;
    int32_t i;

    if (NULL ==
        (entry =
         track_kmalloc(HPEU_sizeof_GCentry(&lfsmdev.hpeutab), GFP_KERNEL,
                       M_GC))) {
        return false;
    }

    for (i = 0; i < lfsmdev.hpeutab.cn_disk; i++) {
        gc_item_init(&entry->hpeu_gc_item.ar_gc_item[i], NULL);
    }

    entry->hpeu_gc_item.id_hpeu = pick_eu->id_hpeu;
    entry->hpeu_gc_item.id_grp = PGROUP_ID(pick_eu->disk_num);
    list_add_tail(&entry->link, list_to_gc);
    return true;
}

static void HPEUgc_entry_free_delete(HPEUgc_entry_t * entry)
{
    if (atomic_dec_return
        (&lfsmdev.ar_pgroup[entry->hpeu_gc_item.id_grp].cn_gcing) == 0) {
        wake_up_interruptible(&lfsmdev.worker_queue);
    }
    list_del_init(&entry->link);
    track_kfree(entry, HPEU_sizeof_GCentry(&lfsmdev.hpeutab), M_GC);
}

static int32_t HPEUgc_submit_copyios(gc_item_t * gc_item, lfsm_dev_t * td,
                                     int32_t cn_valid)
{
    bio_end_io_t *ar_bi_end_io[MAX_ID_ENDBIO];
    struct bio_container *bioc;
    gcadd_info_t *gc_addr;
    int32_t ret, i, cn_submit;

    ar_bi_end_io[ID_ENDBIO_BRM] = sflush_end_bio_gcwrite_read_brm;
    ar_bi_end_io[ID_ENDBIO_NORMAL] = sflush_end_bio_gcwrite_read;
    cn_submit = 0;

    for (i = 0; i < cn_valid; i++) {
        bioc = gc_item->ar_pbioc[i];
        if (NULL == bioc) {
            continue;
        }

        gc_addr = &gc_item->gc_addr;

        bioc->start_lbno = gc_addr->lbn[i];
        bioc->source_pbn = gc_addr->old_pbn[i];
        bioc->bio->bi_iter.bi_size = FLASH_PAGE_SIZE;
        bioc->bio->bi_vcnt = MEM_PER_FLASH_PAGE;

        atomic_inc(&gc_item->eu->cn_pinned_pages);

        if ((ret =
             read_flash_page_vec(td, gc_addr->old_pbn[i],
                                 bioc->bio->bi_io_vec, 0, bioc, true,
                                 ar_bi_end_io)) < 0) {
            atomic_dec(&gc_item->eu->cn_pinned_pages);
            LOGERR("ZZZ: HPEUgc copyread fail eu %llu lbn %llu src_pbn %llu "
                   "rest_pin %d\n", gc_item->eu->start_pos, bioc->start_lbno,
                   bioc->source_pbn, cn_valid - i);
            return ret;
        }

        cn_submit++;
        atomic_inc(&td->bioboxpool.cn_total_copied);
    }

    return cn_submit;
}

static void _HPEUgc_smart_copy_wait(struct EUProperty *eu)
{
    int32_t cnt, wait_ret;

    cnt = 0;
    while (true) {
        cnt++;
        wait_ret = wait_event_interruptible_timeout(gc[eu->disk_num]->io_queue,
                                                    atomic_read(&eu->
                                                                cn_pinned_pages)
                                                    == 0,
                                                    LFSM_WAIT_EVEN_TIMEOUT);
        if (wait_ret <= 0) {
            LOGWARN("HPEUgc IO no response after seconds %d\n",
                    LFSM_WAIT_EVEN_TIMEOUT_SEC * cnt);
        } else {
            break;
        }
    }
}

static bool HPEUgc_smart_copy(HPEUgc_item_t * hpeu_item, lfsm_dev_t * td,
                              int32_t * ar_valid)
{
    gc_item_t *gc_item;
    struct EUProperty *eu;
    int32_t i, j, cn_submit;
    bool ret;

    ret = true;

    for (i = 0; i < td->hpeutab.cn_disk; i++) {
        if (ar_valid[i] == 0) {
            continue;
        }

        gc_item = &hpeu_item->ar_gc_item[i];
        if ((cn_submit = HPEUgc_submit_copyios(gc_item, td, ar_valid[i])) < 0) {
            LOGWARN("HPEU_GC didn't send all %d\n", i);
            ret = false;        // abort this hpeu_gc
            break;
        }
    }
    //printk("%s hpeu_item %d id_disk %d read submit %d finish\n",__FUNCTION__,hpeu_item->id_hpeu, hpeu_item->ar_gc_item[0].eu->disk_num,cn_submit);

    for (i = 0; i < td->hpeutab.cn_disk; i++) {
        if (ar_valid[i] == 0) {
            continue;
        }

        gc_item = &hpeu_item->ar_gc_item[i];
        eu = gc_item->eu;
        //printk("1.id_hpeu %d eu addr %llu pin %d temper %d cn_free %llu\n",eu->id_hpeu,eu->start_pos, atomic_read(&eu->cn_pinned_pages), eu->temper,gc[eu->disk_num]->free_list_number);
        //lfsm_wait_event_interruptible(gc[eu->disk_num]->io_queue,
        //        atomic_read(&eu->cn_pinned_pages) == 0);
        _HPEUgc_smart_copy_wait(eu);
//        while(wait_event_interruptible_timeout(gc[eu->disk_num]->io_queue, atomic_read(&eu->cn_pinned_pages)==0, 30000)==0)
//            printk("GC hang on %s:%d for %d secs eu %llu cn_pin %d\n",__FILE__,__LINE__,30000/1000,eu->start_pos, atomic_read(&eu->cn_pinned_pages));
        //printk("2.id_hpeu %d eu addr %llu pin %d temper %d cn_free %llu\n",eu->id_hpeu, eu->start_pos, atomic_read(&eu->cn_pinned_pages), eu->temper, gc[eu->disk_num]->free_list_number);

        for (j = 0; j < ar_valid[i]; j++) {
            if (NULL == (gc_item->ar_pbioc[j])) {
                continue;
            }
            //io_queue_fin for read fail, status for write fail
            if ((gc_item->ar_pbioc[j]->io_queue_fin != 0) ||
                (gc_item->ar_pbioc[j]->status != BLK_STS_OK)) {
                LOGWARN("bioc(%d) lbn %llu old_pbn %llu dest_pbn %llu err %d, "
                        "status %d \n", j, gc_item->ar_pbioc[j]->start_lbno,
                        gc_item->ar_pbioc[j]->source_pbn,
                        gc_item->ar_pbioc[j]->dest_pbn,
                        gc_item->ar_pbioc[j]->io_queue_fin,
                        gc_item->ar_pbioc[j]->status);
                /// since we move update_ppq_bitmap after IO success, we don't reset here, TF,2013/08/27
                //if (gc_item->ar_pbioc[j]->dest_pbn!=-1)
                //    reset_bitmap_ppq(gc_item->ar_pbioc[j]->dest_pbn, gc_item->ar_pbioc[j]->source_pbn, gc_item->gc_addr.lbn[j], td);
                ret = false;
            }
            post_rw_check_waitlist(td, gc_item->ar_pbioc[j], 0);
            put_bio_container(td, gc_item->ar_pbioc[j]);
        }
    }
    //LOGINFO("finish gc of hpeu %d, ret = %s \n",hpeu_item->id_hpeu, ret?"true":"false");
    return ret;
/*
l_fail:
    HPEUgc_arpbioc_free(hpeu_item, ar_valid);
    HPEUgc_validpg_free(hpeu_item);
    return false;
*/
}

static void HPEUgc_insert_list_or_drop(HPEUgc_item_t * hpeu_item,
                                       lfsm_dev_t * td)
{
    gc_item_t *gc_item;
    int32_t i;

    //tifa: the updatelog is no need h whole lock, because change ULeu (freelist_delete) don't happend inside the function 2012.08.08
    //Tifa : 2012.10.19 according to version 1178 updatelog outside is locked by eu->disk_num whole lock, and hook function
    //       in rmutex will check lock state. so that need call rmutex_lock.
    //Tifa:2014.01.22 move after gc_eu_reinsert_or_drop, because freelist_insert will clean EU's id_hpeu as -1, so need to log first
    LFSM_RMUTEX_LOCK(&gc[hpeu_item->ar_gc_item[0].eu->disk_num]->whole_lock);
    UpdateLog_logging(td, hpeu_item->ar_gc_item[0].eu, -1, UL_HPEU_ERASE_MARK);
    LFSM_RMUTEX_UNLOCK(&gc[hpeu_item->ar_gc_item[0].eu->disk_num]->whole_lock);

    for (i = 0; i < td->hpeutab.cn_disk; i++) {
        gc_item = &hpeu_item->ar_gc_item[i];
        metalog_packet_register(&td->mlogger, NULL, gc_item->eu->start_pos,
                                MLOG_TAG_EU_ERASE);
        gc_eu_reinsert_or_drop(td, gc_item);
    }
    HPEU_free(&td->hpeutab, hpeu_item->id_hpeu);
}

static void HPEUgc_remove_list(HPEUTab_t * phpeutab, HPEUgc_item_t * hpeu_item)
{
    gc_item_t *gc_item;
    struct HListGC *h;
    int32_t i;

    for (i = 0; i < phpeutab->cn_disk; i++) {
        gc_item = &hpeu_item->ar_gc_item[i];
        h = gc[gc_item->eu->disk_num];
        if ((gc_item->id_srclist = gc_remove_eu_from_lists(h, gc_item->eu)) < 0) {
            LFSM_ASSERT(0);
        }
    }
}

static bool HPEUgc_copy_process(HPEUgc_item_t * hpeu_item, lfsm_dev_t * td,
                                int32_t * ar_valid)
{
    if (!HPEUgc_smart_copy(hpeu_item, td, ar_valid)) {
        return false;
    }

    HPEUgc_remove_list(&td->hpeutab, hpeu_item);    //HPEUgc: list delete
    HPEUgc_insert_list_or_drop(hpeu_item, td);  //HPEUgc: insert to freelist

    return true;
}

static int32_t HPEUgc_get_total_valid(HPEUgc_item_t * hpeu_item,
                                      int32_t cn_ssds_per_hpeu)
{
    struct EUProperty *eu;
    int32_t i, total_valid;

    total_valid = 0;
    for (i = 0; i < cn_ssds_per_hpeu; i++) {
        eu = hpeu_item->ar_gc_item[i].eu;
        total_valid += atomic_read(&eu->eu_valid_num);
    }
    return total_valid;
}

bool HPEUgc_selectable(struct HPEUTab *phpeutab, int32_t id_hpeu)
{
    struct EUProperty *ar_peu[(1 << RDDCY_ECC_MARKER_BIT_NUM)]; //ding: malloc may encounter performance issue, so use static
    int32_t i;

    if (HPEU_get_eus(phpeutab, id_hpeu, ar_peu) != phpeutab->cn_disk) {
        LOGINFO("skip id_hpeu %d due to incompleteness\n", id_hpeu);
        return false;
    }

    for (i = 0; i < phpeutab->cn_disk; i++) {
        if (eu_is_active(ar_peu[i]) >= 0) {
            return false;
        }
    }

    return true;
}

static bool HPEUgc_eu_init(HPEUgc_item_t * hpeu_item, lfsm_dev_t * td)
{
    struct HPEUTab *hpeutb;
    struct EUProperty **ar_peu;
    int32_t i, ret;
    bool ret2;

    hpeutb = &td->hpeutab;
    ret2 = true;
    if (NULL == (ar_peu = (struct EUProperty **)
                 track_kmalloc(td->hpeutab.cn_disk *
                               sizeof(struct EUProperty *), GFP_KERNEL,
                               M_GC))) {
        return false;
    }

    memset(ar_peu, 0, td->hpeutab.cn_disk * sizeof(struct EUProperty *));
    //printk("hpeu_gc_item %d\n",hpeu_item->id_hpeu);

    if ((ret =
         HPEU_get_eus(hpeutb, hpeu_item->id_hpeu,
                      ar_peu)) != td->hpeutab.cn_disk) {
        LOGERR("id_hpeu %d incompleteness (%d)\n", hpeu_item->id_hpeu, ret);
        ret2 = false;
        goto l_end;
    }

    for (i = 0; i < td->hpeutab.cn_disk; i++) {
        if (eu_is_active(ar_peu[i]) >= 0) { //isActive
            ret2 = false;
            goto l_end;
        }
        hpeu_item->ar_gc_item[i].eu = ar_peu[i];

        //printk("%s gc_item[%d]= eu(%p) addr %llu valid %d\n",__FUNCTION__,i,ar_eu[i],ar_eu[i]->start_pos, atomic_read(&ar_eu[i]->eu_valid_num));
    }

  l_end:
    track_kfree(ar_peu, td->hpeutab.cn_disk * sizeof(struct EUProperty *),
                M_GC);

    return ret2;
}

static bool HPEUgc_is_valid_overthres(int32_t total_valid,
                                      int32_t maxcn_data_disk, int32_t order)
{
    if ((total_valid) > (((DATA_FPAGE_IN_SEU * maxcn_data_disk) >> order))) {
        return true;
    } else {
        return false;
    }
}

static bool HPEUgc_is_all_valid(int32_t total_valid, int32_t maxcn_data_disk)
{
    if ((DATA_FPAGE_IN_SEU * maxcn_data_disk) == total_valid) {
        return true;
    } else {
        return false;
    }
}

//return selected cn_eu for gc
static int32_t HPEUgc_select_gcEU(struct list_head *pbeGC_list, lfsm_dev_t * td)
{
    struct list_head beGC_badlist;
    HPEUgc_entry_t *cur_entry, *next_entry;
    HPEUgc_item_t *phpeu_gc_item;
    int32_t cn_pick, cn_pick_bad, total_valid, maxcn_data_disk;
    //int32_t cn_unwritten;

    cn_pick = 0;
    cn_pick_bad = 0;
    total_valid = 0;

    INIT_LIST_HEAD(&beGC_badlist);

    maxcn_data_disk = hpeu_get_maxcn_data_disk(td->hpeutab.cn_disk);
    list_for_each_entry_safe(cur_entry, next_entry, pbeGC_list, link) {
        phpeu_gc_item = &cur_entry->hpeu_gc_item;
        //remove active and non-init eu
        if (!HPEUgc_eu_init(phpeu_gc_item, td)) {
            HPEUgc_entry_free_delete(cur_entry);
            continue;
        }
        // remove unfinish write hpeu
        // only pick the unwritten count ==0
#if 0                           //Ingore unwritten check because after rebuilding, we will have unwritten hpeu
        if ((cn_unwritten =
             HPEU_is_unfinish(&td->hpeutab, phpeu_gc_item->id_hpeu)) != 0) {
            LOGWARN("WARNING: unwritten count is not zero, id_hpeu %d "
                    "(unwritten %d pg)\n", cur_entry->hpeu_gc_item.id_hpeu,
                    cn_unwritten);
            HPEUgc_entry_free_delete(cur_entry);
            continue;
        }
#endif
        //remove all valid eu
        total_valid = (HPEUgc_get_total_valid(&cur_entry->hpeu_gc_item,
                                              td->hpeutab.
                                              cn_disk) >>
                       SECTORS_PER_FLASH_PAGE_ORDER);
        if (HPEUgc_is_all_valid(total_valid, maxcn_data_disk)) {
            LOGWARN("Is all valid id_hpeu %d (toatl_valid %d pg)\n",
                    cur_entry->hpeu_gc_item.id_hpeu, total_valid);
            HPEUgc_entry_free_delete(cur_entry);
            continue;
        }

        if (HPEUgc_is_valid_overthres(total_valid, maxcn_data_disk, 1)) {
            //printk("WARNING: Too many valid num in id_hpeu %d (toatl_valid %d pg)\n",cur_entry->hpeu_gc_item.id_hpeu, total_valid);
            list_del_init(&cur_entry->link);
            list_add_tail(&cur_entry->link, &beGC_badlist);
            cn_pick_bad++;
            continue;
        }
        cn_pick++;
        //printk("Good valid num in id_hpeu %d (toatl_valid %d pg)\n",cur_entry->hpeu_gc_item.id_hpeu, total_valid);
    }
    //printk("select pick %d bad_pick %d\n",cn_pick,cn_pick_bad);
    if (cn_pick == 0) {
        list_replace(&beGC_badlist, pbeGC_list);
        cn_pick = cn_pick_bad;
    } else {
        list_for_each_entry_safe(cur_entry, next_entry, &beGC_badlist, link) {
            HPEUgc_entry_free_delete(cur_entry);
        }
    }

    return cn_pick;
}

// true is allempty, false is at least one nonempty
static bool sublist_is_allempty(struct list_head *pbeGC_list,
                                int32_t cn_sublist)
{
    int32_t i, cn_emptylist;

    cn_emptylist = 0;

    for (i = 0; i < cn_sublist; i++) {
        if (list_empty(&pbeGC_list[i])) {
            cn_emptylist++;
        }
    }
    if (cn_emptylist == cn_sublist) {
        return true;
    }
    return false;
}

static bool sublist_mix(struct list_head *pbeGC_list, int32_t cn_sublist,
                        struct list_head *ret_pbeGC_list)
{
    HPEUgc_entry_t *hpeugc_entry;
    int32_t i;

    while (!sublist_is_allempty(pbeGC_list, cn_sublist)) {
        for (i = 0; i < cn_sublist; i++) {
            if (list_empty(&pbeGC_list[i])) {
                continue;
            }

            if (NULL == (hpeugc_entry = list_entry(pbeGC_list[i].next,
                                                   typeof(HPEUgc_entry_t),
                                                   link))) {
                LOGERR("pgroup %d head 0x%p is_empty %d\n", i, &pbeGC_list[i],
                       list_empty(&pbeGC_list[i]));
                return false;
            }
            list_move(&hpeugc_entry->link, ret_pbeGC_list);
        }
    }

    return true;
}

// return cn total picked EU list int32_t ret_pbeGC_list with pgroup mixing
static int32_t HPEUgc_pgroup_selectEU(lfsm_dev_t * td, int32_t * ar_gc_flag,
                                      struct list_head *ret_pbeGC_list)
{
    struct list_head *ar_list_tobe_GC;  //[LFSM_PGROUP_NUM];
    struct HListGC *h;
    HPEUgc_entry_t *cur_entry, *next_entry;
    int32_t i, cn_total_pick, cn_pick;

    cn_total_pick = 0;
    cn_pick = 0;

    if (NULL == (ar_list_tobe_GC = (struct list_head *)
                 track_kmalloc(td->cn_pgroup * sizeof(struct list_head),
                               GFP_KERNEL, M_GC))) {
        return 0;
    }

    for (i = 0; i < td->cn_pgroup; i++) {
        INIT_LIST_HEAD(&ar_list_tobe_GC[i]);
    }

    for (i = 0; i < td->cn_pgroup; i++) {
        atomic_set(&td->ar_pgroup[i].cn_gcing, 0);
        //if (ar_gc_flag[i]==GC_OFF)
        //    continue;
        if (td->ar_pgroup[i].state != epg_good) {
            continue;
        }

        if ((ar_gc_flag[i] =
             gc_threshold(gc[td->ar_pgroup[i].min_id_disk],
                          ar_gc_flag[i])) == GC_OFF) {
            continue;
        }

        h = gc[HPEU_EU2HPEU_OFF(0, i)];
        if (gc_pickEU_insert_GClist(h, &ar_list_tobe_GC[i], ar_gc_flag[i]) == 0) {
            //HPEUgc: pick EU from heap, hlist and falselist
            continue;
        }

        if ((cn_pick = HPEUgc_select_gcEU(&ar_list_tobe_GC[i], td)) == 0) {
            goto l_free_allentry;
        }
        cn_total_pick += cn_pick;
        atomic_set(&td->ar_pgroup[i].cn_gcing, cn_pick);

/*
        LOGINFO("pgroup %d gc_flag %d cn_pick %d; cn_free",i, ar_gc_flag[i], cn_pick);
        for(j=0; j<DISK_PER_HPEU; j++)
            printk("(%d)%lld ",HPEU_EU2HPEU_OFF(j,i),gc[HPEU_EU2HPEU_OFF(j,i)]->free_list_number);
        printk("\n");
*/
    }

    if (!sublist_mix(ar_list_tobe_GC, td->cn_pgroup, ret_pbeGC_list)) {
        LOGERR("sublist_mix\n");
        goto l_free_allentry;
    }
    track_kfree(ar_list_tobe_GC, td->cn_pgroup * sizeof(struct list_head),
                M_GC);
    return cn_total_pick;

  l_free_allentry:
    for (i = 0; i < td->cn_pgroup; i++) {
        list_for_each_entry_safe(cur_entry, next_entry, &ar_list_tobe_GC[i],
                                 link) {
            HPEUgc_entry_free_delete(cur_entry);
        }
    }

    list_for_each_entry_safe(cur_entry, next_entry, ret_pbeGC_list, link) {
        HPEUgc_entry_free_delete(cur_entry);
    }

    track_kfree(ar_list_tobe_GC, td->cn_pgroup * sizeof(struct list_head),
                M_GC);
    return 0;
}

// return valid
int32_t HPEUgc_process(lfsm_dev_t * td, int32_t * ar_gc_flag)
{
    struct list_head beGC_list;
    HPEUgc_entry_t *cur_entry, *next_entry;
    HPEUgc_item_t *phpeu_gc_item;
    int32_t *ar_valid;          //[DISK_PER_HPEU]={0};
    int32_t i, cn_total_valid;
    bool ret, isfirst_run;

    cn_total_valid = 0;
    ret = true;
    isfirst_run = true;

    if (lfsm_readonly == 1) {
        return 0;
    }

    INIT_LIST_HEAD(&beGC_list);

    if (HPEUgc_pgroup_selectEU(td, ar_gc_flag, &beGC_list) == 0) {
        return cn_total_valid;
    }

    ar_valid =
        (int32_t *) track_kmalloc(td->hpeutab.cn_disk * sizeof(int32_t),
                                  GFP_KERNEL, M_GC);
    memset(ar_valid, 0, sizeof(int32_t) * td->hpeutab.cn_disk);

    list_for_each_entry_safe(cur_entry, next_entry, &beGC_list, link) {
        phpeu_gc_item = &cur_entry->hpeu_gc_item;
        if (!ret) {
            goto l_free_entry;
        }

        if (td->ar_pgroup[phpeu_gc_item->id_grp].state != epg_good) {
            LOGWARN("not allow GC fail disk eu %llu\n",
                    phpeu_gc_item->ar_gc_item[0].eu->start_pos);
            goto l_free_entry;
        }

        if (!(ret = HPEUgc_validpg_alloc(phpeu_gc_item))) {
            //HPEUgc: check valid and alloc valid pg
            goto l_free_entry;
        }

        for (i = 0; i < td->hpeutab.cn_disk; i++) {
            ar_valid[i] = phpeu_gc_item->ar_gc_item[i].cn_valid_pg;
            cn_total_valid += ar_valid[i];
        }

        if (!(ret = HPEUgc_process_lpninfo(phpeu_gc_item, td, ar_valid))) {
            //HPEUgc: get lpn info
            goto l_free_valid;
        }

        if (!
            (ret =
             HPEUgc_arpbioc_alloc(&td->hpeutab, phpeu_gc_item, ar_valid))) {
            //HPEUgc: alloc bioc
            goto l_free_valid;
        }

        if (!(ret = HPEUgc_bioc_get(phpeu_gc_item, td, ar_valid))) {
            //HPEUgc: get bioc (including check conflict)
            goto l_free_bioc;
        }

        if (isfirst_run) {
            LFSM_RMUTEX_LOCK(&gc[0]->whole_lock);
            UpdateLog_logging(td, pbno_eu_map[0], 0, UL_GC_MARK);   //HPEUgc: updatelog GC logging
            LFSM_RMUTEX_UNLOCK(&gc[0]->whole_lock);
            isfirst_run = false;
        }

        if (!(ret = HPEUgc_copy_process(phpeu_gc_item, td, ar_valid))) {
            /*
               next_entry=cur_entry; // redo
               HPEUgc_arpbioc_free(phpeu_gc_item, ar_valid);
               HPEUgc_validpg_free(phpeu_gc_item);
               continue;
             */
            LOGERR("Can't gc id_hpeu %d because write fail\n",
                   phpeu_gc_item->id_hpeu);
        }

      l_free_bioc:
        HPEUgc_arpbioc_free(&td->hpeutab, phpeu_gc_item, ar_valid);
      l_free_valid:
        HPEUgc_validpg_free(phpeu_gc_item);
      l_free_entry:
        HPEUgc_entry_free_delete(cur_entry);

        if (ret) {
            HPEUgc_waitup_queue(td);
        }
    }
    track_kfree(ar_valid, td->hpeutab.cn_disk * sizeof(int32_t), M_GC);

    return cn_total_valid;
}

int32_t HPEUgc_dec_pin_and_wakeup(sector64 source_pbn)
{
    struct EUProperty *eu;

    eu = eu_find(source_pbn);
    if (unlikely(NULL == eu)) {
        LOGERR("%llu give bad eu\n", source_pbn);
        return -EINVAL;
    }
    if (atomic_dec_and_test(&eu->cn_pinned_pages)) {
        wake_up_interruptible(&gc[eu->disk_num]->io_queue);
    }

    return 0;
}
