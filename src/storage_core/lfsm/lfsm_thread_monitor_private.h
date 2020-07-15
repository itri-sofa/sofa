/*
 * SPDX-License-Identifier: GPL-2.0-or-later OR Apache-2.0
 * Copyright (C) 2015-2020 Industrial Technology Research Institute
 *
 *
 *
 *  
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
