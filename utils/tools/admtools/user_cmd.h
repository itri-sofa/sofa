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

#ifndef USER_CMD_H_
#define USER_CMD_H_

typedef enum {
    CMD_OP_CONFIG,
    CMD_OP_LFSM,
    CMD_OP_END,
} ucmd_opcode_t;

typedef enum {
    lfsm_subop_getPhysicalCap,
    lfsm_subop_getCapacity,
    lfsm_subop_getgroups,
    lfsm_subop_getgroupdisks,
    lfsm_subop_getsparedisks,
    lfsm_subop_getDiskSMART,
    lfsm_subop_getGcInfo,
    lfsm_subop_getperf,
    lfsm_subop_getstatus,
    lfsm_subop_getproperties,
    lfsm_subop_activate,
    lfsm_subop_setgcratio,
    lfsm_subop_setgcreserve,
    lfsm_subop_end,
} lfsm_subop_t;

typedef enum {
    cfg_subop_updatefile,
    cfg_subop_update_append_file,
    cfg_subop_end,
} cfg_subop_t;

typedef struct user_cmd_common {
    uint16_t opcode;
    uint16_t subopcode;
    uint32_t tx_id;
    uint16_t result;
} user_cmd_t;

//add lfsm

//need move to lfsm_cmd.h and include here?
#define SMART_INSIZE 8
#define SMART_OUTSIZE 8192
#define SPARE_OUTSIZE 1024
#define GROUPDISK_OUTSIZE 1024
#define PERF_OUTSIZE 4096
#define MAX_DEV_PATH 16

typedef struct gcInfo {
    uint32_t gcSeuResv;
    uint64_t gcInfoTest64;
    uint32_t gcInfoTest;
} gcInfo_t;

typedef struct gcInfo_cmd {
    user_cmd_t cmd;
    gcInfo_t gcInfo;
} lfsm_gcInfo_cmd_t;

typedef struct diskSMART_cmd {
    user_cmd_t cmd;
    int8_t diskName[SMART_INSIZE];
    int8_t msginfo[SMART_OUTSIZE];
} lfsm_diskSMART_cmd_t;

typedef struct sparedisks_cmd {
    user_cmd_t cmd;
    int8_t ret_sparedisks[SPARE_OUTSIZE];
} lfsm_sparedisks_cmd_t;

typedef struct groupdisks_cmd {
    user_cmd_t cmd;
    uint32_t groupId;
    int8_t ret_groupdisks[GROUPDISK_OUTSIZE];
} lfsm_groupdisks_cmd_t;

typedef struct groups_cmd {
    user_cmd_t cmd;
    uint32_t ret_groups;
} lfsm_groups_cmd_t;

typedef struct capacity_cmd {
    user_cmd_t cmd;
    uint64_t ret_cap;
} lfsm_capacity_cmd_t;

typedef struct physicalCap_cmd {
    user_cmd_t cmd;
    uint64_t ret_phycap;
} lfsm_physicalCap_cmd_t;

typedef struct perf_cmd {
    user_cmd_t cmd;
    uint32_t request_size;
    int8_t ret_perf[PERF_OUTSIZE];
} lfsm_perf_cmd_t;

//end lfsm

/** config related start **/
typedef struct cfg_updatefile_cmd {
    user_cmd_t cmd;
    int8_t *cfgfile;
    int8_t *itemName;
    int8_t *itemVal;
    int8_t *itemSet;
} cfg_ufile_cmd_t;
/** config related end **/

#endif /* USER_CMD_H_ */
