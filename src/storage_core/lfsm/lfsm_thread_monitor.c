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
#include <linux/types.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/spinlock_types.h>
#include <linux/wait.h>
#include "config/lfsm_setting.h"
#include "config/lfsm_feature_setting.h"
#include "config/lfsm_cfg.h"
#include "lfsm_thread_monitor_private.h"
#include "common/common.h"
#include "common/mem_manager.h"
#include "common/rmutex.h"
#include "io_manager/mqbiobox.h"
#include "io_manager/biocbh.h"
#include "nextprepare.h"
#include "diskgrp.h"
#include "system_metadata/Dedicated_map.h"
#include "lfsm_core.h"

static TMon_item_t *_find_thd_list_lockless(pid_t thdID,
                                            struct list_head *mylist)
{
    TMon_item_t *cur, *next, *tItem;

    tItem = NULL;

    list_for_each_entry_safe(cur, next, mylist, list_thd_tbl) {
        if (cur->thd_id == thdID) {
            tItem = cur;
            break;
        }
    }

    return tItem;
}

static int32_t add_thd_tbl(TMon_item_t * tItem, TMon_tbl_t * mytbl)
{
    struct list_head *mylist;
    TMon_item_t *tItem_find;
    int32_t ret, bk_index;

    ret = 0;

    bk_index = tItem->thd_id & TBL_MOD_VAL;
    mylist = &mytbl->thd_tbl[bk_index];

    write_lock(&mytbl->tbl_lock);

    do {
        tItem_find = _find_thd_list_lockless(tItem->thd_id, mylist);
        if (tItem_find != NULL) {
            ret = -EEXIST;
            break;
        }

        list_add_tail(&tItem->list_thd_tbl, mylist);
        mytbl->cnt_items++;
    } while (0);

    write_unlock(&mytbl->tbl_lock);

    return ret;
}

static void rm_thd_tbl(pid_t thdID, TMon_tbl_t * mytbl)
{
    struct list_head *mylist;
    TMon_item_t *tItem_find;
    int32_t bk_index;

    bk_index = thdID & TBL_MOD_VAL;
    mylist = &mytbl->thd_tbl[bk_index];

    write_lock(&mytbl->tbl_lock);

    do {
        tItem_find = _find_thd_list_lockless(thdID, mylist);
        if (tItem_find == NULL) {
            break;
        }

        list_del(&tItem_find->list_thd_tbl);
        mytbl->cnt_items--;
    } while (0);

    write_unlock(&mytbl->tbl_lock);

    if (tItem_find != NULL) {
        track_kfree(tItem_find, sizeof(TMon_item_t), M_OTHER);
    }
}

void dereg_thread_monitor(pid_t thdID)
{
    rm_thd_tbl(thdID, &thd_mon_tbl);
    if (lfsm_bg_thd_pid == thdID) {
        lfsm_bg_thd_pid = -1;
    }
}

static void _init_thd_item(TMon_item_t * tItem, pid_t thdID, int8_t * thdName)
{
    tItem->thd_id = thdID;
    tItem->access_cnt = 0;
    INIT_LIST_HEAD(&tItem->list_thd_tbl);
    memset(tItem->thd_name, '\0', LEN_THD_NAME);
    memset(tItem->thd_step, '\0', LEN_THD_STEP);
    strcpy(tItem->thd_name, thdName);
}

void reg_thread_monitor(pid_t thdID, int8_t * thdName)
{
    TMon_item_t *tItem;

    tItem = track_kmalloc(sizeof(TMon_item_t), GFP_KERNEL, M_OTHER);
    if (IS_ERR_OR_NULL(tItem)) {
        LOGINFO("fail register thread %d %s to monitor\n", thdID, thdName);
        return;
    }

    _init_thd_item(tItem, thdID, thdName);

    if (strncmp(thdName, "lfsm_bg_thread", strlen("lfsm_bg_thread")) == 0) {
        lfsm_bg_thd_pid = thdID;
    }

    if (add_thd_tbl(tItem, &thd_mon_tbl)) {
        track_kfree(tItem, sizeof(TMon_item_t), M_OTHER);
    }
}

bool is_lfsm_bg_thd(pid_t thdID)
{
    if (!start_check) {
        return false;
    }

    if (lfsm_bg_thd_pid == thdID) {
        return true;
    } else {
        return false;
    }
}

void log_thread_exec_step(pid_t thdID, int8_t * msg)
{
    TMon_item_t *tItem;
    struct list_head *mylist;
    int32_t bk_index;

    bk_index = thdID & TBL_MOD_VAL;
    mylist = &thd_mon_tbl.thd_tbl[bk_index];

    read_lock(&thd_mon_tbl.tbl_lock);

    do {
        tItem = _find_thd_list_lockless(thdID, mylist);
        if (tItem == NULL) {
            break;
        }

        tItem->access_cnt++;
        memset(tItem->thd_step, '\0', LEN_THD_STEP);
        strcpy(tItem->thd_step, msg);
    } while (0);

    read_unlock(&thd_mon_tbl.tbl_lock);
}

void init_thread_monitor(void)
{
    int32_t i;

    for (i = 0; i < TBL_HASH_SIZE; i++) {
        INIT_LIST_HEAD(&thd_mon_tbl.thd_tbl[i]);
    }

    rwlock_init(&thd_mon_tbl.tbl_lock);
    thd_mon_tbl.cnt_items = 0;
    lfsm_bg_thd_pid = -1;
    start_check = 0;
}

static void move_thd_item_buff(struct list_head *mylist,
                               struct list_head *bufflist)
{
    TMon_item_t *cur, *next;

    list_for_each_entry_safe(cur, next, mylist, list_thd_tbl) {
        list_move_tail(&cur->list_thd_tbl, bufflist);
    }
}

static void rel_thd_item(struct list_head *bufflist)
{
    TMon_item_t *cur, *next;

    list_for_each_entry_safe(cur, next, bufflist, list_thd_tbl) {
        list_del(&cur->list_thd_tbl);
        track_kfree(cur, sizeof(TMon_item_t), M_OTHER);
    }
}

static void rel_all_thd_item_tbl(TMon_tbl_t * mytbl)
{
    struct list_head bufflist;
    int32_t i;

    for (i = 0; i < TBL_HASH_SIZE; i++) {
        /*
         * NOTE: 2016-08-18 Lego: Good coding habit:
         * try not call a function which might sleep when hold spin lock.
         * kfree should not trigger sleep but kmalloc might trigger sleep
         * We treat kfree the same with kmalloc
         */
        INIT_LIST_HEAD(&bufflist);
        write_lock(&mytbl->tbl_lock);
        move_thd_item_buff(&mytbl->thd_tbl[i], &bufflist);
        write_unlock(&mytbl->tbl_lock);

        rel_thd_item(&bufflist);
    }
}

void rel_thread_monitor(void)
{
    rel_all_thd_item_tbl(&thd_mon_tbl);
    lfsm_bg_thd_pid = -1;
    start_check = 0;
}

static int32_t _show_thd_info(int8_t * buffer, struct list_head *mylist)
{
    TMon_item_t *cur, *next;
    int32_t len;

    len = 0;
    list_for_each_entry_safe(cur, next, mylist, list_thd_tbl) {
        len += sprintf(buffer + len, "%d %d %s %s\n",
                       cur->thd_id, cur->access_cnt, cur->thd_name,
                       cur->thd_step);
    }

    return len;
}

int32_t show_thread_monitor(int8_t * buffer, int32_t buff_size)
{
    int32_t len, i;

    len = 0;

    for (i = 0; i < TBL_HASH_SIZE; i++) {
        read_lock(&thd_mon_tbl.tbl_lock);
        len += _show_thd_info(buffer + len, &thd_mon_tbl.thd_tbl[i]);
        read_unlock(&thd_mon_tbl.tbl_lock);

        if (len >= buff_size - 80) {
            break;
        }
    }

    return len;
}
