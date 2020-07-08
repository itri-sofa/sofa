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
#include <linux/spinlock.h>
#include <linux/slab.h>         // kmalloc
#include "../common/common.h"
#include "../common/mem_manager.h"
#include "../common/tools.h"
#include "thd_perf_mon.h"
#include "thd_perf_mon_private.h"

static thdpm_item_t *_find_bk_list_lockless(pid_t thdID,
                                            struct list_head *mylist)
{
    thdpm_item_t *cur, *next, *tItem;

    tItem = NULL;

    list_for_each_entry_safe(cur, next, mylist, litem_thd_perfMon) {
        if (cur->thd_id == thdID) {
            tItem = cur;
            break;
        }
    }

    return tItem;
}

static void _log_perf_tbl(pid_t thdID, int32_t rw, int64_t dur)
{
    thdpm_item_t *tpmItem;
    struct list_head *mylist;
    int32_t bk_index;

    bk_index = thdID & THD_TBL_MODVAL;
    mylist = &thdPerfMon_tbl.thd_perfMon_tbl[bk_index];

    //NOTE: If affect perf too much, don't lock
    read_lock(&thdPerfMon_tbl.thd_perfMon_tbllock);

    do {
        tpmItem = _find_bk_list_lockless(thdID, mylist);
        if (tpmItem == NULL) {
            break;
        }

        if (rw) {
            tpmItem->cnt_proc_w++;
            tpmItem->time_proc_w += dur;
        } else {
            tpmItem->cnt_proc_r++;
            tpmItem->time_proc_r += dur;
        }
    } while (0);

    read_unlock(&thdPerfMon_tbl.thd_perfMon_tbllock);
}

void log_thd_perf_time(pid_t thdID, int32_t rw, int64_t time_s, int64_t time_e)
{
    int64_t t_dur;

    t_dur = time_e - time_s;

    if (t_dur < 0) {
        return;
    }

    _log_perf_tbl(thdID, rw, t_dur);
}

static void _log_stage_perf_tbl(pid_t thdID, int32_t stage, int64_t dur)
{
    thdpm_item_t *tpmItem;
    struct list_head *mylist;
    int32_t bk_index;

    bk_index = thdID & THD_TBL_MODVAL;
    mylist = &thdPerfMon_tbl.thd_perfMon_tbl[bk_index];

    //NOTE: If affect perf too much, don't lock
    read_lock(&thdPerfMon_tbl.thd_perfMon_tbllock);

    do {
        tpmItem = _find_bk_list_lockless(thdID, mylist);
        if (tpmItem == NULL) {
            break;
        }

        tpmItem->stage_time[stage] += dur;
        tpmItem->stage_count[stage]++;
    } while (0);

    read_unlock(&thdPerfMon_tbl.thd_perfMon_tbllock);
}

void log_thd_perf_stage_time(pid_t thdID, int32_t stage, int64_t time_s,
                             int64_t time_e)
{
    int64_t t_dur;

    if (stage >= MAX_MONITOR_STAGE) {
        return;
    }

    t_dur = time_e - time_s;
    if (t_dur < 0) {
        return;
    }

    _log_stage_perf_tbl(thdID, stage, t_dur);
}

static void _clear_thd_perf_type(struct list_head *mylist)
{
    thdpm_item_t *cur, *next;
    int32_t i;

    list_for_each_entry_safe(cur, next, mylist, litem_traversal) {
        cur->time_proc_r = 0;
        cur->time_proc_w = 0;
        cur->cnt_proc_r = 0;
        cur->cnt_proc_w = 0;

        for (i = 0; i < MAX_MONITOR_STAGE; i++) {
            cur->stage_time[i] = 0;
            cur->stage_count[i] = 0;
        }
    }
}

static void clear_thd_perf_value(void)
{
    int32_t i;

    for (i = 0; i < LFSM_THD_PERFMON_END; i++) {
        write_lock(&thdPerfMon_tbl.thd_perfMon_tbllock);
        _clear_thd_perf_type(&thdPerfMon_tbl.lhead_perfMon_traversal[i]);
        write_unlock(&thdPerfMon_tbl.thd_perfMon_tbllock);
    }
}

static void _init_tpm_item(thdpm_item_t * tItem, pid_t thdID,
                           thd_type_t thdType, int8_t * thdName)
{
    int32_t i;

    tItem->thd_id = thdID;
    tItem->thread_type = thdType;
    tItem->cnt_proc_r = 0;
    tItem->cnt_proc_w = 0;
    tItem->time_proc_r = 0;
    tItem->time_proc_w = 0;

    for (i = 0; i < MAX_MONITOR_STAGE; i++) {
        tItem->stage_time[i] = 0;
        tItem->stage_count[i] = 0;
    }

    INIT_LIST_HEAD(&tItem->litem_thd_perfMon);
    INIT_LIST_HEAD(&tItem->litem_traversal);
    memset(tItem->thd_name, '\0', LEN_THD_NAME);
    strcpy(tItem->thd_name, thdName);
}

static int32_t add_thd_tbl(thdpm_item_t * tItem, THD_PMon_tbl_t * mytbl)
{
    struct list_head *mylist;
    thdpm_item_t *tItem_find;
    int32_t ret, bk_index;

    ret = 0;

    bk_index = tItem->thd_id & THD_TBL_MODVAL;
    mylist = &mytbl->thd_perfMon_tbl[bk_index];

    write_lock(&mytbl->thd_perfMon_tbllock);

    do {
        tItem_find = _find_bk_list_lockless(tItem->thd_id, mylist);
        if (tItem_find != NULL) {
            ret = -EEXIST;
            break;
        }

        list_add_tail(&tItem->litem_thd_perfMon, mylist);
        list_add_tail(&tItem->litem_traversal,
                      &mytbl->lhead_perfMon_traversal[tItem->thread_type]);
        mytbl->thd_perfMon_cnt_items++;
    } while (0);

    write_unlock(&mytbl->thd_perfMon_tbllock);

    return ret;
}

void reg_thd_perf_monitor(pid_t thdID, thd_type_t thdType, int8_t * thdName)
{
    thdpm_item_t *tpmItem;

    tpmItem = track_kmalloc(sizeof(thdpm_item_t), GFP_KERNEL, M_PERF_MON);
    if (IS_ERR_OR_NULL(tpmItem)) {
        LOGINFO("fail register thread %d %s to monitor\n", thdID, thdName);
        return;
    }

    _init_tpm_item(tpmItem, thdID, thdType, thdName);

    if (add_thd_tbl(tpmItem, &thdPerfMon_tbl)) {
        track_kfree(tpmItem, sizeof(thdpm_item_t), M_PERF_MON);
    }
}

static void rm_thd_tbl(pid_t thdID, THD_PMon_tbl_t * mytbl)
{
    struct list_head *mylist;
    thdpm_item_t *tpmItem;
    int32_t bk_index;

    bk_index = thdID & THD_TBL_MODVAL;
    mylist = &mytbl->thd_perfMon_tbl[bk_index];

    write_lock(&mytbl->thd_perfMon_tbllock);

    do {
        tpmItem = _find_bk_list_lockless(thdID, mylist);
        if (tpmItem == NULL) {
            break;
        }

        list_del(&tpmItem->litem_thd_perfMon);
        list_del(&tpmItem->litem_traversal);
        mytbl->thd_perfMon_cnt_items--;
    } while (0);

    write_unlock(&mytbl->thd_perfMon_tbllock);

    if (tpmItem != NULL) {
        track_kfree(tpmItem, sizeof(thdpm_item_t), M_PERF_MON);
    }
}

void dereg_thd_perf_monitor(pid_t thdID)
{
    rm_thd_tbl(thdID, &thdPerfMon_tbl);
}

void init_thd_perf_monitor(void)
{
    int32_t i;

    for (i = 0; i < THD_TBL_HASH_SIZE; i++) {
        INIT_LIST_HEAD(&thdPerfMon_tbl.thd_perfMon_tbl[i]);
    }

    for (i = 0; i < LFSM_THD_PERFMON_END; i++) {
        INIT_LIST_HEAD(&thdPerfMon_tbl.lhead_perfMon_traversal[i]);
    }
    //NOTE: currently, we won't lock table for avoiding perf degrade
    rwlock_init(&thdPerfMon_tbl.thd_perfMon_tbllock);
    thdPerfMon_tbl.thd_perfMon_cnt_items = 0;
}

static int32_t move_thd_perfMon_item_buff(struct list_head *mylist,
                                          struct list_head *bufflist)
{
    thdpm_item_t *cur, *next;
    int32_t cnt;

    cnt = 0;
    list_for_each_entry_safe(cur, next, mylist, litem_thd_perfMon) {
        list_move_tail(&cur->litem_thd_perfMon, bufflist);
        list_del(&cur->litem_traversal);
        cnt++;
    }

    return cnt;
}

static void rel_thd_perfMon_item(struct list_head *bufflist)
{
    thdpm_item_t *cur, *next;

    list_for_each_entry_safe(cur, next, bufflist, litem_thd_perfMon) {
        list_del(&cur->litem_thd_perfMon);
        track_kfree(cur, sizeof(thdpm_item_t), M_PERF_MON);
    }
}

static void rel_all_thd_item_tbl(THD_PMon_tbl_t * mytbl)
{
    struct list_head bufflist;
    int32_t i, cnt;

    for (i = 0; i < THD_TBL_HASH_SIZE; i++) {
        /*
         * NOTE: 2016-09-13 Lego: Good coding habit:
         * try not call a function which might sleep when hold spin lock.
         * kfree should not trigger sleep but kmalloc might trigger sleep
         * We treat kfree the same with kmalloc
         */
        INIT_LIST_HEAD(&bufflist);
        write_lock(&mytbl->thd_perfMon_tbllock);
        cnt = move_thd_perfMon_item_buff(&mytbl->thd_perfMon_tbl[i], &bufflist);
        mytbl->thd_perfMon_cnt_items -= cnt;
        write_unlock(&mytbl->thd_perfMon_tbllock);

        rel_thd_perfMon_item(&bufflist);
    }
}

void rel_thd_perf_monitor(void)
{
    rel_all_thd_item_tbl(&thdPerfMon_tbl);

    if (thdPerfMon_tbl.thd_perfMon_cnt_items != 0) {
        LOGWARN("Thd Perf Mon some perf monitor item not released %d\n",
                thdPerfMon_tbl.thd_perfMon_cnt_items);
    }
}

#ifdef ENABLE_THD_PERF_MON
static int8_t *_get_thd_name(thd_type_t thdType)
{
    int32_t nmIndex;

    if (thdType > LFSM_THD_PERFMON_END || thdType < LFSM_IO_THD) {
        nmIndex = LFSM_THD_PERFMON_END;
    } else {
        nmIndex = thdType;
    }

    return thdType_name[nmIndex];
}

static int32_t _show_thdperf_type_summary(int8_t * buffer, int32_t buff_size,
                                          struct list_head *mylist)
{
    thdpm_item_t *cur, *next;
    uint64_t cnt_all, t_all, cnt_read, t_read, cnt_write, t_write;
    uint64_t iops_each, iops_all, t_stage_perf, t_stage;
    uint64_t t_stage_all[MAX_MONITOR_STAGE], t_stage_cnt_all[MAX_MONITOR_STAGE];
    int32_t len, total_thd, i;
    thd_type_t ctype;

    len = 0;
    total_thd = 0;
    cnt_all = 0;
    t_all = 0;
    cnt_read = 0;
    t_read = 0;
    cnt_write = 0;
    t_write = 0;

    iops_all = 0;
    iops_each = 0;

    ctype = LFSM_THD_PERFMON_END;

    for (i = 0; i < MAX_MONITOR_STAGE; i++) {
        t_stage_all[i] = 0;
        t_stage_cnt_all[i] = 0;
    }

    list_for_each_entry_safe(cur, next, mylist, litem_traversal) {
        total_thd++;
        ctype = cur->thread_type;

        cnt_read += cur->cnt_proc_r;
        cnt_write += cur->cnt_proc_w;

        t_read += jiffies_to_usecs(cur->time_proc_r);
        t_write += jiffies_to_usecs(cur->time_proc_w);

        for (i = 0; i < MAX_MONITOR_STAGE; i++) {
            t_stage_all[i] += cur->stage_time[i];
            t_stage_cnt_all[i] += cur->stage_count[i];
        }
        iops_each = do_divide((cur->cnt_proc_r + cur->cnt_proc_w) * 1000000,
                              (jiffies_to_usecs(cur->time_proc_r) +
                               jiffies_to_usecs(cur->time_proc_w)));
        iops_all += iops_each;
    }

    cnt_all = cnt_read + cnt_write;
    t_all = t_read + t_write;

#if 0
    len += sprintf(buffer + len,
                   "Summary: %s %d all %llu avg %llu IOPS (%llu %llu us) "
                   "read %llu IOPS (%llu %llu us) write %llu IOPS (%llu %llu us)\n",
                   _get_thd_name(ctype), total_thd, iops_all,
                   do_divide(cnt_all * 1000000, t_all), cnt_all, t_all,
                   do_divide(cnt_read * 1000000, t_read), cnt_read, t_read,
                   do_divide(cnt_write * 1000000, t_write), cnt_write, t_write);
#else
    len += sprintf(buffer + len,
                   "Summary: %s %d all %llu avg %llu IOPS (%llu %llu us)\n",
                   _get_thd_name(ctype), total_thd, iops_all,
                   do_divide(cnt_all * 1000000, t_all), cnt_all, t_all);
#endif
    for (i = 0; i < MAX_MONITOR_STAGE; i++) {
        t_stage = jiffies_to_usecs(t_stage_all[i]);
        t_stage_perf = do_divide(t_stage * 1000, t_stage_cnt_all[i]);
        if (i == 0) {
            len += sprintf(buffer + len, "        s%d: %llu (%llu %llu)",
                           i, t_stage_perf, t_stage, t_stage_cnt_all[i]);
        } else {
            len += sprintf(buffer + len, " s%d: %llu (%llu %llu)",
                           i, t_stage_perf, t_stage, t_stage_cnt_all[i]);
        }
    }
    len += sprintf(buffer + len, "\n");

    return len;
}

static int32_t _show_thdperf_type_one(int8_t * buffer, int32_t buff_size,
                                      struct list_head *mylist)
{
    thdpm_item_t *cur, *next;
    uint64_t cnt_all, t_all, cnt_read, t_read, cnt_write, t_write, t_stage,
        t_perf;
    uint64_t t_stage_cnt;
    int32_t len, i;

    len = 0;

    list_for_each_entry_safe(cur, next, mylist, litem_traversal) {
        cnt_all = cur->cnt_proc_r + cur->cnt_proc_w;
        t_all = cur->time_proc_r + cur->time_proc_w;
        t_all = jiffies_to_usecs(t_all);

        cnt_read = cur->cnt_proc_r;
        t_read = jiffies_to_usecs(cur->time_proc_r);

        cnt_write = cur->cnt_proc_w;
        t_write = jiffies_to_usecs(cur->time_proc_w);
#if 0
        len += sprintf(buffer + len,
                       "         %d %s "
                       "%llu IOPS (%llu %llu us) "
                       "read: %llu IOPS (%llu %llu us) "
                       "write: %llu IOPS (%llu %llu us)\n",
                       cur->thd_id, cur->thd_name,
                       do_divide(cnt_all * 1000000, t_all), cnt_all, t_all,
                       do_divide(cnt_read * 1000000, t_read), cnt_read, t_read,
                       do_divide(cnt_write * 1000000, t_write), cnt_write,
                       t_write);
#else
        len += sprintf(buffer + len,
                       "         %d %s "
                       "%llu IOPS (%llu %llu us)\n",
                       cur->thd_id, cur->thd_name,
                       do_divide(cnt_all * 1000000, t_all), cnt_all, t_all);
#endif
        for (i = 0; i < MAX_MONITOR_STAGE; i++) {
            t_stage = cur->stage_time[i];
            t_stage_cnt = cur->stage_count[i];
            t_stage = jiffies_to_usecs(t_stage);
            t_perf = do_divide(t_stage * 1000, t_stage_cnt);
            if (i == 0) {
                len +=
                    sprintf(buffer + len, "             s%d: %llu (%llu %llu)",
                            i, t_perf, t_stage, t_stage_cnt);
            } else {
                //len += sprintf(buffer + len, " s%d: %llu",
                //        i, cur->stage_time[i]*1000 / t_all);
                len += sprintf(buffer + len, " s%d: %llu (%llu %llu)",
                               i, t_perf, t_stage, t_stage_cnt);
            }
        }
        len += sprintf(buffer + len, "\n");
    }

    return len;
}
#endif

int32_t show_thd_perf_monitor(int8_t * buffer, int32_t buff_size)
{
    int32_t len, size;
#ifdef ENABLE_THD_PERF_MON
    int32_t i;
#endif

    len = 0;
    size = buff_size;

#ifndef ENABLE_THD_PERF_MON
    len += sprintf(buffer + len, "Thread Performance monitor not enable\n");
#else

    for (i = 0; i < LFSM_THD_PERFMON_END; i++) {
        read_lock(&thdPerfMon_tbl.thd_perfMon_tbllock);
        len += _show_thdperf_type_summary(buffer + len, size,
                                          &thdPerfMon_tbl.
                                          lhead_perfMon_traversal[i]);
        len +=
            _show_thdperf_type_one(buffer + len, size,
                                   &thdPerfMon_tbl.lhead_perfMon_traversal[i]);
        size = buff_size - len;
        read_unlock(&thdPerfMon_tbl.thd_perfMon_tbllock);

        if (size < 80) {
            break;
        }
    }
#endif

    return len;
}

void exec_thd_perfMon_cmdstr(int8_t * buffer, int32_t buff_size)
{
    if (strncmp(buffer, "clear", 5) == 0) {
        clear_thd_perf_value();
    } else {
        LOGINFO("PERF MON unknown command: %s\n", buffer);
    }
}
