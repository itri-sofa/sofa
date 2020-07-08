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

#ifndef STORAGE_CORE_LFSM_PERF_MONITOR_IOREQ_PENDSTAGE_MON_H_
#define STORAGE_CORE_LFSM_PERF_MONITOR_IOREQ_PENDSTAGE_MON_H_

//#define ENABLE_IOR_STAGE_MON

typedef enum {
    IOR_STAGE_LFSM_RCV,
    IOR_STAGE_WAIT_IOT,
    IOR_STAGE_PROCESS_IOT,
    IOR_STAGE_SUBMIT_SSD,
    IOR_STAGE_NO_METADATA_RESP,
    IOR_STAGE_SSD_RESP,
    IOR_STAGE_WAIT_BHT,
    IOR_STAGE_PROCESS_BHT,
    IOR_STAGE_LFSMIOT_PROCESS_DONE,
    IOR_STAGE_LFSMBH_PROCESS_DONE,
} ioreq_stage_t;

typedef struct ioreq_process_stage_mon {
    uint64_t *ioreqs_rcv_all;   //bio received by lfsm, protect this by biobox_pool_lock of bio request queue lock
    int32_t *ioreqs_wait_iot;   //We protect this by biobox_pool_lock of bio request queue lock
    int32_t *ioreqs_process_iot;    //Each bio io handler thread has it's own iot counting
    uint64_t *ioreqs_submit_ssd;    //Each bio handler has a corresponding submit counter
    uint64_t *ioreqs_NoMD_done; //Each bio handler has a corresponding read IO without metadata counter
    uint64_t *ioreqs_ssd_resp;  //We protect this by using lock of ar_lock_biocbh of biocbh_t
    int32_t *ioreqs_wait_bht;   //We protect this by using lock of ar_lock_biocbh of biocbh_t
    int32_t *ioreqs_process_bht;    //Each biocbh thread has it's own bht counting
    uint64_t *ioreqs_iot_done;  //bio request finished by io thread
    uint64_t *ioreqs_bh_done;   //bio request finished by bh thread
} IOReq_PS_mon_t;

extern inline void add_ioreq_stage_count(ioreq_stage_t stage, int32_t index);
extern inline void dec_ioreq_stage_count(ioreq_stage_t stage, int32_t index);

extern int32_t show_IOR_pStage_mon(int8_t * buffer);
extern void exec_IOR_pStageMon_cmdstr(int8_t * buffer, int32_t buff_size);
extern void init_IOR_pStage_mon(void);
extern void rel_IOR_pStage_mon(void);

#ifdef ENABLE_IOR_STAGE_MON
#define INC_IOR_STAGE_CNT(stage, index) add_ioreq_stage_count(stage, index)
#define DEC_IOR_STAGE_CNT(stage, index) dec_ioreq_stage_count(stage, index)
#else
#define INC_IOR_STAGE_CNT(stage, index) {}
#define DEC_IOR_STAGE_CNT(stage, index) {}
#endif

#endif /* STORAGE_CORE_LFSM_PERF_MONITOR_IOREQ_PENDSTAGE_MON_H_ */
