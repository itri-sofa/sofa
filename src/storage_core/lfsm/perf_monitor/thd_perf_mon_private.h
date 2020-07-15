/*
 * SPDX-License-Identifier: GPL-2.0-or-later OR Apache-2.0
 * Copyright (C) 2015-2020 Industrial Technology Research Institute
 *
 *
 *
 *  
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
