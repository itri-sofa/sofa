/*
 * SPDX-License-Identifier: GPL-2.0-or-later OR Apache-2.0
 * Copyright (C) 2015-2020 Industrial Technology Research Institute
 *
 *
 *
 *  
 */
#ifndef STORAGE_CORE_LFSM_PERF_MONITOR_SSD_PERF_MON_PRIVATE_H_
#define STORAGE_CORE_LFSM_PERF_MONITOR_SSD_PERF_MON_PRIVATE_H_

#define LEN_DISK_NAME      64
#define MAX_NUM_DISK       128

typedef struct disk_perfMon_item {
    int8_t disk_name[LEN_DISK_NAME];
    int32_t disk_id;
    uint64_t cnt_proc_w;
    uint64_t time_proc_w;       //unit: jiffies
    uint64_t cnt_proc_r;
    uint64_t time_proc_r;       //unit: jiffies
} diskpm_item_t;

diskpm_item_t diskPM_tbl[MAX_NUM_DISK];
int32_t total_ssds_pm;
#endif /* STORAGE_CORE_LFSM_PERF_MONITOR_SSD_PERF_MON_PRIVATE_H_ */
