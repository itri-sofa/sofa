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
