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
/*
** This is the header file for EU_access.h file
** Only the function prototypes declared below are exported to be used by other files
*/

#ifndef    EU_ACCESS_H
#define    EU_ACCESS_H

#define EU_UNUSABLE 0xfffffffe  /* Max erase count */

extern int32_t LVP_heap_insert(struct HListGC *, struct EUProperty *);
extern bool LVP_heap_delete(struct HListGC *, struct EUProperty *);
extern void HList_delete_A(struct HListGC *h, struct EUProperty *eu);
extern int32_t HList_delete(struct HListGC *, struct EUProperty *);
extern void HList_insert(struct HListGC *, struct EUProperty *);
extern void FalseList_insert(struct HListGC *h, struct EUProperty *eu);
extern void FalseList_delete(struct HListGC *h, struct EUProperty *eu);
extern int32_t FreeList_insert_withlock(struct EUProperty *eu);

extern struct EUProperty *FreeList_delete_by_pos(lfsm_dev_t * td,
                                                 sector64 eu_start_pos,
                                                 int32_t eu_usage);
extern struct EUProperty *FreeList_delete_by_pos_A(sector64 eu_start_pos,
                                                   int32_t eu_usage);
extern void FreeList_Dump(struct HListGC *h);

extern int32_t FreeList_insert(struct HListGC *, struct EUProperty *);
extern int32_t FreeList_insert_nodirty(struct HListGC *h,
                                       struct EUProperty *eu);

extern struct EUProperty *FreeList_delete(struct HListGC *, int32_t eu_usage,
                                          bool from_head);
struct EUProperty *FreeList_delete_with_log(lfsm_dev_t * td, struct HListGC *h,
                                            int32_t eu_usage, bool from_head);
extern struct EUProperty *FreeList_delete_A(lfsm_dev_t * td, struct HListGC *h,
                                            int32_t tp_eu_usage, bool from_head,
                                            bool is_log_needed);

bool FreeList_delete_selected_eu(struct HListGC *h, struct EUProperty *eu,
                                 int32_t eu_usage);
extern int32_t FreeList_delete_eu_direct(struct EUProperty *eu, int32_t usage);

extern void move_old_eu_to_HList(struct HListGC *, struct EUProperty *);
extern void sort_erase_count(struct HListGC *);
extern bool drop_EU(uint32_t EU_number);
extern void drift_element_down(struct HListGC *h, int32_t input_index,
                               bool isBackup);
extern struct EUProperty *eu_find(sector64 pbno);
extern int32_t eu_valid_check(struct EUProperty *eu);
extern int32_t eu_is_active(struct EUProperty *eu);
extern struct EUProperty *EU_EnsureUsable(struct EUProperty *eu_org);
int32_t start_eu_idx_in_disk(int32_t id_disk);
extern bool EU_copyall(lfsm_dev_t * td, struct EUProperty *eu_source,
                       struct EUProperty *eu_dest,
                       int32_t cn_page /* 0 means use log_frontier */ ,
                       bool isAppendLPN, int8_t * ar_valid_byte);
extern struct EUProperty *EU_reset(struct EUProperty *eu_org);
extern bool EU_rescue(lfsm_dev_t * td, struct EUProperty **eu_main,
                      struct EUProperty **eu_backup, int32_t cn_page,
                      bool isAppendLPN, int8_t * ar_valid_byte);

extern bool sysEU_rescue(lfsm_dev_t * td, struct EUProperty **eu_main,
                         struct EUProperty **eu_backup, SNextPrepare_t * next,
                         struct mutex *lock);
extern bool eu_same_temperature(sector64 prev, sector64 curr);

#define PBNO_TO_EUID(pbno) ((int32_t)((pbno - eu_log_start)>>EU_BIT_SIZE))

extern int32_t freelist_scan_and_check(struct HListGC *h);
extern int32_t eumap_start(int32_t id_disk);

#endif
