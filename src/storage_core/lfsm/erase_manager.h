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
#ifndef _ERASEMGR_INC_509960448
#define _ERASEMGR_INC_509960448

#define ERSMGR_ENABLE_THRESHOLD (MIN_RESERVE_EU-1)
#define ERSMGR_MAX_ERASE_NUM (MIN_RESERVE_EU)

extern int32_t gcn_free_total;
enum erase_state { ERASE_SUBMIT_FAIL = -2, LOG_FAIL = -1, DIRTY = 0, EU_PICK =
        1, LOG_SUBMIT = 2, LOG_OK = 3, ERASE_SUBMIT = 4, ERASE_DONE = 5
};

typedef struct ersmgr {
    wait_queue_head_t wq;
    wait_queue_head_t wq_item;
    struct task_struct *pIOthread;
} ersmgr_t;

typedef struct sersmgr_item {
    struct EUProperty *peu;     //=NULL;//[MAX_ERASE_NUM*LFSM_DISK_NUM], ;
    struct bio_container *ar_pbioc[2];  //=NULL;//[MAX_ERASE_NUM*LFSM_DISK_NUM*2];
    devio_fn_t cb;              //=NULL;//[MAX_ERASE_NUM*LFSM_DISK_NUM];
//    int32_t err[MAX_ERASE_NUM*LFSM_DISK_NUM]={0}; // err=0:all success, 1:updatelog fail, <0:erase fail
} sersmgr_item_t;

extern int32_t ersmgr_init(ersmgr_t * pemgr);
extern void ersmgr_destroy(ersmgr_t * pemgr);
extern bool ersmgr_createthreads(ersmgr_t * pemgr, void *arg);

extern void ersmgr_wakeup_worker(ersmgr_t * pemgr);
extern int32_t ersmgr_pre_erase(lfsm_dev_t * td);
extern int32_t ersmgr_result_handler(lfsm_dev_t * td, int32_t err,
                                     struct EUProperty *eu);

#endif
