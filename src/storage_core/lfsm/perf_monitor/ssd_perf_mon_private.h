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
