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
** This file constitues of all the functions used during the initialization and destruction
** of struct EUProperty of all the EUs
*/
#include <linux/types.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/bio.h>
#include "config/lfsm_setting.h"
#include "config/lfsm_feature_setting.h"
#include "config/lfsm_cfg.h"
#include "common/common.h"
#include "common/rmutex.h"
#include "common/mem_manager.h"
#include "io_manager/mqbiobox.h"
#include "io_manager/biocbh.h"
#include "nextprepare.h"
#include "diskgrp.h"
#include "system_metadata/Dedicated_map.h"
#include "lfsm_core.h"
#include "bmt.h"
#include "io_manager/io_common.h"
#include "EU_access.h"
#include "GC.h"
#include "io_manager/io_write.h"
#include "system_metadata/Dedicated_map.h"
#include "system_metadata/Super_map.h"
#include "special_cmd.h"
#include "spare_bmt_ul.h"
#include "EU_create.h"
#include "erase_count.h"
#include "bmt_ppdm.h"
#include "metabioc.h"
#include "mdnode.h"
#include "metalog.h"

/*
** This is a future work function which tries to trigger the garbage collection of FTL
** The rationale behind is that if we over-write the first sector of the eu, FTL triggers gc of the eu
**
** @e : The eu upon which the dummy write is to be performed
**
** Returns void
*/
void dummy_write(struct EUProperty *e)
{
    e->log_frontier = DUMMY_SIZE_IN_SECTOR;
    atomic_set(&e->eu_valid_num, DUMMY_SIZE_IN_SECTOR);
}

/** Using helper bio from BMT, erase the targt EU
*** return int:// -EIO, -ENODEV,0, -ENOEXEC 
**/
int32_t EU_erase(struct EUProperty *victim_EU)
{
    int32_t ret_erase_cmd;

    if (lfsm_readonly == 1) {
        return 0;
    }
    // below func return value -EIO, -ENODEV,0
    if ((ret_erase_cmd =
         build_erase_command(victim_EU->start_pos,
                             NUM_EUS_PER_SUPER_EU)) == 0) {
        goto l_normal_end;
    } else if (ret_erase_cmd < 0) {
        //Erase Failure, mark as bad
        LOGERR("Erase Failure for %llu dropping\n ", victim_EU->start_pos);
        drop_EU(((victim_EU->start_pos -
                  eu_log_start) >> (SECTORS_PER_SEU_ORDER)));
        return -EIO;
    } else {
        return -ENOEXEC;
    }

  l_normal_end:
    ECT_erase_count_inc(victim_EU);
    diskgrp_cleanlog(&grp_ssds, victim_EU->disk_num);
    return 0;
}

// foregound use only
int32_t EU_erase_if_undone(struct HListGC *h, struct EUProperty *eu_ret)
{
    int32_t ret;

    if (atomic_read(&eu_ret->erase_state) == DIRTY) {
        return -EINVAL;
    }

    ret = 0;
    if (atomic_read(&eu_ret->erase_state) < ERASE_SUBMIT &&
        atomic_read(&eu_ret->erase_state) > DIRTY) {
        ret = EU_erase(eu_ret);
        if (ret == 0) {
            atomic_set(&eu_ret->erase_state, ERASE_DONE);
            atomic_inc(&h->cn_clean_eu);
        } else {
            atomic_set(&eu_ret->erase_state, ERASE_SUBMIT_FAIL);
            return ret;
        }

        //printk("state is %d \n",atomic_read(&eu_ret->erase_state));
        cn_erase_front++;
    } else {
        lfsm_wait_event_interruptible(lfsmdev.ersmgr.wq_item,
                                      atomic_read(&eu_ret->erase_state) !=
                                      ERASE_SUBMIT);

        if (atomic_read(&eu_ret->erase_state) == ERASE_SUBMIT_FAIL) {

#if LFSM_ENABLE_TRIM==1
            return -ENODEV;
#endif
            if (eu_ret->erase_count == EU_UNUSABLE) {
                goto l_end;
            }

            ret = ersmgr_result_handler(&lfsmdev, -EIO, eu_ret);
            if (ret < 0 && ret != -EIO) {
                return -ENODEV;
            } else if (ret == 0) {
                atomic_inc(&h->cn_clean_eu);
            }
        }
    }

  l_end:
    if ((ret == 0) && (eu_ret->erase_count == EU_UNUSABLE)) {   //judge no matter foreground or background
        LOGWARN("EU %llus (0x%p) is erased normally but marked EU_UNUSABLE\n",
                eu_ret->start_pos, eu_ret);
        return -EIO;
    }
    return ret;
    //printk("state is %d \n",atomic_read(&eu_ret->erase_state));
}

/*
** To flush in-memory (PBA->LBA) metadata information of the active eus to the disk during the driver unload
** This information in necessary during GC
**
** @td : lfsm_dev_t object
** @active_eu : The active eu for which metadata flush has to happen
**
** Returns void
**
*/
int32_t flush_metadata_of_active_eu(lfsm_dev_t * td, struct HListGC *h,
                                    struct EUProperty *active_eu)
{
    struct bio_container *pbioc;
    sector64 pbno;

    pbno = active_eu->start_pos + active_eu->eu_size -
        (METADATA_SIZE_IN_SECTOR - DUMMY_SIZE_IN_SECTOR);
    if (NULL == (active_eu->act_metadata.mdnode)) {
        LOGWARN("active metadata %llu is empty\n", active_eu->start_pos);
        return 0;
    }

    if (active_eu->log_frontier == 0) {
        mdnode_free(&active_eu->act_metadata, td->pMdpool);
        return 0;
    }

    if (NULL == (pbioc = metabioc_build(td, h, pbno))) {
        return -EIO;
    }

    if (lfsmCfg.testCfg.crashSimu)
        mdnode_free(&active_eu->act_metadata, td->pMdpool);
    else
        metabioc_save_and_free(td, pbioc, true);

    return 0;
}

// log_frontier<0 means don't change the value of peu->log_frontier

int32_t EU_param_init(struct EUProperty *peu, int32_t eu_usage,
                      int32_t log_frontier)
{
    peu->id_hpeu = -1;

    if (!IS_ERR_OR_NULL(peu->act_metadata.mdnode)) {
        LOGERR("mdnode isn't null peu (%p) %lld\n", peu, peu->start_pos);
        return -EINVAL;
    }
    memset(peu->bit_map, 0,
           LFSM_CEIL_DIV((peu->eu_size - METADATA_SIZE_IN_SECTOR) >>
                         SECTORS_PER_FLASH_PAGE_ORDER, PAGES_PER_BYTE));
    if (log_frontier >= 0) {
        peu->log_frontier = log_frontier;
    }

    peu->usage = eu_usage;

    return 0;
}

/*
** To initialize the EUProperty structure of one eu
** Allocates memory for the bitmap of the eu. Memory is released in one_EU_destroy() during the driver unload
**
** @e : The eu to be initialized
** @s : The erasure unit size in sectors
** @start : The starting sector of the eu
**
** Returns void
*/
static bool one_EU_init(struct EUProperty *e, int32_t s, sector64 start,
                        int32_t disk_num)
{
    int32_t num_of_data_pages;
    num_of_data_pages =
        (s - METADATA_SIZE_IN_SECTOR) >> SECTORS_PER_FLASH_PAGE_ORDER;

    atomic_set(&e->erase_state, DIRTY);
    e->eu_size = s;
    atomic_set(&e->eu_valid_num, 0);
    e->index_in_updatelog = -1;
    e->log_frontier = 0;
    e->temper = 0;
    e->start_pos = start;
    atomic_set(&e->state_list, -1); //initial set
    e->index_in_LVP_Heap = -1;
    e->disk_num = disk_num;
    e->usage = EU_UNKNOWN;
    e->id_hpeu = -1;
    atomic_set(&e->cn_pinned_pages, 0);
    /* Alocating memory for EU-based bitmap */
    e->bit_map = (int8_t *)
        track_kmalloc((num_of_data_pages + PAGES_PER_BYTE - 1) >>
                      PAGES_PER_BYTE_ORDER, GFP_KERNEL, M_GC_BITMAP);

    if (IS_ERR_OR_NULL(e->bit_map)) {
        LOGERR("Allocating fail\n");
        return false;
    }

    memset(e->bit_map, 0,
           (num_of_data_pages + PAGES_PER_BYTE - 1) >> PAGES_PER_BYTE_ORDER);
    xorbmp_init(&e->xorbmp);
#ifndef EU_BITMAP_REFACTOR_PERF
    spin_lock_init(&e->lock_bitmap);
#endif
    mdnode_init(&e->act_metadata);
    e->fg_metadata_ondisk = false;
    e->erase_count = 0;

    dummy_write(e);
    return true;
}

/*
** To free the memory allocated for the bitmap of the eu
**
** @e : The EUProperty structure of the eu
**
** Returns void
*/
static void one_EU_destroy(struct EUProperty *e, lfsm_dev_t * td)
{
    int32_t num_of_data_pages;

    num_of_data_pages =
        (e->eu_size - METADATA_SIZE_IN_SECTOR) >> SECTORS_PER_FLASH_PAGE_ORDER;
    if (e->bit_map) {
        track_kfree(e->bit_map,
                    (num_of_data_pages + PAGES_PER_BYTE -
                     1) >> PAGES_PER_BYTE_ORDER, M_GC_BITMAP);
    }

    e->bit_map = 0;
    if (!IS_ERR_OR_NULL(e->act_metadata.mdnode)) {
        LOGWARN("A EU(%p) %llu active metadata not save\n", e, e->start_pos);
        mdnode_free(&e->act_metadata, td->pMdpool);
    }

    xorbmp_destroy(&e->xorbmp);
}

/*
** To release all the memory allocated in all_EUs_init()
**
** @h : Garbage Collection structure
**
** Returns void
*/
static void all_EUs_destroy(struct HListGC *h, uint64_t freenum)
{
    int32_t i, nRegion;

    nRegion = h->NumberOfRegions;
//    printk("%s: destroy %lu EUs\n", __FUNCTION__,freenum);
    free_composed_bio(h->metadata_io_bio, METADATA_BIO_VEC_SIZE);
    for (i = 0;
         i < (1 << (EU_ORDER - FLASH_PAGE_ORDER)) / READ_EU_ONE_SHOT_SIZE;
         i++) {
        free_composed_bio(h->read_io_bio[i],
                          READ_EU_ONE_SHOT_SIZE * MEM_PER_FLASH_PAGE);
    }

    ECT_destroy(&h->ECT);

//    printk("%s: free_list_number: %llu, hot_list: %d, LVP_Heap: %d, region: %d\n",
//        __FUNCTION__, h->free_list_number, h->hlist_number, h->LVP_Heap_number, h->NumberOfRegions);
    mutex_destroy(&h->lock_metadata_io_bio);
    rmutex_destroy(&h->whole_lock);
    //track_kfree(h->pbno_eu_map,nRegion*sizeof(int),M_GC);
    track_kfree(h->active_eus, nRegion * sizeof(struct EUProperty *), M_GC);

    track_kfree(h->par_cn_log, nRegion * sizeof(int32_t), M_GC);
}

void EU_cleanup(lfsm_dev_t * td)
{
    uint64_t freenum;
    int32_t i;

    freenum = grp_ssds.capacity >> (SECTORS_PER_SEU_ORDER);
    if (pbno_eu_map == 0) {
        return;
    }

    for (i = 0; i < freenum; i++) {
        //printk("LL: Clean EU %llu pos %llu valid=%d list=%d\n",pbno_eu_map[i]->start_pos/FPAGE_PER_SEU/ SECTORS_PER_FLASH_PAGE,
        //    pbno_eu_map[i]->start_pos,atomic_read(&pbno_eu_map[i]->eu_valid_num),atomic_read(&pbno_eu_map[i]->state_list));
        one_EU_destroy(pbno_eu_map[i], td);
        track_kfree(pbno_eu_map[i], sizeof(struct EUProperty), M_GC);
    }

    track_kfree(pbno_eu_map, freenum * sizeof(struct EUProperty *), M_GC);
}

int32_t EU_close_active_eu(lfsm_dev_t * td, struct HListGC *h,
                           int32_t(*move_func) (struct HListGC * h,
                                                struct EUProperty * eu))
{
    int32_t ret, i;

    for (i = 0; i < h->NumberOfRegions; i++) {
        if (NULL == (h->active_eus[i])) {
            continue;
        }

        LOGINFO("[%d] act_eu(%llu) frontier %d\n", h->disk_num,
                h->active_eus[i]->start_pos, h->active_eus[i]->log_frontier);
        if ((ret = flush_metadata_of_active_eu(td, h, h->active_eus[i])) < 0) {
            LOGERR("flush_metadata_of_active_eu fail ret=%d\n", ret);
            return ret;
        }
        //TODO: Bad coding style for lock and unlock
        LFSM_RMUTEX_LOCK(&h->whole_lock);

        if ((ret = move_func(h, h->active_eus[i])) < 0) {
            LFSM_RMUTEX_UNLOCK(&h->whole_lock);
            LOGERR("Fail to move eu %p %d\n", h->active_eus[i], ret);
            return ret;
        }
        LFSM_RMUTEX_UNLOCK(&h->whole_lock);
    }

    return 0;
}

/*
** The main destructor of the EUProperty structures during the unload of the driver
** To destroy the memory allocated  to maintain the heap
** Calls all_EUs_destroy()
**
** @h : Garbage Collection structure
** @td : lfsm_dev_t object
**
** Returns void
*/
int32_t EU_destroy(lfsm_dev_t * td)
{
    struct EUProperty *entry, *next;
    struct HListGC *h;
    int32_t i, ret, ret_final, disk_num;

    ret_final = 0;
    disk_num = 0;

    for (disk_num = 0; disk_num < td->cn_ssds; disk_num++) {
        if (gc == 0) {
            return -EINVAL;
        }

        if (IS_ERR_OR_NULL(h = gc[disk_num])) {
            continue;
        }
//        printk("%s: free_list_number: %llu, hlist_number: %d, LVP_Heap_number: %d, Region num: %d\n", __FUNCTION__,
//        h->free_list_number, h->hlist_number, h->LVP_Heap_number, h->NumberOfRegions);

        /* Moving EUs from HList to Free List */
        if (h->hlist_number > 0) {
            list_for_each_entry_safe(entry, next, &h->hlist, list) {
                HList_delete_A(h, entry);
                if ((ret = FreeList_insert(h, entry)) < 0) {
                    ret_final = ret;
                    LOGERR("Fail to freelist_insert1 %d\n", ret);
                }
            }
        }

        /* Moving EUs from LVP_Heap to Free List */
        for (i = h->LVP_Heap_number - 1; i >= 0; i--) {
            entry = h->LVP_Heap[i];
            LVP_heap_delete(h, entry);
            if ((ret = FreeList_insert(h, entry)) < 0) {
                ret_final = ret;
                LOGERR("Fail to freelist_insert2 %d\n", ret);
            }
        }

        if (IS_ERR_OR_NULL(h->active_eus)) {
            goto l_end;
        }
        /* Moving Active EUs to Free List */
        if ((ret = EU_close_active_eu(td, h, FreeList_insert)) < 0) {
            ret_final = ret;
            LOGERR("close_active_eu fail, ret = %d\n", ret);
        }
        //for(i = 0; i < h->NumberOfRegions; i++)
        //    check_mdlist_and_sparelpn(td,h->active_eus[i],NULL);
      l_end:
        h->LVP_Heap_number = 0;
        track_kfree(h->LVP_Heap, h->total_eus * sizeof(struct EUProperty *),
                    M_GC_HEAP);
        track_kfree(h->LVP_Heap_backup,
                    h->total_eus * sizeof(struct EUProperty *), M_GC_HEAP);

//        printk("Before freeing memory in function %s, free_list_number: %llu, hlist_number: %d, LVP_Heap_number: %d, Region num: %d\n", __FUNCTION__,
//        h->free_list_number, h->hlist_number, h->LVP_Heap_number, h->NumberOfRegions);

        all_EUs_destroy(h, h->total_eus);
    }

    return ret_final;
}

static int32_t system_eunext_init(lfsm_dev_t * td)
{
    if (!ECT_next_eu_init(td)) {
        LOGERR("Internal_err:%s ECT_next_eu_init fail\n", __FUNCTION__);
        return -ENOEXEC;
    }

    if (HPEU_eunext_init(&td->hpeutab) < 0) {
        LOGERR("init hpeu next fail\n");
        return -ENOEXEC;
    }

    if (UpdateLog_eunext_init(&td->UpdateLog) < 0) {
        LOGERR("init hpeu next fail\n");
        return -ENOEXEC;
    }
    //prepare dedicated next eu
    if (dedicated_map_eunext_init(&td->DeMap) < 0) {
        LOGERR("init ddmap nexteu fail\n");
        return -ENOEXEC;
    }

    return 0;
}

static int32_t system_eu_rescueA(lfsm_dev_t * td, int32_t id_fail_disk)
{
    int32_t ret;

    ret = 0;
    if (!UpdateLog_rescue(td, &td->UpdateLog)) {
        LOGERR("UpdateLog_rescue fail\n");
        return -EIO;
    }

    LOGINFO(" Updatelog rescue finish  EU\n");
    // rescure ECT, ECT lock is locked inside ECT rescue
    //if((ret=ECT_rescue(gc[id_fail_disk]))<0)

    if (ECT_rescue_disk(td, id_fail_disk) < 0) {
        LOGINERR("ECT_online_rescue fail\n");
        return ret;
    }

    LOGINFO(" ECT rescue finished\n");

    // rescure HPEU
    if (!HPEU_rescue(&td->hpeutab)) {
        LOGINERR("HPEU rescue fail code %d\n", ret);
        return -EIO;
    }

    LOGINFO(" HPEU rescue finished\n");

    // rescure BMT
    if (!BMT_rescue(td, td->ltop_bmt)) {
        LOGINERR("BMT rescue fail code %d\n", ret);
        return -EIO;
    }
    LOGINFO(" BMT rescue finished\n");

    // rescue Dedicate map
    if (!dedicated_map_rescue(td, &td->DeMap)) {
        LOGERR("dedicate map rescue fail code %d\n", ret);
        return -EIO;
    }
    LOGINFO(" dedicate_map rescue finished\n");

    // rescue Supermap
    if (super_map_rescue(td, &grp_ssds)) {  // supermap lock inside his function
        LOGERR("super map rescue %d\n", 1);
        return -ENOEXEC;
    }
    return ret;
}

static void subdev_set_devstate(diskgrp_t * pgrp, enum edev_state state)
{
    int32_t i;
    for (i = 0; i < pgrp->cn_max_drives; i++) {
        pgrp->ar_subdev[i].load_from_prev = state;
    }
}

/*
 * Description: init dedicate map EU if fresh start
 *              init all system eu if not fresh start
 */
static int32_t system_eu_init(lfsm_dev_t * td, diskgrp_t * pgrp)
{
    int32_t ret, idx_pg;

    ret = 0;
    if (td->prev_state == sys_unloaded || td->prev_state == sys_loaded) {
        if ((ret = system_eunext_init(td)) < 0) {
            return ret;
        }

        if ((ret = system_eu_rescueA(td, -1)) < 0) {
            return ret;
        }

        if (!ECT_commit_all(td, true)) {
            LOGINERR("ECT_commit_all fail\n");
            return -ENOEXEC;
        }

        if (update_ondisk_BMT(td, td->ltop_bmt, 0) < 0) {   //Reuse =1 as free_list is not yet available
            LOGERR("update_on_diskBMT fail\n");
            return -ENOEXEC;
        }
        // FIXME@weafon : should integrate into HPEU_load and should keep the next in ddmap.
        HPEU_sync_all_eus(&td->hpeutab);
        if (HPEU_save(&td->hpeutab)) {
            LOGERR("HPEU save fail\n");
            return -ENOEXEC;
        }

        if (UpdateLog_eu_init(&td->UpdateLog, 0, true) < 0) {
            LOGERR("init updatelog nexteu fail\n");
            return -ENOEXEC;
        }

        if (metalog_eulog_reset(&td->mlogger, gc, true) < 0) {
            LOGERR("init eulog fail\n");
            return -ENOEXEC;
        }

        if ((ret = dedicated_map_logging(td)) < 0) {
            return ret;
        }
    } else if (td->prev_state == sys_fresh) {
        dedicated_map_eu_init(&td->DeMap);
        if (ulfb_offset > 0) {
            UpdateLog_eu_init(&td->UpdateLog, HPAGE_PER_SEU - ulfb_offset,
                              false);
            LOGINFO("ul_frontier_back_offset %d \n", ulfb_offset);
        } else {                //ulfb_offset=0,-1,...
            UpdateLog_eu_init(&td->UpdateLog, 0, false);
        }
        LOGINFO("update log start: %llu %d , %llu %d\n",
                td->UpdateLog.eu_main->start_pos,
                td->UpdateLog.eu_main->disk_num,
                td->UpdateLog.eu_backup->start_pos,
                td->UpdateLog.eu_backup->disk_num);
        LOGINFO("update log next: %llu %d , %llu %d\n",
                td->UpdateLog.next.eu_main->start_pos,
                td->UpdateLog.next.eu_main->disk_num,
                td->UpdateLog.next.eu_backup->start_pos,
                td->UpdateLog.next.eu_backup->disk_num);

        for (idx_pg = 0; idx_pg < td->cn_ssds; idx_pg++) {
            ExtEcc_eu_init(td, idx_pg, false);
            LOGINFO("External ECC start: %llu %d\n",
                    td->ExtEcc[idx_pg].eu_main->start_pos,
                    td->ExtEcc[idx_pg].eu_main->disk_num);
        }

        metalog_eulog_reset(&td->mlogger, gc, false);
        subdev_set_devstate(pgrp, all_ready);
        if ((ret = dedicated_map_logging(td)) < 0) {
            return ret;
        }
        LOGINFO("ddmap start: %llu\n", td->DeMap.eu_main->start_pos);
        super_map_logging_all(td, pgrp);
        LOGINFO("SUPER map logging done\n");
    }

    return 0;
}

/*
** To categorize EUs into different sections: Heap, HList and Active EUs
** load_from_prev flag tells whether previous unload of the driver was successful or not
** Also, it identifies a fresh new disk as well
**
** Move the EUs from the free list to their respective categories based on bitmap
** @h : Garbage Collection structure
**
** Returns true: fail
**         false: ok
*/
bool categorize_EUs_based_on_bitmap(lfsm_dev_t * td)
{
    int32_t i;

    /// If it's an already used disk no matter successfully unloaded or not
    /// Use eu_valid_number info to insert the elements into the LVP Heap
    if (td->prev_state != sys_fresh) {

//        if (HPEU_iterate(&td->hpeutab, rddcy_search_failslot(td), FreeList_delete_eu_direct)<0)
        if (HPEU_iterate(&td->hpeutab, -1, FreeList_delete_eu_direct) < 0) {
            return true;
        }
    }

    if (system_eu_init(td, &grp_ssds) < 0) {
        return true;
    }

    if (metalog_create_thread_all(&lfsmdev.mlogger) < 0) {
        return true;
    }

    HPEU_reset_birthday(&td->hpeutab);
    for (i = 0; i < td->cn_ssds; i++) {
        if (!active_eu_init(td, gc[i])) {
            return true;
        }
    }

    return false;
}

static bool eu_is_supermap(struct EUProperty *eu)
{
    sector64 addr;
    int32_t id_dev;

    pbno_lfsm2bdev(eu->start_pos, &addr, &id_dev);
    if ((addr >> SECTORS_PER_FLASH_PAGE_ORDER) == 0) {
        return true;
    }
    return false;
}

static int32_t identify_bad_EU(diskgrp_t * pgrp, int32_t id_baddisk)
{
    // serach all EU in id_disk
    struct EUProperty *eu;
    int32_t i, start;

    start = eumap_start(id_baddisk);

    for (i = 0; i < gc[id_baddisk]->total_eus; i++) {
        eu = pbno_eu_map[start + i];

        if (eu->disk_num != id_baddisk) {
            LOGERR("wrong id_disk start: %d  num: %llu, diskid %d, i : %d\n",
                   start, gc[id_baddisk]->total_eus, id_baddisk, start + i);
            return -ERANGE;
        }

        if ((!eu_is_supermap(eu)) && (eu->erase_count == EU_UNUSABLE)) {
            FreeList_insert_withlock(eu);   // insert original bad block
        }

        eu->erase_count = 0;    // reset all eu erase count
        atomic_set(&eu->erase_state, DIRTY);    // reset all eu erase state
    }

    atomic_set(&gc[id_baddisk]->cn_clean_eu, 0);

    return 0;
}

int32_t system_eu_rescue(lfsm_dev_t * td, int32_t id_fail_disk)
{
    int32_t ret;

    LOGINFO("start rebuild fail disk %d SYSTEM part\n", id_fail_disk);

    if ((ret = identify_bad_EU(&grp_ssds, id_fail_disk)) < 0) { // clean the every eu's EU->erase_count
        return ret;
    }
    // rescure update log
    LOGINFO(" rescue finish idetify bad EU\n");

    if ((ret = system_eu_rescueA(td, id_fail_disk)) < 0) {
        return ret;
    }

    LOGINFO("SUPERMAP ready %d\n", 1);
    if ((ret = dedicated_map_logging(td)) < 0) {
        return ret;
    }

    return 0;
}

// initial active eus for one disk
bool active_eu_init(lfsm_dev_t * td, struct HListGC *h)
{
    int32_t i;

    sort_erase_count(h);

    //TODO: bad lock unlock coding style
    for (i = 0; i < h->NumberOfRegions; i++) {
        LFSM_RMUTEX_LOCK(&h->whole_lock);
        if (NULL == (change_active_eu_A(td, h, i, false))) {
            LFSM_RMUTEX_UNLOCK(&h->whole_lock);
            return false;
        }
        UpdateLog_logging(td, h->active_eus[i], i, UL_NORMAL_MARK);
        LFSM_RMUTEX_UNLOCK(&h->whole_lock);
    }
    return true;
}

//return 0 means read from main, 1 means read from bkup, <0 means read fail
int32_t EU_spare_read_unit(lfsm_dev_t * td, int8_t * buffer,
                           sector64 addr_main, sector64 addr_bkup, int32_t size)
{
    if ((addr_main == -1)
        || (devio_query_lpn(buffer, addr_main, size) == false)) {
        if (addr_bkup == -1) {
            return -EIO;
        } else if (devio_query_lpn(buffer, addr_bkup, size) == false) {
            return -EIO;
        }
        return 1;
    }
    return 0;
}

bool EU_spare_read_A(lfsm_dev_t * td, int8_t * buffer, sector64 addr_main,
                     sector64 addr_bkup, int32_t num_pages, int8_t * ar_isbkup)
{
    int32_t num_page_in_round;
    int32_t ret;

    ret = 0;
    while (num_pages > 0) {
        if (num_pages > MAX_LPN_ONE_SHOT) {
            num_page_in_round = MAX_LPN_ONE_SHOT;
        } else {
            num_page_in_round = num_pages;
        }

        if ((ret =
             EU_spare_read_unit(td, buffer, addr_main, addr_bkup,
                                num_page_in_round << SZ_SPAREAREA_ORDER)) < 0) {
            return false;
        }

        if (ar_isbkup) {
            memset(ar_isbkup, ret, num_page_in_round);
            ar_isbkup += num_page_in_round;
        }
        buffer += (num_page_in_round << SZ_SPAREAREA_ORDER);
        addr_main = (addr_main == -1) ? -1 :
            (addr_main + (num_page_in_round << SECTORS_PER_FLASH_PAGE_ORDER));
        addr_bkup = (addr_bkup == -1) ? -1 :
            (addr_bkup + (num_page_in_round << SECTORS_PER_FLASH_PAGE_ORDER));
        num_pages -= num_page_in_round;
    }

    return true;
}

bool EU_spare_read(lfsm_dev_t * td, int8_t * buffer, sector64 addr,
                   int32_t num_pages)
{
    return EU_spare_read_A(td, buffer, addr, -1, num_pages, NULL);
}

/*
** To initialize the EUProperty structure of all the EUs
** Calls one_EU_init on all the eus
** Initializes the active eus based on the number of regions (temperatures)
** Allocates read_io_bio s by calling compose_bio(). These read_io_bio s are used during gc
**
** @h : Garbage Collection structure
** @nRegion : Number of temperatures intended (3, in general)
** @cn_eu_of_disk : Number of EUs in total for this disk
** @e_size : Size of one eu in sectors
** @log_start : The starting sector of the data log (data log mean loc for user data)
** @prev_disk_eus : Total number of EUs initialized so far.
**
** Returns true: init fail
**         false: init ok
*/
static bool all_EUs_init(lfsm_dev_t * td, struct HListGC *h, int32_t nRegion,
                         sector64 cn_eu_of_disk, int32_t e_size,
                         sector64 log_start, uint64_t prev_disk_eus,
                         int32_t disk_num)
{
    struct EUProperty *e;
    int32_t i;
    sector64 eu_sector_offset;

    h->free_list_number = 0;
    atomic_set(&h->cn_clean_eu, 0);
    h->eu_size = e_size;
    h->eu_log_start = log_start;
    h->NumberOfRegions = nRegion;
    h->total_eus = cn_eu_of_disk;
    h->pbno_eu_map = pbno_eu_map;   //tf:question
    mutex_init(&h->lock_metadata_io_bio);
    rmutex_init(&h->whole_lock);

    if (IS_ERR_OR_NULL(h->pbno_eu_map)) {
        LOGERR("pbno_eu_map is NULL\n");
        return true;
    }

    INIT_LIST_HEAD(&h->free_list);

    for (i = cn_eu_of_disk - 1; i >= 0; i--) {
        e = (struct EUProperty *)
            track_kmalloc(sizeof(struct EUProperty), GFP_KERNEL, M_GC);
        if (IS_ERR_OR_NULL(e)) {
            LOGERR("fail alloc mem EU when init all EUs\n");
            return true;
        }

        eu_sector_offset = (sector64) i *h->eu_size;
        if (!one_EU_init
            (e, e_size, h->eu_log_start + eu_sector_offset, disk_num)) {
            return true;
        }

        INIT_LIST_HEAD(&e->list);

        /* All EUs except the first two go to the free pool during intialization */
        if (FreeList_insert(h, e) < 0) {
            LOGERR("fail insert EU to free list\n");
            LFSM_ASSERT(0);
        }

        /* Set the mapping array so as to easily fetch a struct EUProperty pointer from PBN */
        pbno_eu_map[i + prev_disk_eus] = e; // EUProperty_kmalloc
    }

    h->active_eus = (struct EUProperty **)
        track_kmalloc(h->NumberOfRegions * sizeof(struct EUProperty *),
                      GFP_KERNEL, M_GC);
    if (IS_ERR_OR_NULL(h->active_eus)) {
        LOGERR("Allocating active eus fail\n");
        return true;
    }

    h->par_cn_log = (int32_t *)
        track_kmalloc(h->NumberOfRegions * sizeof(int32_t), GFP_KERNEL, M_GC);
    if (IS_ERR_OR_NULL(h->par_cn_log)) {
        LOGERR("allocate ar_cn_log fail\n");
        return true;
    }
    memset(h->par_cn_log, 0, h->NumberOfRegions * sizeof(int));

    /*printk("\nAt the beginning in function %s, free_list_number: %llu, hlist_number: %d, LVP_Heap_number: %d, f: %llu\n", __FUNCTION__,
       h->free_list_number, h->hlist_number, h->LVP_Heap_number, f); */
    memset(h->active_eus, 0, h->NumberOfRegions * sizeof(struct EUProperty *));
    init_waitqueue_head(&h->io_queue);
    atomic_set(&h->io_queue_fin, 0);

    /* Allocating memory for bios used during an EU read for GC */
    for (i = 0;
         i < (1 << (EU_ORDER - FLASH_PAGE_ORDER)) / READ_EU_ONE_SHOT_SIZE;
         i++) {
        if (compose_bio
            (&h->read_io_bio[i], NULL, gc_end_bio_read_io, h,
             READ_EU_ONE_SHOT_SIZE * FLASH_PAGE_SIZE,
             READ_EU_ONE_SHOT_SIZE * MEM_PER_FLASH_PAGE) < 0) {
            LOGERR("compose_bio(read_io_bio) fail\n");
            return true;
        }
//        h->read_io_bio_org_flag[i] = h->read_io_bio[i]->bi_flags;
    }

    if (compose_bio(&h->metadata_io_bio, NULL, gc_end_bio_read_io, h,
                    METADATA_BIO_VEC_SIZE * PAGE_SIZE,
                    METADATA_BIO_VEC_SIZE) < 0) {
        LOGERR("compose_bio(metadata_io_bio) fail\n");
        return true;
    }

    if (!ECT_init(h, cn_eu_of_disk)) {
        LOGERR("ECT_init fail\n");
        return true;
    }

    return false;
}

/*
** The main constructor of the EUProperty structures
** Caller of the all_EUs_init(). Called during the initialization
** Allocates memory to maintain the heap
**
** @h : Garbage Collection structure
** @nRegion : Number of temperatures intended (3, in general)
** @free_eus_in_disk : Number of EUs in a disk
** @euS : Size of one eu in sectors
** @hl : hlist limit (10%, by default)
** @log_start : The starting sector of the data log
** @prev_disk_eus : Total number of EUs initialized so far.
**
** Returns : true : init fail
**           false: init ok
*/
bool EU_init(lfsm_dev_t * td, struct HListGC *h, int32_t nRegion,
             sector64 free_eus_in_disk, int32_t euSize, int32_t hl,
             sector64 log_start, uint64_t prev_disk_eus, int32_t disk_num)
{
    int32_t retry;

    retry = 0;
    /* To initialize all the EUs, their bitmaps, pbn-to-EU mapping array and read-eu bios */
    if (all_EUs_init
        (td, h, nRegion, free_eus_in_disk, euSize, log_start, prev_disk_eus,
         disk_num)) {
        LOGERR("all_EUs_init fail\n");
        return true;
    }

    /* HList limit is set */
    h->hlist_limit = (int32_t) free_eus_in_disk / HLIST_CAPACITY_RATIO;

    /* This is the list which holds the EUs in the HList and it's count */
    INIT_LIST_HEAD(&h->hlist);
    h->hlist_number = 0;

    /* This is the list which holds the EUs in the FalseList and it's count */
    INIT_LIST_HEAD(&h->false_list);
    atomic_set(&h->false_list_number, 0);

    /* Allocating memory for the array to store the heap of EUs and it's count */
    while (IS_ERR_OR_NULL(h->LVP_Heap = (struct EUProperty **)
                          track_kmalloc(free_eus_in_disk *
                                        sizeof(struct EUProperty *), GFP_KERNEL,
                                        M_GC_HEAP))) {
        LOGERR("Allocating LVP_Heap fail (retry=%d)\n", retry);
        schedule();
        if ((retry++) > LFSM_MAX_RETRY) {
            return true;
        }
    }

    retry = 0;
    while (IS_ERR_OR_NULL(h->LVP_Heap_backup = (struct EUProperty **)
                          track_kmalloc(free_eus_in_disk *
                                        sizeof(struct EUProperty *), GFP_KERNEL,
                                        M_GC_HEAP))) {
        LOGERR("Allocating LVP_Heap fail (retry=%d)\n", retry);
        schedule();
        if ((retry++) > LFSM_MAX_RETRY) {
            goto l_LVP_backup_fail;
        }
    }

    h->LVP_Heap_backup_number = 0;
    h->LVP_Heap_number = 0;
    if (IS_ERR_OR_NULL(h->LVP_Heap)) {
        LOGERR("LVP_Heap is NULL\n");
        goto l_fail;
    }

    h->min_erase_eu = NULL;
    h->max_erase_eu = NULL;

    LOGINFO("cn_free : %llu, cn_hlist: %d, cn_LVP_Heap: %d, f: %llu\n",
            h->free_list_number, h->hlist_number, h->LVP_Heap_number,
            free_eus_in_disk);
    return false;

  l_fail:
    track_kfree(h->LVP_Heap_backup,
                free_eus_in_disk * sizeof(struct EUProperty *), M_GC_HEAP);
  l_LVP_backup_fail:
    track_kfree(h->LVP_Heap, free_eus_in_disk * sizeof(struct EUProperty *),
                M_GC_HEAP);
    return true;
}
