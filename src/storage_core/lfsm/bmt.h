/*
 * SPDX-License-Identifier: GPL-2.0-or-later OR Apache-2.0
 * Copyright (C) 2015-2020 Industrial Technology Research Institute
 *
 *
 *
 *  
 */
/*
** This is the header file for Bmt.h file
** Only the function prototypes declared below are exported to be used by other files
*/
#ifndef    BMT_H
#define    BMT_H

#define    PAGES_PER_BYTE        8
#define    PAGES_PER_BYTE_ORDER    3

#define SZ_BVEC (DBMTE_NUM >> PAGES_PER_BYTE_ORDER) // 1 byte = 8 bits
/*
** Per-page-queue structure
*/
typedef struct sconflict {
    uint8_t bvec[SZ_BVEC];      //how many lpn (8 bytes) in a ppq bucket and 1 bite represent 1 lpn
    struct list_head confl_list;    //conflict list
    spinlock_t confl_lock;
} sconflict_t;

typedef struct _metadata_lpn2ppn_map_item {
// if it is a pointer to Direct BMT array: it is the point to D_BMT_A,
// otherwise, it is a pointer to link list of A_BMT_E
    dbmtE_bucket_t *pDbmtaUnit;
    struct list_head LRU;
    uint32_t non_pending_num;   //non_pending_num: number ppn not yet be flushed
    bool fg_pending;            //true: this bmt is flushing
    int32_t index;
    struct mutex ppq_lock;
    sector64 addr_ondisk;       // THE absolute on-disk sector address of the ppq
    int32_t ondisk_logi_page;
    int32_t ondisk_logi_eu;
    int32_t cn_bioc_pins;
    sconflict_t conflict;
} MD_L2P_item_t;

/*
** BMT Structure
*/
typedef struct _metadata_lpn2ppn_map_table {
    struct list_head cache_LRU_head;
    spinlock_t LRU_spinlock;
    MD_L2P_item_t *per_page_queue;
    int32_t cn_ppq;             /* num entry in LPN-PPN mapping array */
    struct EUProperty **BMT_EU;
    struct EUProperty **BMT_EU_backup;
    struct EUProperty *BMT_EU_gc;
    struct EUProperty *BMT_EU_backup_gc;
    int32_t BMT_num_EUs;        /* # of EUs required for LPN-PPN mapping and validate bitmap */
    int32_t pagebmt_cn_used;
    int8_t *AR_PAGEBMT_isValidSegment;

    atomic64_t cn_lookup_query;
    atomic64_t cn_valid_page_ssd;
    atomic64_t cn_dirty_page_ssd;
    atomic_t cn_lookup_missing;

    struct mutex cmt_lock;      //tf add, 20120209; weafon disagree
    bool fg_gceu_dirty;
    struct lock_class_key *ar_key;
} MD_L2P_table_t;

extern int32_t PbnoBoundaryCheck(lfsm_dev_t * td, sector64 pbno_over_disk);
extern int32_t update_ppq_and_bitmap(lfsm_dev_t * td,
                                     struct bio_container *pBioc);
extern int32_t per_page_queue_update(lfsm_dev_t * td, sector64 * lbno,
                                     int32_t size, sector64 * pbno,
                                     enum EDBMTA_OP_DIRTYBIT op_dirty);
extern int32_t BMT_setup_for_scale_up(lfsm_dev_t * td, MD_L2P_table_t * bmt,
                                      MD_L2P_table_t * bmt_orig,
                                      int32_t BMT_num_EUs,
                                      int32_t BMT_num_EUs_orig,
                                      int32_t cn_ssd_orig);

extern bool generate_BMT(lfsm_dev_t * td, int32_t BMT_num_EUs,
                         struct diskgrp *pgrp);

extern bool create_BMT(MD_L2P_table_t **, sector64);
extern void BMT_destroy(MD_L2P_table_t * bmt);
extern bool generate_bitmap_and_validnum(lfsm_dev_t * td,
                                         struct EUProperty **pbno_eu_map);

extern int32_t bmt_lookup(lfsm_dev_t * td, MD_L2P_table_t * bmt, sector64 lpn,
                          bool incr_acc_count, int32_t cn_pin_ppq,
                          struct D_BMT_E *ret_dbmta);

extern int32_t clear_PBN_bitmap_validcount(sector64 pbno);
extern int32_t bmt_dump(MD_L2P_table_t * bmt, int8_t * buf);
extern bool BMT_rescue(lfsm_dev_t * td, MD_L2P_table_t * bmt);
extern void reset_bitmap_ppq(sector64 pbno_clear, sector64 pbno_set,
                             sector64 lpn, lfsm_dev_t * td);
extern int32_t ppq_alloc_reload(MD_L2P_item_t * ppq, lfsm_dev_t * td,
                                MD_L2P_table_t * bmt, int32_t idx_ppq);
extern int32_t test_pbno_bitmap(sector64 pbno);
extern int32_t bmt_cn_fpage_valid_bitmap(MD_L2P_table_t * bmt);
extern int32_t bmt_sz_ar_ValidSegment(MD_L2P_table_t * bmt);
#endif
