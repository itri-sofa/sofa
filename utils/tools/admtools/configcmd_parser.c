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

#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <sys/types.h>
#include <string.h>
#include <stdint.h>
#include "user_cmd.h"
#include "configcmd_parser_private.h"
#include "configcmd_parser.h"

static user_cmd_t *_get_updatefile_cmd (int32_t argc, int8_t **argv , int32_t sub_opcode) 
{
    cfg_ufile_cmd_t *ucmd;
    int32_t arg_index, hasSetting;

    if (argc < 5) {
        syslog(LOG_INFO, "[SOFA ADM] wrong para to update file\n");
        return NULL;
    }

    hasSetting = 0;
    ucmd = (cfg_ufile_cmd_t *)malloc(sizeof(cfg_ufile_cmd_t));

    ucmd->itemSet = NULL;
    ucmd->cmd.opcode = CMD_OP_CONFIG;
    ucmd->cmd.subopcode = sub_opcode; 

    arg_index = 3;
    if (argc == 5) {
        ucmd->cfgfile = DEF_CFG_FN;
    } else if (argc == 6) {
        if (strncmp(argv[3], CMD_CFGFN_PREFIX, strlen(CMD_CFGFN_PREFIX))) {
            ucmd->cfgfile = DEF_CFG_FN;
            hasSetting = 1;
        } else {
            ucmd->cfgfile = argv[3] + strlen(CMD_CFGFN_PREFIX);
            arg_index++;
        }
    } else if (argc == 7) {
        if (strncmp(argv[3], CMD_CFGFN_PREFIX, strlen(CMD_CFGFN_PREFIX))) {
            syslog(LOG_INFO,
                    "[SOFA ADM] wrong config file name para to update file\n");
            free(ucmd);
            return NULL;
        }

        ucmd->cfgfile = argv[3] + strlen(CMD_CFGFN_PREFIX);
        arg_index++;
        hasSetting = 1;
    }

    ucmd->itemName = argv[arg_index];
    arg_index++;

    ucmd->itemVal = argv[arg_index];

    if (hasSetting) {
        ucmd->itemSet = argv[argc - 1];
        syslog(LOG_INFO, "[SOFA ADM] update config file %s with %s=%s %s\n",
                ucmd->cfgfile, ucmd->itemName, ucmd->itemVal, ucmd->itemSet);
    } else {
        syslog(LOG_INFO, "[SOFA ADM] update config file %s with %s=%s\n",
                ucmd->cfgfile, ucmd->itemName, ucmd->itemVal);
    }

    return (user_cmd_t *)ucmd;
}


static int32_t _get_config_subop (int8_t *cmd)
{
    int32_t i, ret;

    ret = cfg_subop_end;
    for (i = cfg_subop_updatefile; i < cfg_subop_end; i++) {
        if (strcmp(cmd, cfgcmds_str[i])) {
            continue;
        }

        ret = i;
        break;
    }

    return ret;
}

user_cmd_t *_get_config_cmd (int32_t argc, int8_t **argv)
{
    user_cmd_t *ucmd;
    cfg_subop_t cfgsubop;

    ucmd = NULL;

    if (argc < 3) {
        syslog(LOG_INFO, "[SOFA ADM] wrong command format num arguments %d\n",
                argc);
        return ucmd;
    }

    cfgsubop = _get_config_subop(argv[2]);
    switch (cfgsubop) {
    case cfg_subop_updatefile:
        ucmd = _get_updatefile_cmd(argc, argv,  cfg_subop_updatefile );
        break;
    case cfg_subop_update_append_file:
        ucmd = _get_updatefile_cmd(argc, argv, cfg_subop_update_append_file );
        break;
    default:
        syslog(LOG_ERR, "[SOFA ADM] unknow subop code %s\n", argv[2]);
        break;
    }

    return ucmd;
}
