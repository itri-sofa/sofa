/*
 * SPDX-License-Identifier: GPL-2.0-or-later OR Apache-2.0
 * Copyright (C) 2015-2020 Industrial Technology Research Institute
 *
 *
 *
 *  
 */
#ifndef _SUPER_MAP_H
#define _SUPER_MAP_H

#define SUPERMAP_SEU_NUM         1  //only support 1 in current construction
#define SUPERMAP_SIZE            (SUPERMAP_SEU_NUM*(1<<(SECTORS_PER_SEU_ORDER)))

#define TAG_SUPERMAP "LFSM-SBP" // weafon: 8 char limitation

/*
 * On disk representation of super map
 */
typedef struct super_map_entry {
    int8_t magicword[8];
    sector64 dedicated_eu;      /* logical sector position of dedicated eu */
    sector64 dedicated_eu_copy; /* logical sector position of backup dedicated eu */
    uint64_t seq;               /* seq number of log record */
    enum edev_state signature;  /* lfsm state */
    int32_t logic_disk_id;      /* logical disk id in disk group */
} super_map_entry_t;

extern void super_map_logging_append(lfsm_dev_t * td, diskgrp_t * pgrp,
                                     int32_t cn_ssd_orig);
extern void super_map_logging_all(lfsm_dev_t * td, diskgrp_t * pgrp);
extern int32_t super_map_get_last_entry(int32_t disk_num, diskgrp_t * pgrp);
extern int32_t super_map_set_dedicated_map(lfsm_dev_t * td,
                                           lfsm_subdev_t * ar_subdev);

extern sector64 find_max_seq_entry(diskgrp_t * pgrp, super_map_entry_t * store_rec, int32_t disk_num);  // returns frontier
extern void super_map_initEU(diskgrp_t * pgrp, struct HListGC *h,
                             sector64 log_start, int32_t free_number,
                             int32_t disk_num);

//extern bool supermap_frontier_check(lfsm_dev_t *td);
extern int32_t super_map_get_primary(lfsm_dev_t * td);
extern bool super_map_rescue(lfsm_dev_t * td, diskgrp_t * pgrp);
extern int32_t super_map_get_diskid(struct block_device *bdev);

extern int32_t super_map_allocEU(diskgrp_t * pgrp, int32_t idx_disk);
extern void super_map_freeEU(diskgrp_t * pgrp);
extern void supermap_dump(lfsm_subdev_t * ar_subdev, int32_t disk_num);
extern bool super_map_init(lfsm_dev_t * td, int32_t i,
                           lfsm_subdev_t * ar_subdev);
extern void super_map_destroy(lfsm_dev_t * td, int32_t i,
                              lfsm_subdev_t * ar_subdev);
#endif
