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

#ifndef LFSMPRIVATE_H_
#define LFSMPRIVATE_H_

int32_t lfsm_resetData;
int32_t lfsm_readonly;
int32_t ulfb_offset;

lfsm_dev_t lfsmdev;
struct HListGC **gc;
struct EUProperty **pbno_eu_map;
sector64 eu_log_start;
//extern void refrigerator(void);
//atomic_t ioreq_counter;

lfsm_dev_t *gplfsm;
EXPORT_SYMBOL(gplfsm);

int32_t disk_change = 0;
//spinlock_t dump_lock= SPIN_LOCK_UNLOCKED;
int32_t assert_flag = 0;        /* control lfsm whether pause */
atomic_t cn_paused_thread;
atomic_t wio_cn[TEMP_LAYER_NUM];

int32_t lfsm_state_bg_thread = 0;
bool fg_gc_continue = false;    //weafon give up

int32_t lfsm_run_mode = 0;
int32_t cn_erase_front = 0;
int32_t cn_erase_back = 0;

#endif /* LFSMPRIVATE_H_ */
