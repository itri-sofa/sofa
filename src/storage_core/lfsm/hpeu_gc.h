/*
 * SPDX-License-Identifier: GPL-2.0-or-later OR Apache-2.0
 * Copyright (C) 2015-2020 Industrial Technology Research Institute
 *
 *
 *
 *  
 */

#ifndef _HPEU_GC_H_
#define _HPEU_GC_H_

#include "hpeu.h"
#include "GC.h"

#define MAX_HPEUGCBH_THREAD 1   // only can be 1 thread.
//because when gc conflict, all bioc in other threads will be put back and make system crash. tifa:2013/03/14

typedef struct HPEUgc_item {
    int32_t id_hpeu;
    int32_t id_grp;
    gc_item_t ar_gc_item[0];    //[DISK_PER_HPEU];
} HPEUgc_item_t;

typedef struct HPEUgc_entry {
    struct list_head link;
    HPEUgc_item_t hpeu_gc_item;
} HPEUgc_entry_t;

extern bool HPEUgc_entry_alloc_insert(struct list_head *list_to_gc,
                                      struct EUProperty *pick_eu);
extern int32_t HPEUgc_process(struct lfsm_dev_struct *td, int32_t * ar_gc_flag);
extern int32_t HPEUgc_dec_pin_and_wakeup(sector64 source_pbn);
extern bool HPEUgc_selectable(struct HPEUTab *phpeutab, int32_t id_hpeu);

#endif
