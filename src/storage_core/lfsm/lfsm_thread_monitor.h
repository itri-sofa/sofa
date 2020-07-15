/*
 * SPDX-License-Identifier: GPL-2.0-or-later OR Apache-2.0
 * Copyright (C) 2015-2020 Industrial Technology Research Institute
 *
 *
 *
 *  
 */

#ifndef STORAGE_CORE_LFSM_LFSM_THREAD_MONITOR_H_
#define STORAGE_CORE_LFSM_LFSM_THREAD_MONITOR_H_

extern void dereg_thread_monitor(pid_t thdID);
extern void reg_thread_monitor(pid_t thdID, int8_t * thdName);
extern void log_thread_exec_step(pid_t thdID, int8_t * msg);
extern int32_t show_thread_monitor(int8_t * buffer, int32_t buff_size);
extern void init_thread_monitor(void);
extern void rel_thread_monitor(void);
extern int32_t is_lfsm_bg_thd(pid_t thdID);

#define LOG_THD_EXEC_STEP(x, y)   \
    do { \
        log_thread_exec_step(x, y); \
    } while (0)

#endif /* STORAGE_CORE_LFSM_LFSM_THREAD_MONITOR_H_ */
