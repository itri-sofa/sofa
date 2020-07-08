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

#ifndef DRV_LFSMCMD_HANDLER_PRIVATE_H_
#define DRV_LFSMCMD_HANDLER_PRIVATE_H_

#define SMART_OUTSIZE 8192
#define SPARE_OUTSIZE 1024
#define GROUPDISK_OUTSIZE 1024

typedef enum {
    getPhysicalCap_OK,
    getPhysicalCap_MiscErr,
} getPhysicalCap_resp_t;

typedef enum {
    getCapacity_OK,
    getCapacity_MiscErr,
} getCapacity_resp_t;

typedef enum {
    getgroups_OK,
    getgroups_MiscErr,
} getgroups_resp_t;

typedef enum {
    getgroupdisks_OK,
    getgroupdisks_MiscErr,
} getgroupdisks_resp_t;

typedef enum {
    getsparedisks_OK,
    getsparedisks_MiscErr,
} getsparedisks_resp_t;

typedef enum {
    getGcInfo_OK,
    getGcInfo_MiscErr,
} getGcInfo_resp_t;

typedef enum {
    getperf_OK,
    getperf_MiscErr,
} getperf_resp_t;

typedef struct gcInfo {
    uint32_t gcSeuResv;
    uint64_t gcInfoTest64;
    uint32_t gcInfoTest;
} gcInfo_t;

typedef struct perfInfo {
    uint64_t cal_request_iops;
    uint64_t cal_respond_iops;
    uint64_t cal_avg_latency;
    uint64_t cal_free_list;
} perfInfo_t;

#endif /* DRV_LFSMCMD_HANDLER_PRIVATE_H_ */
