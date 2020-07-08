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

#ifndef STORAGE_CORE_LFSM_PERF_MONITOR_SSD_PERF_MON_H_
#define STORAGE_CORE_LFSM_PERF_MONITOR_SSD_PERF_MON_H_

//#define ENABLE_SSD_PERF_MON

extern void dereg_ssd_perf_monitor(int32_t disk_id);
extern void reg_ssd_perf_monitor(int32_t disk_id, int8_t * diskName);
extern void log_ssd_perf_time(int32_t disk_id, int32_t rw, int64_t time_s,
                              int64_t time_e);
extern int32_t show_ssd_perf_monitor(int8_t * buffer, int32_t buff_size);
extern void exec_ssd_perfMon_cmdstr(int8_t * buffer, int32_t buff_size);
extern void init_ssd_perf_monitor(int32_t total_ssds);

#ifdef ENABLE_SSD_PERF_MON
#define LOG_SSD_PERF(id, iotype, t_s, t_e) log_ssd_perf_time(id, iotype, t_s, t_e)
#else
#define LOG_SSD_PERF(id, iotype, t_s, t_e) {}
#endif

#endif /* STORAGE_CORE_LFSM_PERF_MONITOR_SSD_PERF_MON_H_ */
