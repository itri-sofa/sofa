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
** This file constitues of all the functions used to perform :
** A. Insert, delete and heapify on Heap
** B. Insert, delete on HList
** C. Insert, delete on Freelist
*/
#include <linux/types.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/bio.h>
#include "config/lfsm_setting.h"
#include "config/lfsm_feature_setting.h"
#include "config/lfsm_cfg.h"
#include "common/common.h"
#include "common/mem_manager.h"
#include "common/rmutex.h"
#include "io_manager/mqbiobox.h"
#include "io_manager/biocbh.h"
#include "nextprepare.h"
#include "diskgrp.h"
#include "system_metadata/Dedicated_map.h"
#include "lfsm_core.h"
#include "io_manager/io_common.h"
#include "bmt.h"
#include "GC.h"
#include "EU_create.h"
#include "EU_access.h"
#include "special_cmd.h"
#include "spare_bmt_ul.h"
#include "bmt_ppdm.h"
#include "mdnode.h"

int32_t eu_valid_check(struct EUProperty *eu)
{
    int32_t i;
    for (i = 0; i < grp_ssds.capacity >> SECTORS_PER_SEU_ORDER; i++) {
        if (eu == pbno_eu_map[i]) {
            goto l_succ;
        }
    }

    LOGERR("Internal# %p is not a eu\n", eu);
    dump_stack();
    return -ENOANO;

  l_succ:
    if (eu_find(eu->start_pos) != eu) {
        LOGERR("Internal# eu %p crash\n", eu);
        dump_stack();
        return -EFAULT;
    }

    return 0;
}

bool eu_same_temperature(sector64 prev, sector64 curr)
{
    struct EUProperty *eu_prev, *eu_curr;
    bool eq;

    eu_prev = eu_find(prev);
    eu_curr = eu_find(curr);
    eq = (eu_prev->temper == eu_curr->temper);
    return eq;
}

int32_t eumap_start(int32_t id_disk)
{
    sector64 total_eu;
    int32_t i;

    total_eu = 0;
    for (i = 0; i < id_disk; i++) {
        total_eu += gc[i]->total_eus;
    }

    return (int32_t) total_eu;
}

struct EUProperty *eu_find(sector64 pbno)
{
    if ((pbno >= grp_ssds.capacity) || (pbno <= PBNO_SPECIAL_NOT_IN_CACHE)) {
        dump_stack();
        LOGERR("pbno: %llu over-bound of whole disk\n", pbno);
        return NULL;
    }

    return pbno_eu_map[PBNO_TO_EUID(pbno)];
}

// return temperature or -EFALUT if fail to got it.
int32_t eu_is_active(struct EUProperty *eu)
{
    struct HListGC *h;
    int32_t i;

    h = gc[eu->disk_num];
    for (i = 0; i < h->NumberOfRegions; i++) {
        if (eu == h->active_eus[i]) {
            return i;
        }
    }

    return -EFAULT;
}

struct EUProperty *FreeList_delete_by_pos_A(sector64 eu_start_pos,
                                            int32_t eu_usage)
{
    struct HListGC *h;
    struct EUProperty *eu;

    eu = pbno_eu_map[(eu_start_pos - eu_log_start) >> EU_BIT_SIZE];
    if (NULL == eu) {
        LOGERR("eu is NULL\n");
        dump_stack();
        return eu;
    }
    h = gc[eu->disk_num];
    if (!FreeList_delete_selected_eu(h, eu, eu_usage)) {
        LOGWARN("Can't find selected eu %llu in freelist\n", eu->start_pos);
    }
    return eu;
}

struct EUProperty *FreeList_delete_by_pos(lfsm_dev_t * td, sector64 pos,
                                          int32_t eu_usage)
{
    struct EUProperty *eu_ret;

    eu_ret = eu_find(pos);
    eu_ret->usage = eu_usage;
    if (eu_ret->erase_count == EU_UNUSABLE) {   // bad block
        if (!DEV_SYS_READY(grp_ssds, eu_ret->disk_num)) {
            return eu_ret;      // new inser disk, bad eu will be replaced in later stage
        } else {
            LOGERR("Internal#  eu:%p , disk_num %d\n", eu_ret,
                   eu_ret->disk_num);
            return NULL;        // original disk, un reasonable result
        }
    } else {
        return FreeList_delete_by_pos_A(pos, eu_usage);
    }
}

/*
** To insert an eu into the freelist
**
** @h : Garbage Collection structure
** @eu : The eu to be inserted
**
** Returns void
*/
static int32_t FreeList_insert_setup_init(struct HListGC *h,
                                          struct EUProperty *eu)
{
    if (h->disk_num != eu->disk_num) {
        LOGERR("Wrong disk num: sector %llu disk_num %d expected disk %d\n",
               eu->start_pos, eu->disk_num, h->disk_num);
        return -EPERM;
    }

    if (NULL != (eu->act_metadata.mdnode)) {
        LOGERR("EU(%p) %llu active_metadata is not empty %p\n",
               eu, eu->start_pos, eu->act_metadata.mdnode);
        return -EINVAL;
    }

    eu->log_frontier = 0;
    atomic_set(&eu->eu_valid_num, 0);
    eu->index_in_updatelog = -1;
    eu->fg_metadata_ondisk = false;
    eu->usage = EU_FREE;
    eu->id_hpeu = -1;
    return 0;
}

int32_t FreeList_insert_withlock(struct EUProperty *eu)
{
    struct HListGC *h;
    int32_t ret;

    h = gc[eu->disk_num];
    LFSM_RMUTEX_LOCK(&h->whole_lock);
    ret = FreeList_insert(h, eu);
    LFSM_RMUTEX_UNLOCK(&h->whole_lock);
    return ret;
}

/*
 * Description: insert a free EU to free list and sort base on erase count
 * TODO: the sorting might be a performance bottleneck if list is long
 */
int32_t FreeList_insert(struct HListGC *h, struct EUProperty *eu)
{
    struct EUProperty *peu_inlist, *n;
    int32_t ret;

    ret = 0;
    if ((ret = FreeList_insert_setup_init(h, eu)) < 0) {
        return ret;
    }
    //// insert in erase_count order
    list_for_each_entry_safe_reverse(peu_inlist, n, &h->free_list, list) {
        //    printk("peu_inlist %p peu_next %p peu_next->list %p %p, erase:%d\n",
        //                        peu_inlist,n,&(n->list), (&h->free_list),peu_inlist->erase_count);
        if (eu->erase_count >= peu_inlist->erase_count) {
            break;
        }
    }
    list_add(&eu->list, &(peu_inlist->list));
    h->free_list_number++;
    atomic_set(&eu->erase_state, DIRTY);
    return 0;
}

int32_t FreeList_insert_nodirty(struct HListGC *h, struct EUProperty *eu)
{
    int32_t ret;

    ret = 0;
    if ((ret = FreeList_insert_setup_init(h, eu)) < 0) {
        return ret;
    }
    //// insert head when normal, insert tail when counter<=MIN_RESERVE_EU
    if (h->free_list_number <= MIN_RESERVE_EU) {
        list_add_tail(&eu->list, &h->free_list);
    } else {
        list_add(&eu->list, &h->free_list);
    }
    h->free_list_number++;
    return ret;
}

/*
** To delete an eu from the freelist
**
** @h : Garbage Collection structure
**
** Returns the pointer of the eu deleted
** return value is ERR_PTR(ENOMEM,ENOSPC,EINVAL,ENOEXEC,,valid_eu)
*/
/*
 * TODO: Don't mix up the error code and null pointer since not all functions
 * using IS_ERR_OR_NULL to check pointer
 */
struct EUProperty *FreeList_delete_A(lfsm_dev_t * td, struct HListGC *h,
                                     int32_t tp_eu_usage, bool from_head,
                                     bool is_log_needed)
{
    struct EUProperty *pret_eu;
    int32_t ret;

    ret = 0;
    while (true) {
        if (list_empty(&h->free_list)) {
            return NULL;
        }

        pret_eu =
            list_entry((from_head) ? h->free_list.next : h->free_list.prev,
                       typeof(struct EUProperty), list);
        if (pret_eu == NULL) {
            return NULL;
        }

        if (atomic_read(&pret_eu->erase_state) == DIRTY) {  //may be dirty or log_fail
            atomic_set(&pret_eu->erase_state, EU_PICK);
        }
        //// log eu (only do dirty eu)
        if (is_log_needed) {
            if (UpdateLog_logging_if_undone(td, pret_eu) < 0) {
                return NULL;
            }
        }
        //// delete eu from free_list
        list_del_init(&pret_eu->list);
        if (h->free_list_number == 0) {
            LOGWARN("free_list_number++ for free_list is non zero\n");
        } else {
            h->free_list_number--;
        }

        /// erase eu
        switch (ret = EU_erase_if_undone(h, pret_eu)) {
        case -EIO:
            continue;
        case 0:
            if ((ret = EU_param_init(pret_eu, tp_eu_usage, 0)) < 0) {
                return NULL;
            }
            atomic_dec(&h->cn_clean_eu);
            return pret_eu;
        default:               // <0
            return NULL;
        }
    }

    return NULL;
}

int32_t freelist_scan_and_check(struct HListGC *h)
{
    struct EUProperty *eu_cur, *eu_next;
    int32_t ret;

    eu_next = NULL;
    ret = -ENOSPC;
    LFSM_RMUTEX_LOCK(&h->whole_lock);
    if (h->free_list_number != 0 && list_empty(&h->free_list) == true) {
        LOGERR("id_disk[%d]: freelist cut off list head is empty\n",
               h->disk_num);
        goto l_end;
    }

    list_for_each_entry_safe(eu_cur, eu_next, &h->free_list, list) {
        if ((sector64 *) eu_next->list.prev == (sector64 *) eu_cur) {
            continue;
        }
        LOGERR("id_disk[%d]: freelist cut off eu_next->prev %p "
               "not equal eu_cur %p\n", h->disk_num, eu_next->list.prev,
               eu_cur);
        goto l_end;
    }

    ret = 0;

  l_end:
    LFSM_RMUTEX_UNLOCK(&h->whole_lock);
    return ret;
}

/*
 * Description: get a EU from HListGC
 */
struct EUProperty *FreeList_delete(struct HListGC *h, int32_t eu_usage,
                                   bool from_head)
{
    struct EUProperty *eu_new;
    eu_new = FreeList_delete_A(&lfsmdev, h, eu_usage, from_head, false);
    LFSM_ASSERT(!IS_ERR_OR_NULL(eu_new));
    return eu_new;
}

struct EUProperty *FreeList_delete_with_log(lfsm_dev_t * td,
                                            struct HListGC *h, int32_t eu_usage,
                                            bool from_head)
{
    struct EUProperty *eu_new;

    eu_new = FreeList_delete_A(td, h, eu_usage, from_head, true);
    LFSM_ASSERT(!IS_ERR_OR_NULL(eu_new));
    return eu_new;
}

int32_t FreeList_delete_eu_direct(struct EUProperty *eu, int32_t usage)
{
    struct HListGC *h;
    if (eu->disk_num < 0 || eu->disk_num >= lfsmdev.cn_ssds) {
        return -EINVAL;
    }
//    printk("%s: eu(%p) %llu disk_id %d heap_idx %d list_id %d\n",__FUNCTION__,eu,eu->start_pos, eu->disk_num,eu->index_in_LVP_Heap, atomic_read(&eu->state_list));
    h = gc[eu->disk_num];
    eu->fg_metadata_ondisk = true;
    eu->log_frontier =
        eu->eu_size - (METADATA_SIZE_IN_SECTOR - DUMMY_SIZE_IN_SECTOR);
    list_del_init(&eu->list);
    if (LVP_heap_insert(h, eu) < 0) {
        return -EPERM;
    }
    h->free_list_number--;
    eu->usage = usage;
    return 0;
}

void FreeList_Dump(struct HListGC *h)
{
    struct EUProperty *entry, *next;
    int32_t idx;

    idx = 0;
    LOGINFO("dump free eu\n");
    list_for_each_entry_safe(entry, next, &h->free_list, list) {
        LOGINFO("%d: EU %lld state %d 0x%x\n", idx++,
                (sector64) entry->start_pos, atomic_read(&entry->erase_state),
                entry->erase_count);
    }
    return;
}

bool FreeList_delete_selected_eu(struct HListGC *h, struct EUProperty *eu,
                                 int32_t eu_usage)
{
    struct EUProperty *entry, *next;

    list_for_each_entry_safe(entry, next, &h->free_list, list) {
        if (eu == entry) {
            // printk("Entry %llu found and being deleted\n",entry->start_pos);
            list_del_init(&entry->list);
            h->free_list_number--;
            eu->usage = eu_usage;

            return true;
        }
    }

    return false;
}

void FalseList_insert(struct HListGC *h, struct EUProperty *eu)
{
    LOGINFO("HH: falselist insert pbno=%llu valid num=%d\n", eu->start_pos,
            atomic_read(&eu->eu_valid_num));
    atomic_inc(&h->false_list_number);
    list_add(&eu->list, &h->false_list);
    atomic_set(&eu->state_list, 2);
}

void FalseList_delete(struct HListGC *h, struct EUProperty *eu)
{
    LOGINFO("HH: falselist delete pbno=%llu valid num=%d\n",
            eu->start_pos, atomic_read(&eu->eu_valid_num));
    list_del_init(&eu->list);
    atomic_dec(&h->false_list_number);

    LFSM_ASSERT2(atomic_read(&eu->state_list) == 2,
                 "EU start %llu disk num %d state_list %d\n",
                 eu->start_pos, eu->disk_num, atomic_read(&eu->state_list));
    LFSM_ASSERT(atomic_read(&h->false_list_number) >= 0);
}

/*
** To insert an eu into the hlist
**
** @h : Garbage Collection structure
** @eu : The eu to be inserted
**
** Returns void
*/
void HList_insert(struct HListGC *h, struct EUProperty *eu)
{
    tf_printk("HH: hlist insert pbno=%llu valid num=%d\n",
              eu->start_pos, atomic_read(&eu->eu_valid_num));
    list_add(&eu->list, &h->hlist);
    h->hlist_number++;
    atomic_set(&eu->state_list, 1);
}

/*
** To delete an eu from the hlist
**
** @h : Garbage Collection structure
** @eu : The eu to be deleted
**
** Returns void
*/
void HList_delete_A(struct HListGC *h, struct EUProperty *eu)
{
    list_del_init(&eu->list);
    h->hlist_number--;
}

int32_t HList_delete(struct HListGC *h, struct EUProperty *eu)
{
    tf_printk("HH: hlist delete pbno=%llu valid num=%d\n", eu->start_pos,
              atomic_read(&eu->eu_valid_num));

    if (h->hlist_number <= 0) {
        LOGINERR("hlist not enough, eu(%p) %llu id_disk %d\n", eu,
                 eu->start_pos, eu->disk_num);
        return -ENOMEM;
    }

    if (atomic_read(&eu->state_list) != 1) {
        LOGERR("disk[%d] h[%d] EU addr %llu list_flag %d\n",
               eu->disk_num, h->disk_num, eu->start_pos,
               atomic_read(&eu->state_list));
        return -EINVAL;
//        LFSM_ASSERT(atomic_read(&eu->state_list)==1);
    }

    HList_delete_A(h, eu);

    return 0;
}

void HList_dump(struct HListGC *h)
{
    struct EUProperty *pEU;
    int32_t safe_bound;

    safe_bound = 20;
    LOGINFO("hlist_dump:");
    list_for_each_entry_reverse(pEU, &h->hlist, list) {
        printk("%lld(%d) ", pEU->start_pos, atomic_read(&pEU->state_list));
        safe_bound--;
        if (safe_bound == 0) {
            printk("unsafe exit!\n");
            return;
            break;
        }
    }
    printk("done\n");
}

/*
** Helper function in the heapify process to identify the weaker of the two components
**
** @e1 : eu 1 
** @e2 : eu 2
**
** Returns true if e1's utilization is less than that of e2
** False, otherwise
*/
static bool utilWeakComp(struct EUProperty *e1, struct EUProperty *e2)
{
    if (atomic_read(&e1->eu_valid_num) < atomic_read(&e2->eu_valid_num)) {
        return true;
    }

    return false;
}

/*
** Helper function in the heapify process
** Drift the new added positions up until the heap is built again
**
** @h : Garbage Collection structure
** @input_index : The position of the element with which the swapping is to be done
**
** Returns void
*/
static void drift_element_up(struct HListGC *h, int32_t input_index,
                             bool isBackup)
{
    struct EUProperty *tmp;
    struct EUProperty **heap;
    int32_t *pHeap_number;
    int32_t parent, sibling, max_index, index;

    if (isBackup) {
        pHeap_number = &h->LVP_Heap_backup_number;
        heap = h->LVP_Heap_backup;
    } else {
        pHeap_number = &h->LVP_Heap_number;
        heap = h->LVP_Heap;
    }

    parent = index = input_index;

    do {
        index = parent;
        parent = ((index - 1) >> 1);
        sibling = (parent << 2) + 3 - index;

        if (parent < 0) {
            break;
        }

        max_index = parent;
        if (sibling < *pHeap_number) {
            if (utilWeakComp(heap[sibling], heap[max_index])) {
                max_index = sibling;
            }
        }

        LFSM_ASSERT(parent < *pHeap_number);

        if (utilWeakComp(heap[index], heap[max_index])) {
            max_index = index;
        }

        if (max_index != parent) {
            if (!isBackup) {
                LFSM_ASSERT(h->LVP_Heap[max_index]->index_in_LVP_Heap ==
                            max_index);
                LFSM_ASSERT(h->LVP_Heap[parent]->index_in_LVP_Heap == parent);
                tmp = h->LVP_Heap[max_index];
                tmp->index_in_LVP_Heap = parent;
                h->LVP_Heap[parent]->index_in_LVP_Heap = max_index;
                h->LVP_Heap[max_index] = h->LVP_Heap[parent];
                h->LVP_Heap[parent] = tmp;
                LFSM_ASSERT(h->LVP_Heap[max_index]->index_in_LVP_Heap ==
                            max_index);
                LFSM_ASSERT(h->LVP_Heap[parent]->index_in_LVP_Heap == parent);
            } else {
                tmp = h->LVP_Heap_backup[max_index];
                h->LVP_Heap_backup[max_index] = h->LVP_Heap_backup[parent];
                h->LVP_Heap_backup[parent] = tmp;
            }
        }
    } while (max_index != parent);
}

/*
** Helper function in the heapify process
** Drift the new changed positions down until the heap is built again
**
** @h : Garbage Collection structure
** @input_index : The position of the element with which the swapping is to be done
**
** Returns void
*/
void drift_element_down(struct HListGC *h, int32_t input_index, bool isBackup)
{
    struct EUProperty *tmp;
    struct EUProperty **heap;
    int32_t *pHeap_number;
    int32_t left_child, right_child, max_index, index;

    if (isBackup) {
        pHeap_number = &h->LVP_Heap_backup_number;
        heap = h->LVP_Heap_backup;
    } else {
        pHeap_number = &h->LVP_Heap_number;
        heap = h->LVP_Heap;
    }

    max_index = index = input_index;

    do {
        index = max_index;

        left_child = (index << 1) + 1;
        right_child = (index + 1) << 1;

        max_index = index;

        if (right_child < *pHeap_number) {
            if (utilWeakComp(heap[right_child], heap[max_index])) {
                max_index = right_child;
            }
        }

        if (left_child < *pHeap_number) {
            if (utilWeakComp(heap[left_child], heap[max_index])) {
                max_index = left_child;
            }
        }

        if (max_index != index) {
            if (!isBackup) {
                LFSM_ASSERT(h->LVP_Heap[max_index]->index_in_LVP_Heap ==
                            max_index);
                LFSM_ASSERT(h->LVP_Heap[index]->index_in_LVP_Heap == index);

                tmp = h->LVP_Heap[max_index];
                tmp->index_in_LVP_Heap = index;
                h->LVP_Heap[index]->index_in_LVP_Heap = max_index;
                h->LVP_Heap[max_index] = h->LVP_Heap[index];

                h->LVP_Heap[index] = tmp;

                LFSM_ASSERT(h->LVP_Heap[max_index]->index_in_LVP_Heap ==
                            max_index);
                LFSM_ASSERT(h->LVP_Heap[index]->index_in_LVP_Heap == index);
            } else {
                tmp = h->LVP_Heap_backup[max_index];
                h->LVP_Heap_backup[max_index] = h->LVP_Heap_backup[index];
                h->LVP_Heap_backup[index] = tmp;
            }
        }

    } while (max_index != index);
}

/*
** To insert an eu into the heap and do the heapify, ofcourse.
**
** @h : Garbage Collection structure
** @eu : The eu to be inserted
**
** Returns void
*/
int32_t LVP_heap_insert(struct HListGC *h, struct EUProperty *eu)
{
    if (eu->index_in_LVP_Heap != -1) {
        LOGERR("%s eu(%p) %llu disk_id %d heap_idx %d list_id %d\n",
               __FUNCTION__, eu, eu->start_pos, eu->disk_num,
               eu->index_in_LVP_Heap, atomic_read(&eu->state_list));
        return -EPERM;
    }
    LFSM_ASSERT(eu->disk_num == h->disk_num);

    list_del_init(&eu->list);

    tf_printk("HH: heap insert pbno=%llu valid num=%d hwflag=%d\n",
              eu->start_pos, atomic_read(&eu->eu_valid_num),
              atomic_read(&eu->state_list));

    h->LVP_Heap_number++;
    eu->index_in_LVP_Heap = h->LVP_Heap_number - 1;
    h->LVP_Heap[eu->index_in_LVP_Heap] = eu;
    drift_element_up(h, h->LVP_Heap_number - 1, false);
    atomic_set(&eu->state_list, 0);

    return 0;
}

/*
** To delete an eu from the heap and do the heapify, ofcourse
**
** @h : Garbage Collection structure
** @eu : The eu to be deleted
**
** Returns true if the eu to be deleted is completely invalid (all the pages inside the eu are invalidated)
** False, otherwise
*/
bool LVP_heap_delete(struct HListGC *h, struct EUProperty *eu)
{
    struct EUProperty *tmp, *top_element;
    int32_t max_index, not_equal;
    bool eu_invalid;

    max_index = eu->index_in_LVP_Heap;
    not_equal = 0;
    eu_invalid = true;

    LFSM_ASSERT(h->disk_num == eu->disk_num);
    LFSM_ASSERT(h->LVP_Heap[max_index] == eu);

    tf_printk("HH: heap delete pbno=%llu valid num=%d\n",
              eu->start_pos, atomic_read(&eu->eu_valid_num));

    if (atomic_read(&eu->eu_valid_num)) {
        eu_invalid = false;
    }

    if (h->LVP_Heap_number - 1 != max_index) {

        top_element = h->LVP_Heap[0];
        tmp = h->LVP_Heap[max_index];

        //Swap the left-most leaf with the top element
        //
        LFSM_ASSERT(h->LVP_Heap[0]->index_in_LVP_Heap == 0);
        LFSM_ASSERT(h->LVP_Heap[h->LVP_Heap_number - 1]->index_in_LVP_Heap ==
                    h->LVP_Heap_number - 1);

        h->LVP_Heap[h->LVP_Heap_number - 1]->index_in_LVP_Heap = 0;
        h->LVP_Heap[0] = h->LVP_Heap[h->LVP_Heap_number - 1];

        h->LVP_Heap_number--;
        drift_element_down(h, 0, false);

        if (max_index != 0) {
            // The original one may change its position
            max_index = eu->index_in_LVP_Heap;

            LFSM_ASSERT(max_index < h->LVP_Heap_number);
            LFSM_ASSERT(max_index >= 0);

            LFSM_ASSERT(h->LVP_Heap[max_index]->index_in_LVP_Heap == max_index);

            top_element->index_in_LVP_Heap = max_index;
            h->LVP_Heap[max_index] = top_element;

            drift_element_up(h, max_index, false);

            LFSM_ASSERT(h->LVP_Heap[max_index]->index_in_LVP_Heap == max_index);
        }

        tmp->index_in_LVP_Heap = h->LVP_Heap_number;
        h->LVP_Heap[h->LVP_Heap_number] = tmp;

        not_equal = 1;
    } else {
        h->LVP_Heap_number--;
    }

    return eu_invalid;
}

/*
** To move an EU appropriately to HList
** Takes care about the HList limit
**
** @h : Garbage Collection structure
** @old_eu : The EUProperty pointer where the old PBN belonged to 
**
** Returns void
*/
void move_old_eu_to_HList(struct HListGC *h, struct EUProperty *old_eu)
{
    struct EUProperty *tmp_eu;
    int32_t cn_valid, ret;
    bool eu_invalid;

    tmp_eu = NULL;
    LFSM_ASSERT2(atomic_read(&old_eu->state_list) >= 0,
                 "eu start%llu hwflag=%d\n", old_eu->start_pos,
                 atomic_read(&old_eu->state_list));

    if (atomic_read(&old_eu->state_list) >= 0) {
        switch (atomic_read(&old_eu->state_list)) {
        case 0:
            eu_invalid = LVP_heap_delete(h, old_eu);
            old_eu->index_in_LVP_Heap = -1;
            break;

        case 1:
            if ((ret = HList_delete(h, old_eu)) < 0) {
                LOGINERR("hlist_delete fail ret %d\n", ret);
                LFSM_FORCE_RUN(return);
                LFSM_ASSERT(0);
            }
            break;

        default:
            LFSM_ASSERT(false);
            break;
        }
    }

    if (h->hlist_number >= h->hlist_limit) {
        tmp_eu = list_entry(h->hlist.prev, typeof(struct EUProperty), list);

        if ((ret = HList_delete(h, tmp_eu)) < 0) {
            LOGINERR("hlist_delete fail ret %d\n", ret);
            LFSM_ASSERT(0);
        } else {
            LVP_heap_insert(h, tmp_eu);
        }
    }

    cn_valid = atomic_read(&old_eu->eu_valid_num);
    if (cn_valid == 0) {        //all invalid eu must in heap list
        LVP_heap_insert(h, old_eu);
    } else {
        HList_insert(h, old_eu);    //Inserting the element to the tailof HList.
    }
}

void sort_erase_count(struct HListGC *h)
{
    struct list_head new_list;
    struct EUProperty *node;

    INIT_LIST_HEAD(&new_list);

    new_list.next = h->free_list.next;
    new_list.prev = h->free_list.prev;

    h->free_list.next->prev = &new_list;
    h->free_list.prev->next = &new_list;

    INIT_LIST_HEAD(&h->free_list);
    h->free_list_number = 0;

    while (!list_empty(&new_list)) {
        node = list_entry(new_list.next, typeof(struct EUProperty), list);

        list_del_init(&node->list);

        if (FreeList_insert_nodirty(h, node) < 0) {
            LFSM_ASSERT(0);
        }
    }
}

bool drop_EU(uint32_t EU_number)
{
    struct EUProperty *eu;

    eu = NULL;
    if (EU_number < (grp_ssds.capacity >> (SECTORS_PER_SEU_ORDER))) {
        eu = pbno_eu_map[EU_number];
    } else {
        LOGINFO("Invalid EU number %u (0~ %u)\n", EU_number,
                (uint32_t) (grp_ssds.capacity >> SECTORS_PER_SEU_ORDER));
        return false;
    }

    if (eu->erase_count == EU_UNUSABLE) {
//        printk("EU %u has been dropped\n",EU_number);
        return false;
    }
    eu->erase_count = EU_UNUSABLE;
    // Mark as bad so that next boot we wont use this EU
    //mark_as_bad(eu->start_pos, 1);

    //LOGINFO("dropping EU id:%d disk_num %d start_pos %llu\n",
    //        EU_number, pbno_eu_map[EU_number]->disk_num,pbno_eu_map[EU_number]->start_pos);
    return true;
}

//cn_page 0 means use log_frontier*/
bool EU_copyall(lfsm_dev_t * td, struct EUProperty *eu_source,
                struct EUProperty *eu_dest,
                int32_t cn_page /* 0 means use log_frontier */ ,
                bool isAppendLPN, int8_t * ar_valid_bit)
{
    sector64 pos_source, pos_dest;
    int8_t *buffer;
    bool ret;

    ret = false;
    pos_source = eu_source->start_pos;
    pos_dest = eu_dest->start_pos;

    if (NULL == (buffer = (int8_t *)
                 track_kmalloc(SZ_SPAREAREA * FPAGE_PER_SEU, GFP_KERNEL,
                               M_UNKNOWN))) {
        LOGERR("alloc mem failure\n");
        return false;
    }
    memset(buffer, -1, SZ_SPAREAREA * FPAGE_PER_SEU);

    if (cn_page == 0) {
        cn_page = eu_source->log_frontier >> SECTORS_PER_FLASH_PAGE_ORDER;
    }

    LOGINFO("EU_copyall source (%llu,%d) dest (%llu,%d) cn_page %d\n",
            pos_source, eu_source->disk_num, pos_dest, eu_dest->disk_num,
            cn_page);
    if (EU_erase(eu_dest) < 0) {
        LOGERR("erase failure of disk %d addr %llu\n", eu_dest->disk_num,
               eu_dest->start_pos);
        goto l_end;
    }

    if (eu_source->disk_num == eu_dest->disk_num) {
        ret = copy_whole_eu_inter(td, pos_source, pos_dest, cn_page);
    } else {
        if (isAppendLPN) {
            if (!EU_spare_read(td, buffer, pos_source, FPAGE_PER_SEU)) {
                goto l_end;
            }
        }

        ret = copy_whole_eu_cross(td, pos_source, pos_dest,
                                  (sector64 *) buffer, cn_page, ar_valid_bit);
    }

  l_end:
    if (buffer) {
        track_kfree(buffer, SZ_SPAREAREA * FPAGE_PER_SEU, M_UNKNOWN);
    }

    return ret;
}

struct EUProperty *EU_EnsureUsable(struct EUProperty *eu_org)
{
    struct EUProperty *eu_new;

    if (eu_org->erase_count == EU_UNUSABLE) {
        eu_new = FreeList_delete(gc[eu_org->disk_num], eu_org->usage, false);
        LOGINFO("Change eu_org %p s_pos %llu eu_new %p s_pos %llu\n",
                eu_org, eu_org->start_pos, eu_new, eu_new->start_pos);
        return eu_new;
    }

    return eu_org;
}

struct EUProperty *EU_reset(struct EUProperty *eu_org)
{
    struct EUProperty *eu;

    if (IS_ERR_OR_NULL(eu = EU_EnsureUsable(eu_org))) {
        return NULL;
    }

    if (EU_erase(eu) < 0) {
        LOGERR("erase fail eu %llu\n", eu->start_pos);
        return NULL;
    }

    eu->log_frontier = 0;

    return eu;
}

bool EU_rescue(lfsm_dev_t * td, struct EUProperty **eu_main,
               struct EUProperty **eu_backup, int32_t cn_page, bool isAppendLPN,
               int8_t * ar_valid_bit)
{
    int32_t id_main_eu, id_bkup_eu;

    id_main_eu = (*eu_main)->disk_num;
    id_bkup_eu = (*eu_backup)->disk_num;

    if (IS_ERR_OR_NULL((*eu_main) = EU_EnsureUsable((*eu_main)))) {
        return false;
    }

    if (IS_ERR_OR_NULL((*eu_backup) = EU_EnsureUsable(*eu_backup))) {
        return false;
    }

    LOGINFO("main disk_id %d sign %x ready %d start_pos %llu, "
            "bkup disk_id %d sign %x ready %d start_pos %llu\n",
            id_main_eu, grp_ssds.ar_subdev[id_main_eu].load_from_prev,
            DEV_SYS_READY(grp_ssds, id_main_eu) ? 1 : 0,
            (*eu_main)->start_pos, id_bkup_eu,
            grp_ssds.ar_subdev[id_bkup_eu].load_from_prev,
            DEV_SYS_READY(grp_ssds, id_bkup_eu) ? 1 : 0,
            (*eu_backup)->start_pos);

    if ((!DEV_SYS_READY(grp_ssds, id_main_eu)) &&
        (!DEV_SYS_READY(grp_ssds, id_bkup_eu)) &&
        (grp_ssds.ar_drives[id_main_eu].actived == 0) &&
        (grp_ssds.ar_drives[id_bkup_eu].actived == 0)) {
        dump_stack();
        LOGERR("can't rescue informaction %d \n", 1);
        return false;
    } else if (!DEV_SYS_READY(grp_ssds, id_main_eu)
               && grp_ssds.ar_drives[id_main_eu].actived == 1) {
        return EU_copyall(td, (*eu_backup), (*eu_main), cn_page, isAppendLPN,
                          ar_valid_bit);
    } else if (!DEV_SYS_READY(grp_ssds, id_bkup_eu)
               && grp_ssds.ar_drives[id_bkup_eu].actived == 1) {
        return EU_copyall(td, (*eu_main), (*eu_backup), cn_page, isAppendLPN,
                          ar_valid_bit);
    }
    return true;
}

bool sysEU_rescue(lfsm_dev_t * td, struct EUProperty **eu_main,
                  struct EUProperty **eu_backup, SNextPrepare_t * next,
                  struct mutex *lock)
{
    bool ret = false;

    ret = false;

    if (lfsm_readonly == 1) {
        return true;
    }

    LFSM_MUTEX_LOCK(lock);

    if (IS_ERR_OR_NULL(*eu_main) || IS_ERR_OR_NULL(*eu_backup)) {
        LOGERR("*eu_main %p  *eu_backup %p\n", *eu_main, *eu_backup);
        goto l_end;
    }

    if (!(ret = EU_rescue(td, eu_main, eu_backup, 0, false, NULL))) {
        goto l_end;
    }
    // AN 2012/10/11 because the next->eu_main and next->eu_backup will assign EU later when recovery...,
    //so we must add a check
    if (IS_ERR_OR_NULL(next->eu_main) || IS_ERR_OR_NULL(next->eu_backup)) {
        LOGERR("next->eu_main %p  next->eu_backup %p\n", next->eu_main,
               next->eu_backup);
        goto l_end;
    }
    // TF(2014/04/15): EU_rescue is a smart function would do EU_EnsureUsable() and copy
    // If parameter cn_page=0, the number of copy will be decided by eu->log_frontier in the function
    // In next_EU case, parameter cn_page=0 and log_frontier must be zero, so didn't do any copy
    if (!
        (ret =
         EU_rescue(td, &next->eu_main, &next->eu_backup, 0, false, NULL))) {
        goto l_end;
    }

  l_end:
    LFSM_MUTEX_UNLOCK(lock);
    return ret;
}
