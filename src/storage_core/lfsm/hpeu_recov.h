/*
 * SPDX-License-Identifier: GPL-2.0-or-later OR Apache-2.0
 * Copyright (C) 2015-2020 Industrial Technology Research Institute
 *
 *
 *
 *  
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
