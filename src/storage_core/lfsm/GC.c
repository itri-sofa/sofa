/*
 * SPDX-License-Identifier: GPL-2.0-or-later OR Apache-2.0
 * Copyright (C) 2015-2020 Industrial Technology Research Institute
 *
 *
 *
 *  
 */
/*
** This file constitues of all the functions related to the garbage collection
*/

#include <linux/types.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/wait.h>
#include "config/lfsm_setting.h"
#include "config/lfsm_feature_setting.h"
#include "config/lfsm_cfg.h"
#include "common/mem_manager.h"
#include "common/rmutex.h"
#include "common/common.h"
#include "io_manager/mqbiobox.h"
#include "io_manager/biocbh.h"
#include "nextprepare.h"
#include "diskgrp.h"
#include "system_metadata/Dedicated_map.h"
#include "lfsm_core.h"
#include "bmt.h"
#include "io_manager/io_common.h"
#include "io_manager/io_write.h"
#include "EU_create.h"
#include "EU_access.h"
#include "GC.h"
#include "spare_bmt_ul.h"
#include "bmt_ppq.h"
#include "lfsm_test.h"
#include "bmt_ppdm.h"
#include "err_manager.h"
#include "hpeu_gc.h"
#include "metabioc.h"
#include "mdnode.h"

/*
** @h : Garbage Collection structure
**
** Returns true if the garbage collection threshold is hit
** False, otherwise.
** return 0: no gc need, 
**        1: hit normal gc threshold, 
**        2: hit false gc threshold
*/
int32_t gc_threshold(struct HListGC *h, int32_t gc_trigger)
{
    sector64 total_eu_count;
    uint32_t gc_off_upper, gc_on_lower;
    int32_t ret;

    total_eu_count = h->LVP_Heap_number + h->hlist_number + h->free_list_number;
    ret = GC_OFF;
    tf_printk("num of {heap, hlist, free}={%d, %d, %llu}\n",
              h->LVP_Heap_number, h->hlist_number, h->free_list_number);

    // check if false gc trigger
    if (atomic_read(&(gc[HPEU_ID_FIRST_DISK(h->disk_num)]->false_list_number))
        > 0) {
        return FALSE_GC;
    }

    /*
     * lfsmCfg.gc_lower_ratio: default is 125,
     *     if h->free_list_number <= total*(125/1000) then trigger normal GC
     *     => h->free_list_number*1000/125 <= total_eu_count
     *     => h->free_list_number*8 <= total_eu_count
     * lfsmCfg.gc_upper_ratio: default is 250,
     *     if h->free_list_number >= total*(250/1000) then disable normal GC
     *     => h->free_list_number*(250/1000) >= total_eu_count
     *     => h->free_list_number*4 >= total_eu_count
     */
    gc_off_upper =
        h->free_list_number * GC_PARA_RATIO_BASE / lfsmCfg.gc_upper_ratio;
    gc_on_lower =
        h->free_list_number * GC_PARA_RATIO_BASE / lfsmCfg.gc_lower_ratio;

    // check if all_invalid gc happened, then turn off gc
    if (gc_trigger == ALL_INVALID_GC) {
        ret = GC_OFF;
    } else if (gc_trigger == GC_OFF && gc_on_lower <= total_eu_count) {
        //(GC_UP_THRESHOLD_FACTOR * h->free_list_number) <= total_eu_count) {
        ret = ALL_INVALID_GC;
    }
    // check if > age_group_num start gc; if > gc_threshold_factor stop gc
    if (h->free_list_number <= MIN_RESERVE_EU || gc_on_lower <= total_eu_count) {
        //        (GC_DN_THRESHOLD_FACTOR * h->free_list_number) <= total_eu_count) {
        //gc_on_lower <= total_eu_count
        ret = NORMAL_GC;
    } else if (gc_trigger == NORMAL_GC && gc_off_upper >= total_eu_count) {
        //(GC_UP_THRESHOLD_FACTOR * h->free_list_number) >= total_eu_count) {
        //gc_off_upper >= total_eu_count
        ret = GC_OFF;
    }

    return ret;
}

/*
** @h : Garbage Collection structure
** @eu : The erasure unit to be tested
**
** Returns true if the eu is one of the active eus
** False, otherwise.
*/
static bool gc_enough_eu(int32_t cn_invalid, int32_t free)
{
    dprintk("HH: total_invalid= %d\n", cn_invalid);

    if (cn_invalid >= (free * (FPAGE_PER_SEU - METADATA_SIZE_IN_FLASH_PAGES))) {    //TF
        return true;
    } else {
        return false;
    }
}

void gc_item_init(gc_item_t * gc_item, struct EUProperty *pick_eu)
{
    gc_item->eu = pick_eu;
    gc_item->cn_valid_pg = 0;
    gc_item->id_srclist = -1;
    gc_item->ar_pbioc = NULL;
    memset(&gc_item->gc_addr, 0, sizeof(gcadd_info_t));
}

static bool gc_entry_alloc_insert(struct list_head *list_to_gc,
                                  struct EUProperty *pick_eu)
{
    EU2GC_item_t *entry;
    if (NULL == (entry = track_kmalloc(sizeof(EU2GC_item_t), GFP_KERNEL, M_GC))) {
        return false;
    }

    gc_item_init(&entry->gc_item, pick_eu);
    list_add_tail(&entry->next, list_to_gc);
    return true;
}

static void LVPHeap_backup_pop(struct HListGC *h)
{
    h->LVP_Heap_backup[0] = h->LVP_Heap_backup[h->LVP_Heap_backup_number - 1];
    h->LVP_Heap_backup_number--;
    drift_element_down(h, 0, true);
}

static int32_t gc_pick_from_heap_tree(struct HListGC *h,
                                      struct list_head *list_to_gc,
                                      int32_t * total_invalid, int32_t gc_flag)
{
    struct EUProperty *eu_header;
    int32_t cn_picked;

    cn_picked = 0;
    memcpy(h->LVP_Heap_backup, h->LVP_Heap,
           (h->LVP_Heap_number) * sizeof(struct EUProperty *));
    h->LVP_Heap_backup_number = h->LVP_Heap_number;

    // select all invalid EU first
    while (h->LVP_Heap_backup_number > 0) {
        eu_header = h->LVP_Heap_backup[0];
        if (eu_header->usage == EU_GC) {
            LVPHeap_backup_pop(h);
            continue;
        }

        if (!HPEUgc_selectable(&lfsmdev.hpeutab, eu_header->id_hpeu)) {
            LOGWARN("heap pick eu %llu is active or with mlog\n",
                    eu_header->start_pos);
            LVPHeap_backup_pop(h);
            continue;
        }

        if (atomic_read(&eu_header->eu_valid_num) > 0) {
            //break for a eu with valid page but all invalid eu picked
            break;
        }

        HPEUgc_entry_alloc_insert(list_to_gc, eu_header);
        LVPHeap_backup_pop(h);
        *total_invalid = (FPAGE_PER_SEU - METADATA_SIZE_IN_FLASH_PAGES);
        cn_picked++;
    }

    if (gc_flag == ALL_INVALID_GC) {
        return cn_picked;
    }

    while (h->LVP_Heap_backup_number > 0) {
        eu_header = h->LVP_Heap_backup[0];
        if (eu_header->usage == EU_GC) {
            LVPHeap_backup_pop(h);
            continue;
        }

        if (!HPEUgc_selectable(&lfsmdev.hpeutab, eu_header->id_hpeu)) {
            LOGWARN("heap pick eu %llu is active or with mlog\n",
                    eu_header->start_pos);
            LVPHeap_backup_pop(h);
            continue;
        }

        if (IS_ALL_VALID(h, eu_header)) {
            break;
        }

        if (gc_enough_eu(*total_invalid, 1)
            && VALID_OVER_THRESHOLD(h, eu_header, 7)) {
            break;
        }

        *total_invalid += (FPAGE_PER_SEU - METADATA_SIZE_IN_FLASH_PAGES)
            -
            (atomic_read(&eu_header->eu_valid_num) >>
             SECTORS_PER_FLASH_PAGE_ORDER);
        HPEUgc_entry_alloc_insert(list_to_gc, eu_header);
        LVPHeap_backup_pop(h);
        tf_printk("HH: heap pick eu %llu valid num %d temper %d\n",
                  eu_header->start_pos, atomic_read(&eu_header->eu_valid_num),
                  eu_header->temper);
        cn_picked++;
    }

    //printk("select %d EU to be gc(%d) \n",cn_picked, gc_flag);
    return cn_picked;
}

/*
** @h : Garbage Collection structure
** @head : List head of EU_to_be_GC_list
** @total_valid_blks : The array storing the number of valid blocks of each temperature
** of all the eus picked till now
**
** Returns true if enough eus could be picked from hlist to ensure garbage collection success
** False, otherwise.
*/
static int32_t gc_pick_from_hlist(struct HListGC *h, struct list_head *head,
                                  int32_t * total_invalid)
{
    struct EUProperty *pick_eu;
    int32_t i, cn_picked;

    cn_picked = 0;
    i = h->LVP_Heap_number;

    dprintk("picking from Hlist\n");
    list_for_each_entry_reverse(pick_eu, &h->hlist, list) {
        LFSM_ASSERT(atomic_read(&pick_eu->state_list) == 1);
        LFSM_ASSERT(pick_eu->index_in_LVP_Heap == -1);

        LFSM_ASSERT(h->disk_num == pick_eu->disk_num);
        LFSM_ASSERT(atomic_read(&pick_eu->eu_valid_num) <
                    (h->eu_size - METADATA_SIZE_IN_SECTOR))
            if (pick_eu->usage == EU_GC) {
            continue;
        }

        if (!HPEUgc_selectable(&lfsmdev.hpeutab, pick_eu->id_hpeu)) {
            continue;
        }

        if (gc_enough_eu(*total_invalid, 1)) {
            break;
        }

        *total_invalid += (FPAGE_PER_SEU - METADATA_SIZE_IN_FLASH_PAGES)
            -
            (atomic_read(&pick_eu->eu_valid_num) >>
             SECTORS_PER_FLASH_PAGE_ORDER);
        HPEUgc_entry_alloc_insert(head, pick_eu);
        cn_picked++;
        tf_printk("HH: hlist pick eu %llu valid num %d temper %d\n",
                  pick_eu->start_pos, atomic_read(&pick_eu->eu_valid_num),
                  pick_eu->temper);
        i++;
    }

    return cn_picked;
}

/*
** @h : Garbage Collection structure
** @head : List head of EU_to_be_GC_list
** @total_valid_blks : The array storing the number of valid blocks of each temperature
** of all the eus picked till now
**
** Returns true if enough eus could be picked from hlist to ensure garbage collection success
** False, otherwise.
*/
static int32_t gc_pick_from_falselist(struct HListGC *h, struct list_head *head)
{
    struct EUProperty *pick_eu;

    int32_t cn = 0;
    if (atomic_read(&h->false_list_number) == 0) {
        return cn;
    }

    LOGINFO("picked from Falselist\n");

    list_for_each_entry_reverse(pick_eu, &h->false_list, list) {
        LOGINFO("HH: falselist pick eu %llu valid num %d temper %d\n",
                pick_eu->start_pos, atomic_read(&pick_eu->eu_valid_num),
                pick_eu->temper);
        LFSM_ASSERT(atomic_read(&pick_eu->state_list) == 2);
        LFSM_ASSERT(pick_eu->index_in_LVP_Heap == -1);
        LFSM_ASSERT(eu_is_active(pick_eu) < 0);

        LFSM_ASSERT(h->disk_num == pick_eu->disk_num);
        LFSM_ASSERT(atomic_read(&pick_eu->eu_valid_num) <=
                    (h->eu_size - METADATA_SIZE_IN_SECTOR))
            HPEUgc_entry_alloc_insert(head, pick_eu);
        cn++;
    }

    return cn;
}

static int32_t gc_pick_less_erase_EU(struct HListGC *h,
                                     struct list_head *list_to_gc)
{
    struct EUProperty *eu_header;
    int32_t i, select_cn, limit_picked_eus;

    limit_picked_eus = 5;
    select_cn = 0;

    LOGINFO("min:%d(%llu) max:%d(%llu)\n", h->min_erase_eu->erase_count,
            h->min_erase_eu->start_pos, h->max_erase_eu->erase_count,
            h->max_erase_eu->start_pos);

    for (i = 0; i < h->LVP_Heap_number; i++) {
        if (h->LVP_Heap[i]->erase_count <= h->min_erase_eu->erase_count) {
            gc_entry_alloc_insert(list_to_gc, h->LVP_Heap[i]);
            LOGINFO("GC_CU: select %llu erase count %d valid %d\n",
                    h->LVP_Heap[i]->start_pos, h->LVP_Heap[i]->erase_count,
                    atomic_read(&h->LVP_Heap[i]->eu_valid_num));
            if (select_cn++ > limit_picked_eus) {
                break;
            }
        }
    }

    list_for_each_entry_reverse(eu_header, &h->hlist, list) {
        if (eu_header->erase_count <= h->min_erase_eu->erase_count) {
            gc_entry_alloc_insert(list_to_gc, eu_header);
            LOGINFO("GC_CU: select %llu erase count %d valid %d\n",
                    eu_header->start_pos, eu_header->erase_count,
                    atomic_read(&eu_header->eu_valid_num));
            if (select_cn++ > limit_picked_eus) {
                break;
            }
        }
    }

    h->min_erase_eu = h->max_erase_eu;
    return select_cn;
}

/*
** Interrupt handler for h->read_io_bio
** Sends wake up call to h->io_queue if all the pages of the eu are read.
**
** Returns void
*/
void gc_end_bio_read_io(struct bio *bio)
{
    struct HListGC *h;

    h = bio->bi_private;
    LFSM_ASSERT(h != NULL);
    atomic_inc(&h->io_queue_fin);
    bio->bi_iter.bi_size = READ_EU_ONE_SHOT_SIZE * FLASH_PAGE_SIZE;

    if (atomic_read(&h->io_queue_fin) ==
        ((1 << (EU_ORDER - FLASH_PAGE_ORDER)) / READ_EU_ONE_SHOT_SIZE)) {
        wake_up_interruptible(&h->io_queue);
    }

    return;
}

/*
** Interrupt handler for h->read_io_bio[0] used to read the metadata of the eu to be gc
** Sends wake up call to h->io_queue
**
** Returns void
*/
static void gc_end_bio_read_metadata_io(struct bio *bio)
{
    struct HListGC *h;
    int32_t err;

    h = bio->bi_private;
    LFSM_ASSERT(h != NULL);
    err = blk_status_to_errno(bio->bi_status);

    if (err) {
        atomic_set(&h->io_queue_fin, err);
    } else {
        atomic_inc(&h->io_queue_fin);
    }

    bio->bi_iter.bi_size = METADATA_SIZE_IN_SECTOR - DUMMY_SIZE_IN_SECTOR;
    wake_up_interruptible(&h->io_queue);

    return;
}

static bool metadata_is_empty(struct page **ar_mpage)
{
    onDisk_md_t *mdata;
    bool ret;

    ret = true;
    mdata = kmap(ar_mpage[METADATA_SIZE_IN_PAGES - 1]);
    if (mdata[METADATA_ENTRIES_PER_PAGE - 1].lbno == TAG_METADATA_WRITTEN) {
        ret = false;
    }
    kunmap(ar_mpage[METADATA_SIZE_IN_PAGES - 1]);
    return ret;
}

int32_t meta_chkecksum_on_mpage(struct page **ar_mpage)
{
    onDisk_md_t *md_ptr;
    sector64 checksum;
    int32_t i, j;

    if (metadata_is_empty(ar_mpage)) {
        LOGWARN("metadata is empty\n");
        return 0;
    }

    checksum = 0;
    for (i = 0; i < METADATA_SIZE_IN_PAGES; i++) {
        md_ptr = (onDisk_md_t *) kmap(ar_mpage[i]);
        for (j = 0; j < METADATA_ENTRIES_PER_PAGE; j++) {
            checksum ^=
                (md_ptr[i % METADATA_ENTRIES_PER_PAGE].
                 lbno & (~LFSM_ALLOC_PAGE_MASK));
        }
        kunmap(ar_mpage[i]);
    }

    if (checksum != 0) {
        LOGERR("final checksum=%08llx should be 0!\n", checksum);
        return -1;
    }

    return 1;                   //mean success
}

static void _gcread_eu_md_wait(struct HListGC *h)
{
    int32_t cnt, wait_ret;

    cnt = 0;
    while (true) {
        cnt++;
        wait_ret = wait_event_interruptible_timeout(h->io_queue,
                                                    atomic_read(&h->
                                                                io_queue_fin) !=
                                                    0, LFSM_WAIT_EVEN_TIMEOUT);
        if (wait_ret <= 0) {
            LOGWARN("gc read eu metadata IO no response after seconds %d\n",
                    LFSM_WAIT_EVEN_TIMEOUT_SEC * cnt);
        } else {
            break;
        }
    }
}

 /*
  ** To read the metadata page which lies at the end of the eu
  **
  ** @td: lfsm_dev_t object
  ** @h : Garbage Collection structure
  ** @pos : Starting sector of the eu to be read
  ** @p : Pointer to the page address to which the metadata read is mapped to.
  **
  ** Returns -1:mean read error; 0:mean empty; 1:mean success
  */
int32_t gc_read_eu_metadata(lfsm_dev_t * td, struct HListGC *h, sector64 pos,
                            struct page **ar_mpage)
{
    sector64 final_pos;
    int32_t i, ret;

    ret = 1;

    atomic_set(&h->io_queue_fin, 0);
    h->metadata_io_bio->bi_iter.bi_size =
        (METADATA_SIZE_IN_SECTOR - DUMMY_SIZE_IN_SECTOR) << SECTOR_ORDER;
    h->metadata_io_bio->bi_next = NULL;
    h->metadata_io_bio->bi_iter.bi_idx = 0;
    h->metadata_io_bio->bi_status = BLK_STS_OK;
    h->metadata_io_bio->bi_flags &= ~(1 << BIO_SEG_VALID);
    h->metadata_io_bio->bi_disk = NULL;
    h->metadata_io_bio->bi_phys_segments = 0;
    h->metadata_io_bio->bi_seg_front_size = 0;
    h->metadata_io_bio->bi_seg_back_size = 0;
    h->metadata_io_bio->bi_opf = REQ_OP_READ;

    final_pos = pos + (1 << (SECTORS_PER_SEU_ORDER)) -
        (METADATA_SIZE_IN_SECTOR - DUMMY_SIZE_IN_SECTOR);

    h->metadata_io_bio->bi_iter.bi_sector = final_pos;

    tf_printk("reading meta from %llu start_pos=%llu\n", final_pos, pos);

    LFSM_ASSERT((final_pos % SECTORS_PER_FLASH_PAGE) == 0);

    h->metadata_io_bio->bi_vcnt = METADATA_SIZE_IN_PAGES;
    for (i = 0; i < METADATA_SIZE_IN_PAGES; i++) {

        h->metadata_io_bio->bi_io_vec[i].bv_offset = 0;
        h->metadata_io_bio->bi_io_vec[i].bv_len = PAGE_SIZE;
    }
    h->metadata_io_bio->bi_end_io = gc_end_bio_read_metadata_io;

    if (my_make_request(h->metadata_io_bio) == 0) {
        //lfsm_wait_event_interruptible(h->io_queue,
        //        atomic_read(&h->io_queue_fin) != 0);
        _gcread_eu_md_wait(h);
    }

    if (atomic_read(&h->io_queue_fin) != 1) {
        diskgrp_logerr(&grp_ssds, h->disk_num);
        return -1;
    }
    atomic_set(&h->io_queue_fin, 0);

    for (i = 0; i < METADATA_SIZE_IN_PAGES; i++) {
        ar_mpage[i] = h->metadata_io_bio->bi_io_vec[i].bv_page;
        //dumphex_sector((char *)kmap(p[i]),64);
        //kunmap(p[i]);
    }

    ret = meta_chkecksum_on_mpage(ar_mpage);
    return ret;
}

static bool meta_chkecksum_on_mem(uint8_t ** ar_page)
{
    onDisk_md_t *md_ptr;
    sector64 checksum;
    int32_t i, j, cn_round;

    checksum = 0;
    for (i = 0; i < METADATA_SIZE_IN_PAGES; i++) {
        md_ptr = (onDisk_md_t *) ar_page[i];
        cn_round = (i == METADATA_SIZE_IN_PAGES - 1) ? (METADATA_ENTRIES_PER_PAGE - METADATA_SIZE_IN_FLASH_PAGES + 1) : METADATA_ENTRIES_PER_PAGE;  // +1 include chksum

        for (j = 0; j < cn_round; j++) {
            checksum ^= (md_ptr[j].lbno & (~LFSM_ALLOC_PAGE_MASK));
        }
    }

    if (checksum != 0) {
        LOGERR("checkSum=%llx but not 0\n", checksum);
        return false;
    }

    return true;
}

/*
** gc_copy_metadata copies metadata from in-memory linked list to the @p page
** @td: lfsm_dev_t object
** @h : Garbage Collection structure
** @eu_start : Starting sector of the eu to be read
** @p : Pointer to the page address to which the metadata read is mapped to.
**
** Returns 0
*/
static bool gc_copy_metadata(lfsm_dev_t * td, struct HListGC *h,
                             sector64 eu_start, uint8_t ** ar_page)
{
    struct EUProperty *eu;
    int32_t i;

    eu = pbno_eu_map[(eu_start - eu_log_start) >> EU_BIT_SIZE];
    /** use set metadata page to read from linked list into the page **/
    for (i = 0; i < METADATA_SIZE_IN_PAGES; i++) {
        metabioc_build_page(eu, (onDisk_md_t *) ar_page[i],
                            i == (METADATA_SIZE_IN_PAGES - 1),
                            i * METADATA_ENTRIES_PER_PAGE);
    }

    return meta_chkecksum_on_mem(ar_page);
}

/*
** To read and process the metadata page of the eu
** Counts the number of valid pages in the eu
** Allocates memory for LBN, old_PBN, new_PBN accordingly
** Fills LBN and old_PBN arrays properly 
** Initializes new_PBN to PBN_INVALID
** Memory allocated is freed in the caller
**
** @td : lfsm_dev_t object
** @h : Garbage Collection structure
** @src_eu : the eu which is currently being gc 
** @LBN : Array to hold the LBNs of the valid pages inside the eu
** @old_PBN : Array to hold the old PBNs of the valid pages inside the eu
** @new_PBN : Array to hold the new PBNs to be allocated for the valid pages inside the eu
** @total_valid_blks : The array storing the number of valid blocks of each temperature
** of all the eus picked till now
**
** Returns true:success get metadata; false:empty
*/

static bool gc_process_metadata_of_one_eu(lfsm_dev_t * td, struct HListGC *h,
                                          struct EUProperty *target,
                                          sector64 * LBN, sector64 * old_PBN,
                                          sector64 * new_PBN,
                                          int32_t * valid_pages_in_eu,
                                          sector64 vic_pbno)
{
    struct page *md_page[METADATA_SIZE_IN_PAGES] = { NULL };
    onDisk_md_t *md_entry, *md_ptr;
    int8_t *c_ptr;
    int32_t i, j, k, page_num, retry, ret;
    bool fg_ret;

    fg_ret = false;
    j = 0;
    k = 0;
    page_num = 0;
    ret = 1;

    tf_printk("[TF] gc_process_metadata_of_one_eu valid_pages=%d\n",
              *valid_pages_in_eu);

    LFSM_MUTEX_LOCK(&(h->lock_metadata_io_bio));
    retry = 0;
    while ((ret =
            gc_read_eu_metadata(td, h, target->start_pos, &md_page[0])) <= 0) {
        if (ret == 0) {
            goto l_end;
        }
        LOGWARN("retry gc_read_eu_metadata %d\n", retry);
        if (++retry > 3) {
            goto l_end;
        }
    }

    c_ptr = kmap(md_page[page_num]);
    for (i = 0; i <
         ((target->eu_size -
           METADATA_SIZE_IN_SECTOR) >> SECTORS_PER_FLASH_PAGE_ORDER);
         i++, k++) {
        if (k >= METADATA_ENTRIES_PER_PAGE) {
            kunmap(md_page[page_num]);
            page_num++;
            c_ptr = kmap(md_page[page_num]);
            k = 0;
        }

        md_ptr = (onDisk_md_t *) c_ptr;
        md_entry = &(md_ptr[k]);

        if ((test_bit
             ((i % PAGES_PER_BYTE),
              (void *)&target->bit_map[i >> PAGES_PER_BYTE_ORDER]))
            && ((target->start_pos + (i << SECTORS_PER_FLASH_PAGE_ORDER))
                != vic_pbno)) {
            LBN[j] = md_entry->lbno;
            old_PBN[j] =
                target->start_pos + (i << SECTORS_PER_FLASH_PAGE_ORDER);
            new_PBN[j] = PBN_INVALID;
            if (j % 100 == 0) {
                tf_printk("[TF] %d md_ptr[%d]=%p LBN(%p)=%llu, old_PBN=%llu, "
                          "new_PBN=%llu\n", i, j, md_ptr,
                          &(md_entry->lbno), md_entry->lbno, old_PBN[j],
                          new_PBN[j]);
            }

            j++;
        }
    }
    kunmap(md_page[page_num]);
    *valid_pages_in_eu = j;
    fg_ret = true;

  l_end:
    LFSM_MUTEX_UNLOCK(&(h->lock_metadata_io_bio));
    return fg_ret;
}

int32_t gc_remove_eu_from_lists(struct HListGC *h, struct EUProperty *pTargetEU)
{
    int32_t src_list_id, ret;

    src_list_id = atomic_read(&(pTargetEU->state_list));

    LFSM_RMUTEX_LOCK(&h->whole_lock);
    if (atomic_read(&(pTargetEU->state_list)) == 2) {
        FalseList_delete(h, pTargetEU);
        atomic_set(&(pTargetEU->state_list), -1);
        LOGINFO("[TF]%llu GC FalseList delete\n", pTargetEU->start_pos);
    } else if (atomic_read(&(pTargetEU->state_list)) == 0) {
        LVP_heap_delete(h, pTargetEU);
        pTargetEU->index_in_LVP_Heap = -1;
        atomic_set(&(pTargetEU->state_list), -1);
        dprintk("[TF]%llu GC Heap delete\n", pTargetEU->start_pos);
    } else if (atomic_read(&(pTargetEU->state_list)) == 1) {
        if ((ret = HList_delete(h, pTargetEU)) < 0) {
            src_list_id = ret;
            goto l_end;
        }

        atomic_set(&(pTargetEU->state_list), -1);
        dprintk("[TF]GC Hlist delete\n");
    } else {
        // Weafon: since the foreground process never erase EU, the state_list of a src_eu in gc process should not be -1
        LOGWARN("Unkonwn state_list =%d\n",
                atomic_read(&(pTargetEU->state_list)));
        src_list_id = -1;
        goto l_end;
    }

  l_end:
    LFSM_RMUTEX_UNLOCK(&h->whole_lock);
    return src_list_id;
}

bool gc_eu_reinsert_or_drop(lfsm_dev_t * td, gc_item_t * gc_item)
{
    struct EUProperty *eu;
    struct HListGC *h;

    eu = gc_item->eu;
    h = gc[eu->disk_num];

    LFSM_RMUTEX_LOCK(&h->whole_lock);
    if (gc_item->id_srclist != 2) {
        if (FreeList_insert(h, eu) < 0) {
            LFSM_ASSERT(0);
        }
    } else {
        drop_EU(((eu->start_pos - eu_log_start) >> (SECTORS_PER_SEU_ORDER)));   // drop EU here
    }

    dummy_write(eu);
    LFSM_RMUTEX_UNLOCK(&h->whole_lock);
    return true;
}

static bool gc_need_wearlevel(int32_t id_disk)
{
    /*
     * TODO: Lego 20151109 : EU_COLD_USE_THR is 20, disable it now
     *       We can make this is configuratble
     * if ((gc[id_disk]->max_erase_eu->erase_count -
     * gc[id_disk]->min_erase_eu->erase_count) > EU_COLD_USE_THR) {
     * return true;
     * }
     */
    return false;
}

// return cn picked eu
int32_t gc_pickEU_insert_GClist(struct HListGC *h, struct list_head *beGC_list,
                                int32_t gc_flag)
{
    int32_t cn_picked, total_invalid;

    total_invalid = 0;
    cn_picked = 0;

    LFSM_RMUTEX_LOCK(&h->whole_lock);
    // GC: pick from falselist
    if ((cn_picked += gc_pick_from_falselist(h, beGC_list)) > 0) {
        goto l_end;
    }

    if (!gc_need_wearlevel(h->disk_num)) {  // Normal GC
        cn_picked +=
            gc_pick_from_heap_tree(h, beGC_list, &total_invalid, gc_flag);
        if ((!gc_enough_eu(total_invalid, 1)) && (gc_flag != ALL_INVALID_GC)) {
            cn_picked += gc_pick_from_hlist(h, beGC_list, &total_invalid);
        }

        if (!gc_enough_eu(total_invalid, 1)) {
            if (cn_picked > 0) {
                LOGWARN("Force GC even not enough cn_picked= %d\n", cn_picked);
            }
            goto l_end;
        }
    } else {                    // GC a cold EU.
        LOGINFO("TF: force GC for wear-leveling\n");
        if ((cn_picked = gc_pick_less_erase_EU(h, beGC_list)) == 0) {
            goto l_end;
        }
    }

  l_end:
    LFSM_RMUTEX_UNLOCK(&h->whole_lock);
    return cn_picked;
}

/*
** To read and process the metadata page of the eu
** Counts the number of valid pages in the eu
** Allocates memory for LBN, old_PBN, new_PBN accordingly
** Fills LBN and old_PBN arrays properly
** Initializes new_PBN to PBN_INVALID
** Memory allocated is freed in the caller
**
** @td : lfsm_dev_t object
** @h : Garbage Collection structure
** @src_eu : the eu which is currently being gc
** @LBN : Array to hold the LBNs of the valid pages inside the eu
** @old_PBN : Array to hold the old PBNs of the valid pages inside the eu
** @new_PBN : Array to hold the new PBNs to be allocated for the valid pages inside the eu
** @total_valid_blks : The array storing the number of valid blocks of each temperature
** of all the eus picked till now
**
** Returns void
*/
bool gc_process_metadata_from_mem(lfsm_dev_t * td, struct HListGC *h,
                                  struct EUProperty *target, sector64 * LBN,
                                  sector64 * old_PBN, sector64 * new_PBN,
                                  int32_t * valid_pages_in_eu,
                                  sector64 vic_pbno)
{
    uint8_t *md_page[METADATA_SIZE_IN_PAGES] = { NULL };
    onDisk_md_t *md_entry, *md_ptr;
    int8_t *c_ptr;
    int32_t i, j, num_valid_pages, k, page_num;
    bool ret;

    j = 0;
    ret = false;
    num_valid_pages = *valid_pages_in_eu;
    k = 0;
    page_num = 0;

    memset(md_page, 0, sizeof(uint8_t *) * METADATA_SIZE_IN_PAGES);
    for (i = 0; i < METADATA_SIZE_IN_PAGES; i++) {
        if (NULL == (md_page[i] = (uint8_t *)
                     track_kmalloc(PAGE_SIZE, GFP_KERNEL, M_UNKNOWN))) {
            goto l_end;
        }
    }

    if (!gc_copy_metadata(td, h, target->start_pos, &md_page[0])) {
        LOGERR("fail to copy metadata %lld\n", target->start_pos);
        goto l_end;
    }

    c_ptr = md_page[page_num];
    for (i = 0, j = 0; (i < ((target->eu_size - METADATA_SIZE_IN_SECTOR) >>
                             SECTORS_PER_FLASH_PAGE_ORDER))
         && (j < num_valid_pages); i++, k++) {
        if (k >= METADATA_ENTRIES_PER_PAGE) {
            page_num++;
            c_ptr = md_page[page_num];
            k = 0;
        }

        md_ptr = (onDisk_md_t *) c_ptr;
        md_entry = &md_ptr[k];
        if ((test_bit
             ((i % PAGES_PER_BYTE),
              (void *)&target->bit_map[i >> PAGES_PER_BYTE_ORDER]))
            && ((target->start_pos + (i << SECTORS_PER_FLASH_PAGE_ORDER)) !=
                vic_pbno)) {
            LBN[j] = md_entry->lbno;
            old_PBN[j] =
                target->start_pos + (i << SECTORS_PER_FLASH_PAGE_ORDER);
            new_PBN[j] = PBN_INVALID;
            tf_printk("mem: LBN(%d)=%llu, old_PBN=%llu \n", j, LBN[j],
                      old_PBN[j]);
            j++;
        }
    }
    // Value will change sometimes before the end of the function in case of multithreaded io
    *valid_pages_in_eu = j;
    ret = true;

    mdnode_free(&target->act_metadata, td->pMdpool);
  l_end:
    for (i = 0; i < METADATA_SIZE_IN_PAGES; i++) {
        if (md_page[i]) {
            track_kfree(md_page[i], PAGE_SIZE, M_UNKNOWN);
        }
    }
    return ret;
}

bool gc_process_lpninfo(lfsm_dev_t * td, gc_item_t * gc_item,
                        int32_t * valid_pages_in_eu)
{
    struct EUProperty *eu;
    struct HListGC *h;
    sector64 *LBN, *old_PBN, *new_PBN;

    eu = gc_item->eu;
    h = gc[eu->disk_num];
    LBN = gc_item->gc_addr.lbn;
    old_PBN = gc_item->gc_addr.old_pbn;
    new_PBN = gc_item->gc_addr.new_pbn;

    if (NULL != (eu->act_metadata.mdnode)) {
        return gc_process_metadata_from_mem(td, h, eu, LBN, old_PBN, new_PBN,
                                            valid_pages_in_eu, -1);
    } else {
        if ((eu->fg_metadata_ondisk == false) ||
            gc_process_metadata_of_one_eu(td, h, eu, LBN, old_PBN, new_PBN,
                                          valid_pages_in_eu, -1) == false) {
            //lfsm_write_pause();
            return false;
        }
    }

    return true;
}

bool gc_get_bioc_init(lfsm_dev_t * td, gc_item_t * gc_item, int32_t cn_valid_pg)
{
    struct bio bio_null;
    struct bio_container **ar_bioc;
    sector64 *LBN;
    int32_t i;

    ar_bioc = gc_item->ar_pbioc;
    LBN = gc_item->gc_addr.lbn;

    for (i = 0; i < cn_valid_pg; i++) {
        bio_null.bi_iter.bi_sector = 0;
        if (LBN[i] >= DBMTE_NUM * td->ltop_bmt->cn_ppq) {
            LOGERR("wrong lpn %llu \n", LBN[i]);
            return false;
        }

        if (NULL == (ar_bioc[i] = bioc_gc_get_and_wait(td, &bio_null, LBN[i]))) {
            return false;
        }
    }

    return true;
}

static bool gc_addr_info_alloc(gcadd_info_t * gc_addr, int32_t cn_valid_pg)
{
    if (NULL == (gc_addr->lbn = (sector64 *)
                 track_kmalloc(cn_valid_pg * sizeof(sector64), GFP_KERNEL,
                               M_GC))) {
        goto l_fail;
    }

    if (NULL == (gc_addr->old_pbn = (sector64 *)
                 track_kmalloc(cn_valid_pg * sizeof(sector64), GFP_KERNEL,
                               M_GC))) {
        goto l_fail;
    }

    if (NULL == (gc_addr->new_pbn = (sector64 *)
                 track_kmalloc(cn_valid_pg * sizeof(sector64), GFP_KERNEL,
                               M_GC))) {
        goto l_fail;
    }

    return true;

  l_fail:
    gc_addr_info_free(gc_addr, cn_valid_pg);
    return false;
}

void gc_addr_info_free(gcadd_info_t * gc_addr, int32_t cn_valid_pg)
{
    if (gc_addr->lbn) {
        track_kfree(gc_addr->lbn, cn_valid_pg * sizeof(sector64), M_GC);
    }

    if (gc_addr->old_pbn) {
        track_kfree(gc_addr->old_pbn, cn_valid_pg * sizeof(sector64), M_GC);
    }

    if (gc_addr->new_pbn) {
        track_kfree(gc_addr->new_pbn, cn_valid_pg * sizeof(sector64), M_GC);
    }
}

bool gc_alloc_mapping_valid(gc_item_t * gc_item)
{
    struct EUProperty *target;
    int32_t i, num_valid_pages;

    target = gc_item->eu;
    num_valid_pages = 0;

    for (i = 0; i < ((target->eu_size - METADATA_SIZE_IN_SECTOR) >>
                     SECTORS_PER_FLASH_PAGE_ORDER); i++) {
        if (test_bit
            ((i % PAGES_PER_BYTE),
             (void *)&target->bit_map[i >> PAGES_PER_BYTE_ORDER])) {
            num_valid_pages++;
        }
    }
//    LOGINFO("eu %lld id_hpeu %d valid %d\n",target->start_pos,target->id_hpeu,num_valid_pages);
    LFSM_ASSERT2(num_valid_pages <=
                 (FPAGE_PER_SEU - METADATA_SIZE_IN_FLASH_PAGES),
                 "num_valid_pages %d\n", num_valid_pages);

    if (!gc_addr_info_alloc(&gc_item->gc_addr, num_valid_pages)) {
        return false;
    }

    gc_item->cn_valid_pg = num_valid_pages;
    return true;
}

#ifdef  HLIST_GC_WHOLE_LOCK_DEBUG
int32_t show_hlist_lock_status(int8_t * buffer, int32_t buf_size)
{
    struct HListGC *h;
    int32_t diskID, len;

    len = 0;
    len +=
        sprintf(buffer + len, "HList_GC whole lock status (%d): \n",
                lfsmdev.cn_ssds);

    for (diskID = 0; diskID < lfsmdev.cn_ssds; diskID++) {
        h = gc[diskID];
        if (strlen(h->lock_fn_name) == 0) {
            len += sprintf(buffer + len, "%d %d\n", diskID, h->access_cnt);
        } else {
            len +=
                sprintf(buffer + len, "%d %d %s", diskID, h->access_cnt,
                        h->lock_fn_name);
        }
    }

    return len;
}
#endif
