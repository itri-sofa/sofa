/*
 * SPDX-License-Identifier: GPL-2.0-or-later OR Apache-2.0
 * Copyright (C) 2015-2020 Industrial Technology Research Institute
 *
 *
 *
 *  
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
