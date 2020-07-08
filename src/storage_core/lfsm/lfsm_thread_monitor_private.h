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

#ifndef STORAGE_CORE_LFSM_LFSM_THREAD_MONITOR_PRIVATE_H_
#define STORAGE_CORE_LFSM_LFSM_THREAD_MONITOR_PRIVATE_H_

#define LEN_THD_NAME      64
#define LEN_THD_STEP     128

pid_t lfsm_bg_thd_pid;

int32_t start_check;
typedef struct thread_monitor_item {
    int8_t thd_step[LEN_THD_STEP];
    int8_t thd_name[LEN_THD_NAME];
    struct list_head list_thd_tbl;
    pid_t thd_id;
    int32_t access_cnt;
} TMon_item_t;

#define TBL_HASH_SIZE   1024
#define TBL_MOD_VAL (TBL_HASH_SIZE - 1)
typedef struct thread_monitor_tbl {
    struct list_head thd_tbl[TBL_HASH_SIZE];
    rwlock_t tbl_lock;
    int32_t cnt_items;
} TMon_tbl_t;

TMon_tbl_t thd_mon_tbl;

#endif /* STORAGE_CORE_LFSM_LFSM_THREAD_MONITOR_PRIVATE_H_ */
