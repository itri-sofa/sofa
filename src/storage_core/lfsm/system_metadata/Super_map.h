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
