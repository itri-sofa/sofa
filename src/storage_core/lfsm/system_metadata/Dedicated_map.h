/*
 * SPDX-License-Identifier: GPL-2.0-or-later OR Apache-2.0
 * Copyright (C) 2015-2020 Industrial Technology Research Institute
 *
 *
 *
 *  
 */
#ifndef NEW_DEDICATED_MAP_H
#define NEW_DEDICATED_MAP_H
#define MAX_BMT_EUS    600
#define MAX_ERASE_EUS     lfsmdev.cn_ssds

enum elfsm_state { sys_fresh = 0x3B, sys_loaded = 0x5A, sys_unloaded = 0xA5 };

struct lfsm_dev_struct;
struct SNextPrepare;

typedef struct SDedicatedMap {
    struct EUProperty *eu_main; //*dedicated_eu;
    struct EUProperty *eu_backup;   //*dedicated_eu_copy;
    struct bio *bio;            /* prealloc bio for read/write dedicated map */
    int32_t fg_io_result;       /* result of r/w dedicated map */
    wait_queue_head_t wq;       /* pause thread when r/w dedicated map */
    struct SNextPrepare next;   /* spare EU when current EU full */
    struct mutex sdd_lock;      /* protection when update ddmap */
    int32_t cn_cycle;           /* num of ddmap EU changes */
} SDedicatedMap_t;

/** Stores the start address of the EU which has erase counts along 
    with the disk number.
*/
typedef struct erase_head {
    int8_t magic[4];
    int32_t disk_num;
    sector64 start_pos;
    sector64 frontier;
} erase_head_t;

typedef struct sddmap_ondisk_header {
    int32_t page_number;        /*use the EU log frontier as record number (page unit) */
    enum elfsm_state signature; /* system state */
    sector64 update_log_head;   /* start pos of EU of update log (sector base) */
    sector64 update_log_backup; /* start pos of backup EU of update log (sector base) */
    sector64 BMT_map_gc;        /* start pos of EU of gc (sector base) */
    sector64 BMT_redundant_map_gc;  /* start pos of backup EU of gc (sector base) */
    sector64 logical_capacity;  /* total space of lfsm (system EU and ECC space not in counting */
    sector64 HPEU_map;          /* start pos of EU of hpeu table (sector base) */
    sector64 HPEU_redundant_map;    /* start pos of backup EU of hpeu table (sector base) */
    int32_t HPEU_idx_next;      /* log frontier of hpeu table (sector base) */
    int16_t off_dev_state;      /* offset of state ( = start pos of ddmap + size of ddamp) unit: uint32_t */
    int16_t off_BMT_map;        /* offset of bmt EU number unit: uint32_t */
    int16_t off_erase_map;      /* offset of erase map EU number unit: uint32_t */
    int16_t off_ext_ecc;        /* offset of external ecc info */
    int16_t off_eu_mlogger;     /* offset of mlogger EU number unit: uint32_t */
} sddmap_disk_hder_t;

//-------------public------------//

extern sector64 ddmap_get_BMT_map(sddmap_disk_hder_t * pheader, int32_t idx);
extern sector64 ddmap_get_BMT_redundant_map(sddmap_disk_hder_t * pheader,
                                            int32_t idx, int32_t cn_backup);
extern bool dedicated_map_init(struct lfsm_dev_struct *td);
extern void dedicated_map_destroy(struct lfsm_dev_struct *td);
extern bool dedicated_map_get_last_entry(SDedicatedMap_t * pDeMap,
                                         sddmap_disk_hder_t * old_rec,
                                         struct lfsm_dev_struct *td);
extern int32_t dedicated_map_logging(struct lfsm_dev_struct *td);
extern int32_t dedicated_map_logging_state(struct lfsm_dev_struct *td,
                                           enum elfsm_state signature);
extern int32_t dedicated_map_eunext_init(SDedicatedMap_t * pDeMap);
extern void dedicated_map_eu_init(SDedicatedMap_t * pDeMap);
extern void dedicated_map_change_next(SDedicatedMap_t * pDeMap);
extern bool dedicated_map_rescue(struct lfsm_dev_struct *td,
                                 SDedicatedMap_t * pDeMap);
extern void dedicated_map_set_dev_state(sddmap_disk_hder_t * pheader,
                                        struct diskgrp *pgrp);
extern erase_head_t *ddmap_get_erase_map(sddmap_disk_hder_t * pheader,
                                         int32_t idx_erasemap);
extern struct external_ecc_head *ddmap_get_ext_ecc(sddmap_disk_hder_t * pheader,
                                                   int32_t idx);
extern int32_t ddmap_get_ondisk_size(int32_t cn_disks);
extern void ddmap_header_dump(sddmap_disk_hder_t * prec);
extern sector64 *ddmap_get_eu_mlogger(sddmap_disk_hder_t * pheader);

extern bool dedicated_map_load(SDedicatedMap_t * pDeMap, int32_t dump_item,
                               int32_t lfsm_cn_ssds);
extern bool ddmap_dump(struct lfsm_dev_struct *td, int8_t * data_main,
                       int8_t * data_backup);
#endif
