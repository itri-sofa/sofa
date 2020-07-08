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

#ifndef STORAGE_CORE_LFSM_PERF_MONITOR_THD_PERF_MON_H_
#define STORAGE_CORE_LFSM_PERF_MONITOR_THD_PERF_MON_H_

//#define ENABLE_THD_PERF_MON

typedef enum {
    LFSM_IO_THD,
    LFSM_IO_BH_THD,
    LFSM_GC_THD,
    LFSM_THD_PERFMON_END,
} thd_type_t;

extern void dereg_thd_perf_monitor(pid_t thdID);
extern void reg_thd_perf_monitor(pid_t thdID, thd_type_t thdType,
                                 int8_t * thdName);
extern void log_thd_perf_time(pid_t thdID, int32_t rw, int64_t time_s,
                              int64_t time_e);
extern void log_thd_perf_stage_time(pid_t thdID, int32_t stage, int64_t time_s,
                                    int64_t time_e);
extern int32_t show_thd_perf_monitor(int8_t * buffer, int32_t buff_size);
extern void exec_thd_perfMon_cmdstr(int8_t * buffer, int32_t buff_size);
extern void init_thd_perf_monitor(void);
extern void rel_thd_perf_monitor(void);

#ifdef ENABLE_THD_PERF_MON
#define LOG_THD_PERF(id, iotype, t_s, t_e) log_thd_perf_time(id, iotype, t_s, t_e)
#define LOG_THD_PERF_STAGE_T(id, stage, t_s, t_e) log_thd_perf_stage_time(id, stage, t_s, t_e)
#else
#define LOG_THD_PERF(id, iotype, t_s, t_e) {}
#define LOG_THD_PERF_STAGE_T(id, stage, t_s, t_e) {}
#endif

#endif /* STORAGE_CORE_LFSM_PERF_MONITOR_THD_PERF_MON_H_ */
