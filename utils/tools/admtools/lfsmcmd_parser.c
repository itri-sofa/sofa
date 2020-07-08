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
#include <syslog.h>
#include <sys/types.h>
#include <string.h>
#include <stdint.h>
#include "user_cmd.h"
#include "lfsmcmd_parser_private.h"
#include "lfsmcmd_parser.h"
#include "lfsmcmd_handler.h"

static user_cmd_t *_get_physicalCap_cmd (int32_t argc, int8_t **argv)
{
    lfsm_physicalCap_cmd_t *ucmd;

    if (argc < 3) {
        syslog(LOG_INFO, "[SOFA ADM] wrong para to _get_physicalCap_cmd\n");
        return NULL;
    }

    ucmd = (lfsm_physicalCap_cmd_t *)malloc(sizeof(lfsm_physicalCap_cmd_t));
    ucmd->cmd.opcode = CMD_OP_LFSM;
    ucmd->cmd.subopcode = lfsm_subop_getPhysicalCap;

    //syslog(LOG_INFO, "[SOFA ADM] user command is _get_physicalCap_cmd !!\n");
    return (user_cmd_t *)ucmd;
}

static user_cmd_t *_get_capacity_cmd (int32_t argc, int8_t **argv)
{
    lfsm_capacity_cmd_t *ucmd;

    if (argc < 3) {
        syslog(LOG_INFO, "[SOFA ADM] wrong para to _get_capacity_cmd\n");
        return NULL;
    }

    ucmd = (lfsm_capacity_cmd_t *)malloc(sizeof(lfsm_capacity_cmd_t));
    ucmd->cmd.opcode = CMD_OP_LFSM;
    ucmd->cmd.subopcode = lfsm_subop_getCapacity;

    //syslog(LOG_INFO, "[SOFA ADM] user command is _get_capacity_cmd !!\n");
    return (user_cmd_t *)ucmd;
}

static user_cmd_t *_get_groups_cmd (int32_t argc, int8_t **argv)
{
    lfsm_groups_cmd_t *ucmd;

    if (argc < 3) {
        syslog(LOG_INFO, "[SOFA ADM] wrong para to _get_groups_cmd\n");
        return NULL;
    }

    ucmd = (lfsm_groups_cmd_t *)malloc(sizeof(lfsm_groups_cmd_t));
    ucmd->cmd.opcode = CMD_OP_LFSM;
    ucmd->cmd.subopcode = lfsm_subop_getgroups;

    //syslog(LOG_INFO, "[SOFA ADM] user command is _get_groups_cmd !!\n");
    return (user_cmd_t *)ucmd;
}

static user_cmd_t *_get_groupdisks_cmd (int32_t argc, int8_t **argv)
{
    lfsm_groupdisks_cmd_t *ucmd;

    if (argc < 4) {
        syslog(LOG_INFO, "[SOFA ADM] wrong para to _get_groupdisks_cmd\n");
        return NULL;
    }

    ucmd = (lfsm_groupdisks_cmd_t *)malloc(sizeof(lfsm_groupdisks_cmd_t));
    ucmd->cmd.opcode = CMD_OP_LFSM;
    ucmd->cmd.subopcode = lfsm_subop_getgroupdisks;

    ucmd->groupId = atol(argv[3]);

//    syslog(LOG_INFO, "[SOFA ADM] get group disks with id %u\n",
//            ucmd->groupId);

    return (user_cmd_t *)ucmd;
}

static user_cmd_t *_get_sparedisks_cmd (int32_t argc, int8_t **argv)
{
    lfsm_sparedisks_cmd_t *ucmd;

    if (argc < 3) {
        syslog(LOG_INFO, "[SOFA ADM] wrong para to _get_sparedisks_cmd\n");
        return NULL;
    }

    ucmd = (lfsm_sparedisks_cmd_t *)malloc(sizeof(lfsm_sparedisks_cmd_t));
    ucmd->cmd.opcode = CMD_OP_LFSM;
    ucmd->cmd.subopcode = lfsm_subop_getsparedisks;

    //syslog(LOG_INFO, "[SOFA ADM] user command is _get_sparedisks_cmd !!\n");
    return (user_cmd_t *)ucmd;
}

static user_cmd_t *_get_diskSMART_cmd (int32_t argc, int8_t **argv)
{
    lfsm_diskSMART_cmd_t *ucmd;

    if (argc < 4) {
        syslog(LOG_INFO, "[SOFA ADM] wrong para to get disk SMART\n");
        return NULL;
    }

    ucmd = (lfsm_diskSMART_cmd_t *)malloc(sizeof(lfsm_diskSMART_cmd_t));
    ucmd->cmd.opcode = CMD_OP_LFSM;
    ucmd->cmd.subopcode = lfsm_subop_getDiskSMART;

    snprintf(ucmd->diskName, sizeof(ucmd->diskName), "%s", argv[3]);

    syslog(LOG_INFO, "[SOFA ADM] _get_diskSMART_cmd disk is %s\n", ucmd->diskName);
    return (user_cmd_t *)ucmd;
}

static user_cmd_t *_get_gcInfo_cmd (int32_t argc, int8_t **argv)
{
    lfsm_gcInfo_cmd_t *ucmd;

    if (argc < 3) {
        syslog(LOG_INFO, "[SOFA ADM] wrong para to get gc info\n");
        return NULL;
    }

    ucmd = (lfsm_gcInfo_cmd_t *)malloc(sizeof(lfsm_gcInfo_cmd_t));
    ucmd->cmd.opcode = CMD_OP_LFSM;
    ucmd->cmd.subopcode = lfsm_subop_getGcInfo;

    //syslog(LOG_INFO, "[SOFA ADM] user command is _get_gcInfo_cmd !!\n");
    return (user_cmd_t *)ucmd;
}


static user_cmd_t *_get_perf_cmd (int32_t argc, int8_t **argv)
{
    lfsm_perf_cmd_t *ucmd;

    if (argc < 4) {
        syslog(LOG_INFO, "[SOFA ADM] wrong para to _get_perf_cmd\n");
        return NULL;
    }

    ucmd = (lfsm_perf_cmd_t *)malloc(sizeof(lfsm_perf_cmd_t));
    ucmd->cmd.opcode = CMD_OP_LFSM;
    ucmd->cmd.subopcode = lfsm_subop_getperf;

    _show_checkSOFA();

    ucmd->request_size = atol(argv[3]);

//    syslog(LOG_INFO, "[SOFA ADM] get request_size is %u\n",
//            ucmd->request_size);

    return (user_cmd_t *)ucmd;
}

static user_cmd_t *_get_status_cmd (int32_t argc, int8_t **argv)
{
    user_cmd_t *ucmd;

    if (argc < 3) {
        syslog(LOG_INFO, "[SOFA ADM] wrong para to _get_status_cmd\n");
        return NULL;
    }

    ucmd = (user_cmd_t *)malloc(sizeof(user_cmd_t));
    ucmd->opcode = CMD_OP_LFSM;
    ucmd->subopcode = lfsm_subop_getstatus;

    _show_getstatus();

    //syslog(LOG_INFO, "[SOFA ADM] user command is _get_status_cmd !!\n");
    return (user_cmd_t *)ucmd;
}

static user_cmd_t *_get_properties_cmd (int32_t argc, int8_t **argv)
{
    user_cmd_t *ucmd;

    if (argc < 3) {
        syslog(LOG_INFO, "[SOFA ADM] wrong para to _get_properties_cmd\n");
        return NULL;
    }

    ucmd = (user_cmd_t *)malloc(sizeof(user_cmd_t));
    ucmd->opcode = CMD_OP_LFSM;
    ucmd->subopcode = lfsm_subop_getproperties;

    _show_getproperties();

    //syslog(LOG_INFO, "[SOFA ADM] user command is _get_properties_cmd !!\n");
    return (user_cmd_t *)ucmd;
}


static user_cmd_t *_activate_cmd (int32_t argc, int8_t **argv)
{
    user_cmd_t *ucmd;

    if (argc < 3) {
        syslog(LOG_INFO, "[SOFA ADM] wrong para to _activate_cmd\n");
        return NULL;
    }

    ucmd = (user_cmd_t *)malloc(sizeof(user_cmd_t));
    ucmd->opcode = CMD_OP_LFSM;
    ucmd->subopcode = lfsm_subop_activate;

    _show_activate();

    //syslog(LOG_INFO, "[SOFA ADM] user command is _activate_cmd !!\n");
    return (user_cmd_t *)ucmd;
}

static user_cmd_t *_set_gc_ratio_cmd (int32_t argc, int8_t **argv)
{
    user_cmd_t *ucmd;
    uint32_t on_ratio, off_ratio;

    if (argc < 5) {
        syslog(LOG_INFO, "[SOFA ADM] wrong para to _set_gc_ratio_cmd\n");
        return NULL;
    }

    ucmd = (user_cmd_t *)malloc(sizeof(user_cmd_t));
    ucmd->opcode = CMD_OP_LFSM;
    ucmd->subopcode = lfsm_subop_setgcratio;

    on_ratio = atol(argv[3]);
    off_ratio = atol(argv[4]);

    _show_setgcratio(on_ratio, off_ratio);

    //syslog(LOG_INFO, "[SOFA ADM] user command is _set_gc_ratio_cmd !!\n");
    return (user_cmd_t *)ucmd;
}

static user_cmd_t *_set_gc_reserve_cmd (int32_t argc, int8_t **argv)
{
    user_cmd_t *ucmd;
    uint32_t reserve_value;

    if (argc < 4) {
        syslog(LOG_INFO, "[SOFA ADM] wrong para to _set_gc_reserve_cmd\n");
        return NULL;
    }

    ucmd = (user_cmd_t *)malloc(sizeof(user_cmd_t));
    ucmd->opcode = CMD_OP_LFSM;
    ucmd->subopcode = lfsm_subop_setgcreserve;

    reserve_value = atol(argv[3]);
    _show_setgcreserve(reserve_value);

    //syslog(LOG_INFO, "[SOFA ADM] user command is _set_gc_reserve_cmd !!\n");
    return (user_cmd_t *)ucmd;
}

static int32_t _get_lfsm_subop (int8_t *cmd)
{
    int32_t i, ret;

    ret = lfsm_subop_end;
    for (i = lfsm_subop_getPhysicalCap; i < lfsm_subop_end; i++) {
        if (strcmp(cmd, lfsmCmds_str[i])) {
            continue;
        }

        ret = i;
        break;
    }

    return ret;
}

user_cmd_t *_get_lfsm_cmd (int32_t argc, int8_t **argv)
{
    user_cmd_t *ucmd;
    lfsm_subop_t lfsmsubop;

    ucmd = NULL;

    if (argc < 3) {
        syslog(LOG_INFO, "[SOFA ADM] lfsm wrong command format num arguments %d\n",
                argc);
        return ucmd;
    }

    lfsmsubop = _get_lfsm_subop(argv[2]);
    switch (lfsmsubop) {
    case lfsm_subop_getPhysicalCap:
        ucmd = _get_physicalCap_cmd(argc, argv);
        break;
    case lfsm_subop_getCapacity:
        ucmd = _get_capacity_cmd(argc, argv);
        break;
    case lfsm_subop_getgroups:
        ucmd = _get_groups_cmd(argc, argv);
        break;
    case lfsm_subop_getgroupdisks:
        ucmd = _get_groupdisks_cmd(argc, argv);
        break;
    case lfsm_subop_getsparedisks:
        ucmd = _get_sparedisks_cmd(argc, argv);
        break;
    case lfsm_subop_getDiskSMART:
        ucmd = _get_diskSMART_cmd(argc, argv);
        break;
    case lfsm_subop_getGcInfo:
        ucmd = _get_gcInfo_cmd(argc, argv);
        break;
    case lfsm_subop_getperf:
        ucmd = _get_perf_cmd(argc, argv);
        break;
    case lfsm_subop_getstatus:
        ucmd = _get_status_cmd(argc, argv);
        break;
    case lfsm_subop_getproperties:
        ucmd = _get_properties_cmd(argc, argv);
        break;
    case lfsm_subop_activate:
        ucmd = _activate_cmd(argc, argv);
        break;
    case lfsm_subop_setgcratio:
        ucmd = _set_gc_ratio_cmd(argc, argv);
        break;
    case lfsm_subop_setgcreserve:
        ucmd = _set_gc_reserve_cmd(argc, argv);
        break;
    default:
        syslog(LOG_ERR, "[SOFA ADM] unknow subop code %s\n", argv[2]);
        break;
    }

    return ucmd;
}
