/*
 * SPDX-License-Identifier: GPL-2.0-or-later OR Apache-2.0
 * Copyright (C) 2015-2020 Industrial Technology Research Institute
 *
 *
 *
 *  
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
