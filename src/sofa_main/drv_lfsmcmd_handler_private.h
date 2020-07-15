/*
 * SPDX-License-Identifier: GPL-2.0-or-later OR Apache-2.0
 * Copyright (C) 2015-2020 Industrial Technology Research Institute
 *
 *
 *
 *  
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
