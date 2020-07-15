/*
 * SPDX-License-Identifier: GPL-2.0-or-later OR Apache-2.0
 * Copyright (C) 2015-2020 Industrial Technology Research Institute
 *
 *
 *
 *  
 */
#include <linux/types.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/list.h>
#include "../common/common.h"
#include "../common/tools.h"
#include "ssd_perf_mon_private.h"
#include "ssd_perf_mon.h"

void log_ssd_perf_time(int32_t disk_id, int32_t rw, int64_t time_s,
                       int64_t time_e)
{
    diskpm_item_t *pmItem;
    int64_t t_dur;

    if (disk_id >= MAX_NUM_DISK) {
        return;
    }

    t_dur = time_e - time_s;

    if (t_dur < 0) {
        return;
    }

    pmItem = &diskPM_tbl[disk_id];
    if (rw) {
        pmItem->cnt_proc_w++;
        pmItem->time_proc_w += t_dur;
    } else {
        pmItem->cnt_proc_r++;
        pmItem->time_proc_r += t_dur;
    }
}

static void _init_ssd_pmItem(diskpm_item_t * pmItem)
{
    pmItem->cnt_proc_r = 0;
    pmItem->cnt_proc_w = 0;
    pmItem->disk_id = -1;
    memset(pmItem->disk_name, '\0', LEN_DISK_NAME);
    pmItem->time_proc_r = 0;
    pmItem->time_proc_w = 0;
}

void dereg_ssd_perf_monitor(int32_t disk_id)
{
    if (disk_id >= MAX_NUM_DISK) {
        return;
    }

    _init_ssd_pmItem(&diskPM_tbl[disk_id]);
}

void reg_ssd_perf_monitor(int32_t disk_id, int8_t * diskName)
{
    if (disk_id >= MAX_NUM_DISK) {
        return;
    }

    diskPM_tbl[disk_id].disk_id = disk_id;
    strcpy(diskPM_tbl[disk_id].disk_name, diskName);
}

void init_ssd_perf_monitor(int32_t total_ssds)
{
    int32_t i;

    for (i = 0; i < MAX_NUM_DISK; i++) {
        _init_ssd_pmItem(&diskPM_tbl[i]);
    }

    total_ssds_pm = total_ssds;
}

void exec_ssd_perfMon_cmdstr(int8_t * buffer, int32_t buff_size)
{
    if (strncmp(buffer, "clear", 5) == 0) {
        init_ssd_perf_monitor(total_ssds_pm);
    } else {
        LOGINFO("PERF MON unknown command: %s\n", buffer);
    }
}

#ifdef ENABLE_SSD_PERF_MON
static int32_t _show_ssd_perf(int8_t * buffer, int32_t buff_size,
                              diskpm_item_t * pmItem)
{
    int32_t len;
    uint64_t cnt_all, t_all, cnt_read, t_read, cnt_write, t_write;

    len = 0;

    cnt_read = pmItem->cnt_proc_r;
    cnt_write = pmItem->cnt_proc_w;
    cnt_all = cnt_read + cnt_write;
    t_read = jiffies_to_usecs(pmItem->time_proc_r);
    t_write = jiffies_to_usecs(pmItem->time_proc_w);
    t_all = t_read + t_write;

    len +=
        sprintf(buffer + len,
                "%02d sd%s avg latency: %llu us (%llu %llu us) "
                "read: %llu (%llu %llu) write %llu (%llu %llu)\n",
                pmItem->disk_id, pmItem->disk_name, do_divide(t_all, cnt_all),
                cnt_all, t_all, do_divide(t_read, cnt_read), cnt_read, t_read,
                do_divide(t_write, cnt_write), cnt_write, t_write);

    return len;
}
#endif

int32_t show_ssd_perf_monitor(int8_t * buffer, int32_t buff_size)
{
    int32_t len, size;
#ifdef ENABLE_SSD_PERF_MON
    int32_t i;
#endif

    len = 0;
    size = buff_size;

#ifndef ENABLE_SSD_PERF_MON
    len += sprintf(buffer + len, "SSD Performance monitor not enable\n");
#else

    for (i = 0; i < total_ssds_pm; i++) {
        len += _show_ssd_perf(buffer + len, size, &diskPM_tbl[i]);
        size = buff_size - len;

        if (size < 80) {
            break;
        }
    }
#endif

    return len;
}
