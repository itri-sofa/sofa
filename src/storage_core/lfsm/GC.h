/*
 * SPDX-License-Identifier: GPL-2.0-or-later OR Apache-2.0
 * Copyright (C) 2015-2020 Industrial Technology Research Institute
 *
 *
 *
 *  
 */
/*
** This is the header file for GC.c file
** Only the function prototypes declared below are exported to be used by other files
*/

#ifndef    GC_H
#define    GC_H

#define FALSE_GC 2
#define ALL_INVALID_GC 3
#define NORMAL_GC 1
#define GC_OFF 0

//#define RESERVED_RATIO 2 //1/8
//#define GC_UP_THRESHOLD_FACTOR    (1<<RESERVED_RATIO)
//#define GC_DN_THRESHOLD_FACTOR    ((1<<RESERVED_RATIO)*2)

#define TP_COPY_IO    4         //after a long talk (1hr), weafon give up because the code will be license
#define TP_FLUSH_IO 3
#define TP_GC_IO 2
#define TP_PARTIAL_IO 1
#define TP_NORMAL_IO 0
#define DONTCARE_TYPE -1

#define PBN_INVALID 0
#define EU_BIT_SIZE    (SECTORS_PER_SEU_ORDER)
#define    DUMMY_SIZE_IN_SECTOR    0
#define    DUMMY_SIZE_IN_PAGE    (DUMMY_SIZE_IN_SECTOR>>SECTORS_PER_PAGE_ORDER)

#define CORRECTION(SIZE) ((SIZE%PAGE_SIZE)?1:0)

#define CORRECTION_FLASH_PAGE(SIZE)   ((SIZE%FLASH_PAGE_SIZE)?1:0)

typedef struct on_disk_metadata {
    sector64 lbno;
} onDisk_md_t;

typedef struct on_disk_metadata_list {
    struct list_head list;
    struct on_disk_metadata mdata;
} onDisk_md_list_t;

#define METADATA_ENTRIES_PER_PAGE (PAGE_SIZE/(sizeof(onDisk_md_t)))
#define METADATA_SIZE_IN_BYTES (FPAGE_PER_SEU * (sizeof(onDisk_md_t)))
#define METADATA_SIZE_IN_PAGES LFSM_CEIL_DIV(METADATA_SIZE_IN_BYTES,PAGE_SIZE)
#define METADATA_SIZE_IN_FLASH_PAGES LFSM_CEIL_DIV(METADATA_SIZE_IN_BYTES,FLASH_PAGE_SIZE)
#define METADATA_SIZE_IN_HARD_PAGES LFSM_CEIL_DIV(METADATA_SIZE_IN_BYTES,HARD_PAGE_SIZE)

#define METADATA_SIZE_IN_SECTOR ((METADATA_SIZE_IN_PAGES)*(SECTORS_PER_PAGE))   // 2^Nk-safe
#define    READ_EU_ONE_SHOT_SIZE                MAX_FLASH_PAGE_NUM
#define METADATA_BIO_VEC_SIZE            METADATA_SIZE_IN_PAGES

#define DATA_FPAGE_IN_SEU        (FPAGE_PER_SEU-METADATA_SIZE_IN_FLASH_PAGES)
#define DATA_SECTOR_IN_SEU        (SECTORS_PER_SEU-METADATA_SIZE_IN_SECTOR)

#define IS_ALL_VALID(h,pEU)    (atomic_read(&pEU->eu_valid_num)==(h->eu_size -METADATA_SIZE_IN_SECTOR))
#define VALID_OVER_THRESHOLD(h,pEU,order)    (atomic_read(&pEU->eu_valid_num)>(h->eu_size-METADATA_SIZE_IN_SECTOR)>>order)

#define COLD_THRESHOLD 10
//#define HLIST_CAPACITY    100
#define HLIST_CAPACITY_RATIO 10
struct HListGC;

typedef struct gc_addr_info {
    sector64 *lbn;
    sector64 *old_pbn;
    sector64 *new_pbn;
} gcadd_info_t;

typedef struct gc_item {
    struct EUProperty *eu;
    gcadd_info_t gc_addr;
    struct bio_container **ar_pbioc;
    int32_t id_srclist;
    int32_t cn_valid_pg;
} gc_item_t;

typedef struct EU_to_be_GC_item {
    struct list_head next;
    gc_item_t gc_item;
} EU2GC_item_t;

#include "lfsm_core.h"

extern void gc_end_bio_read_io(struct bio *bio);
extern int32_t gc_read_eu_metadata(lfsm_dev_t * td, struct HListGC *h,
                                   sector64 pos, struct page **p);

bool gc_collect_valid_blks(lfsm_dev_t * td, struct HListGC *h, int32_t gc_flag);
extern int32_t gc_threshold(struct HListGC *h, int32_t);

bool meta_checksum_on_spare(struct page **ar_mpage);

extern bool gc_eu_reinsert_or_drop(lfsm_dev_t * td, gc_item_t * gc_item);
extern bool gc_alloc_mapping_valid(gc_item_t * gc_item);
extern bool gc_process_lpninfo(lfsm_dev_t * td, gc_item_t * gc_item,
                               int32_t * valid_pages_in_eu);
extern bool gc_get_bioc_init(lfsm_dev_t * td, gc_item_t * gc_item,
                             int32_t cn_valid_pg);
extern int32_t gc_remove_eu_from_lists(struct HListGC *h,
                                       struct EUProperty *pTargetEU);
bool gc_copy_validpg(lfsm_dev_t * td, gc_item_t * gc_item, int32_t cn_valid_pg);

extern void gc_addr_info_free(gcadd_info_t * gc_addr, int32_t cn_valid_pg);
extern void gc_item_init(gc_item_t * gc_item, struct EUProperty *pick_eu);
extern int32_t gc_pickEU_insert_GClist(struct HListGC *h,
                                       struct list_head *beGC_list,
                                       int32_t gc_flag);
int32_t gc_data_write(lfsm_dev_t * td, struct bio_container *data_bio_c,
                      sector64 * lbno, bool isPipeline);
int32_t gc_metadata_write(lfsm_dev_t * td, struct bio_container *metadata_bio_c,
                          struct EUProperty *eu_dest);
int32_t bioc_gc_submit(lfsm_dev_t * td, struct HListGC *h,
                       struct bio_container *bio_c, int32_t cn_fpages,
                       sector64 * lbno, sector64 * pbno,
                       sector64 pending_metadata_pbno);
#ifdef HLIST_GC_WHOLE_LOCK_DEBUG
extern int32_t show_hlist_lock_status(int8_t * buffer, int32_t buf_size);
#endif
#endif
