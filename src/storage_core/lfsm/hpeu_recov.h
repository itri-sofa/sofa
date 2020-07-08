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

#ifndef _HPEU_RECOV_
#define _HPEU_RECOV_

#define rec_printk(...)  printk("[PID=%d] ",current->pid);printk(__VA_ARGS__)
//#define rec_printk(...)

struct lfsm_dev_struct;

typedef struct HPEUrecov_entry {
    struct list_head link;
    int32_t id_hpeu;
} HPEUrecov_entry_t;

typedef struct HPEUrecov_param {
    struct list_head lst_id_hpeu;
    int32_t idx_start;
    int32_t idx_end;
} HPEUrecov_param_t;

//struct BMT_update_record;

extern bool HPEUrecov_crash_recovery(struct lfsm_dev_struct *td);

#endif
