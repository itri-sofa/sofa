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
