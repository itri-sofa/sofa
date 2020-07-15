/*
 * SPDX-License-Identifier: GPL-2.0-or-later OR Apache-2.0
 * Copyright (C) 2015-2020 Industrial Technology Research Institute
 *
 *
 *
 *  
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
