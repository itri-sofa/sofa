/*
 * Copyright (c) 2015-2020 Industrial Technology Research Institute.
 *
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 *
 *
 *
 *
 *
 *
 *  
 */

#ifndef CALCULATE_PERF_PRIVATE_H_
#define CALCULATE_PERF_PRIVATE_H_

#define PROBE_INTERVAL 1        //second
#define PERF_LIST_SIZE 60       //1 min
#define CMDSIZE 128
#define PERF_OUTSIZE 4096
#define MAX_REQ_LEN 60

static uint64_t g_req_io_cn_last = 0;
static uint64_t g_resp_io_cn_last = 0;

typedef struct cal_perf {
    struct list_head list;
    int64_t request_iops;
    int64_t respond_iops;
    uint64_t avg_latency;
    uint64_t free_list;
    uint64_t timestamp;
} cal_perf_t;

typedef struct perfInfo {
    uint64_t cal_request_iops;
    uint64_t cal_respond_iops;
    uint64_t cal_avg_latency;
    uint64_t cal_free_list;
} perfInfo_t;

//cal_perf_t sofa_perf;
struct list_head perf_glist;
pthread_mutex_t cal_thread_lock = PTHREAD_MUTEX_INITIALIZER;
int32_t g_cal_perf_running = 0;
uint32_t g_perf_glist_size = 0;

#endif /* CALCULATE_PERF_PRIVATE_H_ */
