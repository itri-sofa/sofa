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

#ifndef STORAGE_CORE_LFSM_PERF_MONITOR_THD_PERF_MON_PRIVATE_H_
#define STORAGE_CORE_LFSM_PERF_MONITOR_THD_PERF_MON_PRIVATE_H_

#define THD_TBL_HASH_SIZE   1024
#define THD_TBL_MODVAL  (THD_TBL_HASH_SIZE - 1)

#define LEN_THD_NAME      64
#define MAX_MONITOR_STAGE   8
typedef struct thd_perfMon_item {
    int8_t thd_name[LEN_THD_NAME];
    struct list_head litem_thd_perfMon;
    struct list_head litem_traversal;   //list item for add to lhead (list head)
    pid_t thd_id;
    thd_type_t thread_type;
    uint64_t cnt_proc_w;
    uint64_t time_proc_w;       //unit: jiffies
    uint64_t cnt_proc_r;        //unit: jiffies
    uint64_t time_proc_r;
    uint64_t stage_time[MAX_MONITOR_STAGE];
    uint64_t stage_count[MAX_MONITOR_STAGE];
} thdpm_item_t;

typedef struct thd_perfMon_tbl {
    struct list_head thd_perfMon_tbl[THD_TBL_HASH_SIZE];
    rwlock_t thd_perfMon_tbllock;
    int32_t thd_perfMon_cnt_items;
    struct list_head lhead_perfMon_traversal[LFSM_THD_PERFMON_END];
} THD_PMon_tbl_t;

THD_PMon_tbl_t thdPerfMon_tbl;

int8_t *thdType_name[] = {
    "lfsm_iod",
    "lfsm_bh",
    "lfsm_gc",
    "unknown",
};

#endif /* STORAGE_CORE_LFSM_PERF_MONITOR_THD_PERF_MON_PRIVATE_H_ */
