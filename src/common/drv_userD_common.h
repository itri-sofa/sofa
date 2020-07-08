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

#ifndef DRV_USERD_COMMON_H_
#define DRV_USERD_COMMON_H_

#define SOFA_CHRDEV_Name    "chr_sofa"
#define SOFA_CHRDEV_MAJORNUM 151

#define IOCTL_CONFIG_DRV        _IOW(SOFA_CHRDEV_MAJORNUM,   1, unsigned long)
#define IOCTL_INIT_DRV          _IOW(SOFA_CHRDEV_MAJORNUM,   2, unsigned long)
#define IOCTL_USER_CMD          _IOWR(SOFA_CHRDEV_MAJORNUM,  3, unsigned long)

#define CFGPARA_NAME_LEN   64
#define CFGPARA_VAL_LEN   64
#define CFGPAGA_VALSET_LEN  128

#define CALCULATE_PERF_ENABLE

typedef struct cfg_para {
    int8_t name[CFGPARA_NAME_LEN];
    int8_t value[CFGPARA_VAL_LEN];
    int8_t valSet[CFGPAGA_VALSET_LEN];
    int32_t name_len;
    int32_t value_len;
    int32_t valSet_len;
} cfg_para_t;

#define USERCMD_PARA_SIZE   4096
#define USERCMD_RESP_SIZE   4096
typedef struct usercmd_para {
    uint16_t opcode;
    uint16_t subopcode;
    int32_t cmd_result;
    int8_t cmd_para[USERCMD_PARA_SIZE];
    int8_t cmd_resp[USERCMD_RESP_SIZE];
} usercmd_para_t;

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

#endif /* DRV_USERD_COMMON_H_ */
