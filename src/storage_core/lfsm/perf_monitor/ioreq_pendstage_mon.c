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
#include <linux/slab.h>         // kmalloc
#include "../common/common.h"
#include "../common/mem_manager.h"
#include "ioreq_pendstage_mon.h"
#include "ioreq_pendstage_mon_private.h"
#include "../config/lfsm_cfg.h"

int32_t show_IOR_pStage_mon(int8_t * buffer)
{
    int32_t len;
#ifdef ENABLE_IOR_STAGE_MON
    uint64_t rcvall, iotdone, bhdone;
    uint64_t all2ssd, all_noMDIO, all_ssd_resp;
    int32_t all_bht_process, all_bht_wait, i;
    int32_t all_iot_process, all_iot_wait;
#endif

    len = 0;

#ifndef ENABLE_IOR_STAGE_MON
    len += sprintf(buffer + len, "IOReq stage monitor not enable\n");
#else

    all_iot_wait = 0;
    all_iot_process = 0;
    all2ssd = 0L;
    all_noMDIO = 0L;
    rcvall = 0L;
    iotdone = 0L;
    for (i = 0; i < lfsmCfg.cnt_io_threads; i++) {
        rcvall += IORPS_Mon.ioreqs_rcv_all[i];
        iotdone += IORPS_Mon.ioreqs_iot_done[i];
        all_iot_wait += IORPS_Mon.ioreqs_wait_iot[i];
        all_iot_process += IORPS_Mon.ioreqs_process_iot[i];
        all2ssd += IORPS_Mon.ioreqs_submit_ssd[i];
        all_noMDIO += IORPS_Mon.ioreqs_NoMD_done[i];
    }

    all_bht_process = 0;
    all_bht_wait = 0;
    all_ssd_resp = 0L;
    bhdone = 0L;
    for (i = 0; i < lfsmCfg.cnt_bh_threads; i++) {
        all_bht_process += IORPS_Mon.ioreqs_process_bht[i];
        all_bht_wait += IORPS_Mon.ioreqs_wait_bht[i];
        all_ssd_resp += IORPS_Mon.ioreqs_ssd_resp[i];
        bhdone += IORPS_Mon.ioreqs_bh_done[i];
    }

    len += sprintf(buffer + len, "IOR stage count "
                   "(total, wait_iot, iot process, wait ssd, wait bh, bht process) = "
                   "(%lld %d %d %lld %d %d)\n",
                   rcvall - iotdone - bhdone,
                   all_iot_wait, all_iot_process,
                   all2ssd - all_noMDIO - all_ssd_resp, all_bht_wait,
                   all_bht_process);
#endif

    return len;
}

static void _reset_IOR_pStage_mon(void)
{
    int32_t i;

    for (i = 0; i < lfsmCfg.cnt_io_threads; i++) {
        IORPS_Mon.ioreqs_rcv_all[i] = 0L;
        IORPS_Mon.ioreqs_iot_done[i] = 0L;
        IORPS_Mon.ioreqs_wait_iot[i] = 0;
        IORPS_Mon.ioreqs_process_iot[i] = 0;
        IORPS_Mon.ioreqs_submit_ssd[i] = 0L;
        IORPS_Mon.ioreqs_NoMD_done[i] = 0L;
    }

    for (i = 0; i < lfsmCfg.cnt_bh_threads; i++) {
        IORPS_Mon.ioreqs_ssd_resp[i] = 0L;
        IORPS_Mon.ioreqs_wait_bht[i] = 0;
        IORPS_Mon.ioreqs_process_bht[i] = 0;
        IORPS_Mon.ioreqs_bh_done[i] = 0L;
    }
}

void exec_IOR_pStageMon_cmdstr(int8_t * buffer, int32_t buff_size)
{
    if (strncmp(buffer, "clear", strlen("clear"))) {
        _reset_IOR_pStage_mon();
    } else {
        LOGINFO("IOReq stage monitor: unknown command: %s\n", buffer);
    }
}

inline void add_ioreq_stage_count(ioreq_stage_t stage, int32_t index)
{
    switch (stage) {
    case IOR_STAGE_LFSM_RCV:
        IORPS_Mon.ioreqs_rcv_all[index]++;
        break;
    case IOR_STAGE_LFSMIOT_PROCESS_DONE:
        IORPS_Mon.ioreqs_iot_done[index]++;
        break;
    case IOR_STAGE_WAIT_IOT:
        IORPS_Mon.ioreqs_wait_iot[index]++;
        break;
    case IOR_STAGE_PROCESS_IOT:
        IORPS_Mon.ioreqs_process_iot[index]++;
        break;
    case IOR_STAGE_SUBMIT_SSD:
        IORPS_Mon.ioreqs_submit_ssd[index]++;
        break;
    case IOR_STAGE_NO_METADATA_RESP:
        IORPS_Mon.ioreqs_NoMD_done[index]++;
        break;
    case IOR_STAGE_SSD_RESP:
        IORPS_Mon.ioreqs_ssd_resp[index]++;
        break;
    case IOR_STAGE_WAIT_BHT:
        IORPS_Mon.ioreqs_wait_bht[index]++;
        break;
    case IOR_STAGE_PROCESS_BHT:
        IORPS_Mon.ioreqs_process_bht[index]++;
        break;
    case IOR_STAGE_LFSMBH_PROCESS_DONE:
        IORPS_Mon.ioreqs_bh_done[index]++;
        break;
    default:
        LOGINFO("IOReq stage monitor: unknoen process stage %d\n", stage);
        break;
    }
}

inline void dec_ioreq_stage_count(ioreq_stage_t stage, int32_t index)
{
    switch (stage) {
    case IOR_STAGE_LFSM_RCV:
    case IOR_STAGE_LFSMIOT_PROCESS_DONE:
    case IOR_STAGE_LFSMBH_PROCESS_DONE:
        break;
    case IOR_STAGE_WAIT_IOT:
        IORPS_Mon.ioreqs_wait_iot[index]--;
        break;
    case IOR_STAGE_PROCESS_IOT:
        IORPS_Mon.ioreqs_process_iot[index]--;
        break;
    case IOR_STAGE_SUBMIT_SSD:
    case IOR_STAGE_NO_METADATA_RESP:
    case IOR_STAGE_SSD_RESP:
        break;
    case IOR_STAGE_WAIT_BHT:
        IORPS_Mon.ioreqs_wait_bht[index]--;
        break;
    case IOR_STAGE_PROCESS_BHT:
        IORPS_Mon.ioreqs_process_bht[index]--;
        break;
    default:
        LOGINFO("IOReq unknoen process stage %d\n", stage);
        break;
    }
}

void init_IOR_pStage_mon(void)
{
    IORPS_Mon.ioreqs_rcv_all =
        track_kmalloc(sizeof(uint64_t) * lfsmCfg.cnt_io_threads, GFP_KERNEL,
                      M_PERF_MON);
    IORPS_Mon.ioreqs_iot_done =
        track_kmalloc(sizeof(uint64_t) * lfsmCfg.cnt_io_threads, GFP_KERNEL,
                      M_PERF_MON);
    IORPS_Mon.ioreqs_process_iot =
        track_kmalloc(sizeof(int32_t) * lfsmCfg.cnt_io_threads, GFP_KERNEL,
                      M_PERF_MON);
    IORPS_Mon.ioreqs_wait_iot =
        track_kmalloc(sizeof(int32_t) * lfsmCfg.cnt_io_threads, GFP_KERNEL,
                      M_PERF_MON);
    IORPS_Mon.ioreqs_submit_ssd =
        track_kmalloc(sizeof(uint64_t) * lfsmCfg.cnt_io_threads, GFP_KERNEL,
                      M_PERF_MON);
    IORPS_Mon.ioreqs_NoMD_done =
        track_kmalloc(sizeof(uint64_t) * lfsmCfg.cnt_io_threads, GFP_KERNEL,
                      M_PERF_MON);

    IORPS_Mon.ioreqs_ssd_resp =
        track_kmalloc(sizeof(uint64_t) * lfsmCfg.cnt_bh_threads, GFP_KERNEL,
                      M_PERF_MON);
    IORPS_Mon.ioreqs_wait_bht =
        track_kmalloc(sizeof(int32_t) * lfsmCfg.cnt_bh_threads, GFP_KERNEL,
                      M_PERF_MON);
    IORPS_Mon.ioreqs_process_bht =
        track_kmalloc(sizeof(int32_t) * lfsmCfg.cnt_bh_threads, GFP_KERNEL,
                      M_PERF_MON);
    IORPS_Mon.ioreqs_bh_done =
        track_kmalloc(sizeof(uint64_t) * lfsmCfg.cnt_bh_threads, GFP_KERNEL,
                      M_PERF_MON);
    _reset_IOR_pStage_mon();
}

void rel_IOR_pStage_mon(void)
{
    track_kfree(IORPS_Mon.ioreqs_rcv_all,
                sizeof(uint64_t) * lfsmCfg.cnt_io_threads, M_PERF_MON);
    track_kfree(IORPS_Mon.ioreqs_iot_done,
                sizeof(uint64_t) * lfsmCfg.cnt_io_threads, M_PERF_MON);
    track_kfree(IORPS_Mon.ioreqs_process_iot,
                sizeof(int32_t) * lfsmCfg.cnt_io_threads, M_PERF_MON);
    track_kfree(IORPS_Mon.ioreqs_wait_iot,
                sizeof(int32_t) * lfsmCfg.cnt_io_threads, M_PERF_MON);
    track_kfree(IORPS_Mon.ioreqs_submit_ssd,
                sizeof(uint64_t) * lfsmCfg.cnt_io_threads, M_PERF_MON);
    track_kfree(IORPS_Mon.ioreqs_NoMD_done,
                sizeof(uint64_t) * lfsmCfg.cnt_io_threads, M_PERF_MON);

    track_kfree(IORPS_Mon.ioreqs_ssd_resp,
                sizeof(uint64_t) * lfsmCfg.cnt_bh_threads, M_PERF_MON);
    track_kfree(IORPS_Mon.ioreqs_wait_bht,
                sizeof(int32_t) * lfsmCfg.cnt_bh_threads, M_PERF_MON);
    track_kfree(IORPS_Mon.ioreqs_process_bht,
                sizeof(int32_t) * lfsmCfg.cnt_bh_threads, M_PERF_MON);
    track_kfree(IORPS_Mon.ioreqs_bh_done,
                sizeof(uint64_t) * lfsmCfg.cnt_bh_threads, M_PERF_MON);
}
