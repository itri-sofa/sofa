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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <syslog.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include "userD_common_def.h"
#include "../common/drv_userD_common.h"
#include "../common/sofa_mm.h"
#include "../common/userspace_list.h"
#include "calculate_perf.h"
#include "calculate_perf_private.h"
#include "userD_main.h"
#include "lfsm_req_handler.h"

#if 0
static void remove_first_node(void)
{
    cal_perf_t *cur, *next;
    list_for_each_entry_safe(cur, next, &perf_glist, list) {
        syslog(LOG_INFO, "SOFA INFO %s: &cur->list = %p , cur = %p\n",
               __FUNCTION__, &cur->list, cur);
        list_del(&cur->list);
        sofa_free(cur);
        break;
    }
}
#endif

static void remove_tail_node(void)
{
    cal_perf_t *tail;

    tail = NULL;

    if (!list_empty(&perf_glist)) {
        tail = (cal_perf_t *) perf_glist.prev;
        list_del(&tail->list);
        sofa_free(tail);
    }
}

static void remove_all_list_node(void)
{
    cal_perf_t *cur, *next;
    list_for_each_entry_safe(cur, next, &perf_glist, list) {
        list_del(&cur->list);
        sofa_free(cur);
    }
}

#if 0
static void cal_req_iops(cal_perf_t * item)
{
    int8_t cmd[CMDSIZE];
    int8_t searchCmd[CMDSIZE];
    int8_t *cmdBuf, *ptr1;
    uint32_t cmdBufSize;
    uint64_t all_io_cnt;

    cmdBufSize = PERF_OUTSIZE;
    all_io_cnt = 0;

    cmdBuf = sofa_malloc(PERF_OUTSIZE);

    syslog(LOG_INFO, "SOFA INFO calculate start\n");

    snprintf(searchCmd, sizeof(searchCmd), "(");
    snprintf(cmd, sizeof(cmd),
             "cat /proc/lfsm/perf_monitor/thread_perf | grep \"Summary: lfsm_iod\" | awk '{print $7}'");
    //syslog(LOG_INFO, "%s\n", cmd);
    ptr1 = CmdSearch(cmd, searchCmd, cmdBuf, cmdBufSize);
    if (ptr1 != NULL) {         //find string

        all_io_cnt = atoi(ptr1 + 1);
        if (g_req_io_cn_last == 0) {
            item->request_iops = 0; //first time
        } else {
            item->request_iops =
                (all_io_cnt - g_req_io_cn_last) / PROBE_INTERVAL;
        }

        g_req_io_cn_last = all_io_cnt;

        //syslog(LOG_INFO, "finally all_io_cnt = %llu, item->request_iops=%llu\n",
        all_io_cnt, item->request_iops);

    } else {
        syslog(LOG_INFO, "%s: Failed !!", __FUNCTION__);
    }

    sofa_free(cmdBuf);
}

static void cal_resp_iops(cal_perf_t * item) {
    int8_t cmd[CMDSIZE];
    int8_t searchCmd[CMDSIZE];
    int8_t *cmdBuf, *ptr1, *pch;
    uint32_t cmdBufSize, i;
    uint64_t all_io_cnt;
    int8_t *delim = "\n";

    cmdBufSize = PERF_OUTSIZE;
    all_io_cnt = 0;
    i = 0;

    cmdBuf = sofa_malloc(PERF_OUTSIZE);

    syslog(LOG_INFO, "SOFA INFO calculate start\n");

    snprintf(searchCmd, sizeof(searchCmd), "(");
    snprintf(cmd, sizeof(cmd),
             "cat /proc/lfsm/perf_monitor/ssd_perf | awk '{print $7}'");
    //syslog(LOG_INFO, "%s\n", cmd);
    ptr1 = CmdSearch(cmd, searchCmd, cmdBuf, cmdBufSize);
    if (ptr1 != NULL) {         //find string

        pch = strtok(cmdBuf, delim);

        while (pch != NULL) {
            //syslog(LOG_INFO, "%s\n", pch);
            all_io_cnt += atoi(pch + 1);
            pch = strtok(NULL, delim);
        }

        if (g_resp_io_cn_last == 0) {
            item->respond_iops = 0; //first time
        } else {
            item->respond_iops =
                (all_io_cnt - g_resp_io_cn_last) / PROBE_INTERVAL;
        }

        g_resp_io_cn_last = all_io_cnt;

        //syslog(LOG_INFO, "finally all_io_cnt = %llu, item->respond_iops=%llu\n",
        all_io_cnt, item->respond_iops);

    } else {
        syslog(LOG_INFO, "%s: Failed !!", __FUNCTION__);
    }

    sofa_free(cmdBuf);
}
#endif

static void cal_iops_lfsm(cal_perf_t * item) {
    uint64_t all_resp_cnt;
    uint64_t all_req_cnt;
    perfInfo_t *perfInfo;
    usercmd_para_t para;

    int32_t ret;
    para.opcode = CMD_OP_LFSM;
    para.subopcode = lfsm_subop_getperf;
    ret = ioctl(fd_chrDev, IOCTL_USER_CMD, (unsigned long)(&para));
    if (ret) {                  //
        syslog(LOG_INFO, "%s: Failed !!", __FUNCTION__);
    } else {

        if (para.cmd_result) {  //TODO: compare with return code
            syslog(LOG_INFO,
                   "SOFA INFO lfsm cal_resp_iops_lfsm para.cmd_result = %d\n",
                   para.cmd_result);
        } else {
            perfInfo = (perfInfo_t *) para.cmd_resp;
            //syslog(LOG_INFO, "SOFA INFO lfsm perfInfo.cal_respond_iops = %llu\n",
            //perfInfo->cal_respond_iops);
        }

        all_resp_cnt = perfInfo->cal_respond_iops;

        if (g_resp_io_cn_last == 0) {
            item->respond_iops = 0; //first time
        } else {
            item->respond_iops =
                (all_resp_cnt - g_resp_io_cn_last) / PROBE_INTERVAL;
        }

        g_resp_io_cn_last = all_resp_cnt;

        //syslog(LOG_INFO, "finally all_resp_cnt = %llu, item->respond_iops=%llu\n",
        //all_resp_cnt, item->respond_iops);

        all_req_cnt = perfInfo->cal_request_iops;

        if (g_req_io_cn_last == 0) {
            item->request_iops = 0; //first time
        } else {
            item->request_iops =
                (all_req_cnt - g_req_io_cn_last) / PROBE_INTERVAL;
        }

        if (item->request_iops != 0) {
            item->avg_latency = 1000000 / item->request_iops;
        }
        g_req_io_cn_last = all_req_cnt;

//        syslog(LOG_INFO,
//                "all_req_cnt = %llu, item->request_iops=%llu, item->avg_latency=%llu\n",
//                all_req_cnt, item->request_iops, item->avg_latency);
    }

}

static void cal_freelist(cal_perf_t * item) {
    int8_t cmd[CMDSIZE];
    int8_t searchCmd[CMDSIZE];
    int8_t *cmdBuf, *ptr1, *pch;
    uint32_t cmdBufSize;
    uint64_t all_cnt;
    int8_t *delim = "\n";

    cmdBufSize = PERF_OUTSIZE;
    all_cnt = 0;

    cmdBuf = sofa_malloc(PERF_OUTSIZE);

    //syslog(LOG_INFO, "SOFA INFO cal_freelist start\n");

    snprintf(searchCmd, sizeof(searchCmd), ",");
    snprintf(cmd, sizeof(cmd),
             "cat /proc/lfsm/listall | grep cn_free | awk '{print $5}'");
    //syslog(LOG_INFO, "%s\n", cmd);
    ptr1 = CmdSearch(cmd, searchCmd, cmdBuf, cmdBufSize);
    if (ptr1 != NULL) {         //find string

        pch = strtok(cmdBuf, delim);

        while (pch != NULL) {
            //syslog(LOG_INFO, "%s\n", pch);
            all_cnt += atoi(pch);
            pch = strtok(NULL, delim);
        }

        item->free_list = all_cnt;

        //syslog(LOG_INFO, "finally item->free_list=%llu\n", all_cnt, item->free_list);

    } else {
        syslog(LOG_INFO, "%s: Failed !!", __FUNCTION__);
    }

    sofa_free(cmdBuf);
}

#if 0
static void cal_latency(cal_perf_t * item) {
    item->avg_latency = 0;
}
#endif

static void init_cal_item(cal_perf_t * item) {
    INIT_LIST_HEAD(&item->list);
    item->request_iops = 0;
    item->respond_iops = 0;
    item->avg_latency = 0;
    item->free_list = 0;
    item->timestamp = 0;
}

static void cal_handler(void) {
    cal_perf_t *item;
    item = (cal_perf_t *) sofa_malloc(sizeof(cal_perf_t));

    init_cal_item(item);

    pthread_mutex_lock(&cal_thread_lock);

    //cal_req_iops(item);
    //cal_resp_iops(item);
    //cal_latency(item);
    cal_iops_lfsm(item);
    cal_freelist(item);

    item->timestamp = (uint64_t) time(NULL);

    if (item->request_iops < 0 || item->respond_iops < 0) {
        syslog(LOG_ERR,
               "[SOFA] USERD WARN cal_handler err!, item->request_iops = %lld, "
               "item->respond_iops = %lld\n", item->request_iops,
               item->respond_iops);
    } else {
        if (g_perf_glist_size < PERF_LIST_SIZE) {
            list_add(&item->list, &perf_glist);
            g_perf_glist_size++;
        } else {
            list_add(&item->list, &perf_glist);
            remove_tail_node();
        }
    }

    pthread_mutex_unlock(&cal_thread_lock);
}

void create_perf_thread(void) {
    pthread_attr_t attr;
    pthread_t cal_pid;
    int32_t ret;

    //syslog(LOG_INFO, "[SOFA] USERD INFO user daemon create_perf_thread \n");

    INIT_LIST_HEAD(&perf_glist);
    g_cal_perf_running = 1;

    while (g_cal_perf_running) {

        pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

        ret = pthread_create(&cal_pid, &attr, (void *)cal_handler, NULL);

        if (ret != 0) {
            syslog(LOG_ERR,
                   "[SOFA] USERD WARN calculate perf create pthread err!\n");
        }

        pthread_attr_destroy(&attr);

        sleep(PROBE_INTERVAL);
    }
}

void stop_perf_thread(void) {
    pthread_mutex_lock(&cal_thread_lock);
    g_cal_perf_running = 0;
    remove_all_list_node();
    g_perf_glist_size = 0;
    pthread_mutex_unlock(&cal_thread_lock);
}

void get_perf_data(void *replyBuf, uint32_t request_length) {
    cal_perf_t *cur, *next;
    uint32_t len;

    len = 0;

    //limit request length
    if (request_length > MAX_REQ_LEN) {
        request_length = MAX_REQ_LEN;
        syslog(LOG_INFO, "SOFA INFO upto MAX_REQ_LEN %u\n", request_length);
    }

    if (request_length > g_perf_glist_size) {
        request_length = g_perf_glist_size;
    }

    pthread_mutex_unlock(&cal_thread_lock);

    list_for_each_entry_safe(cur, next, &perf_glist, list) {
        len += sprintf(replyBuf + len, "%lld;%lld;%llu;%llu;%llu;",
                       cur->request_iops, cur->respond_iops, cur->avg_latency,
                       cur->free_list, cur->timestamp);
        request_length--;
        if (request_length == 0) {
            break;
        }
    }

    pthread_mutex_unlock(&cal_thread_lock);
}
