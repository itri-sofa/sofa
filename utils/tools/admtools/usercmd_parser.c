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
#include <stdbool.h>
#include "user_cmd.h"
#include "usercmd_parser_private.h"
#include "usercmd_parser.h"
#include "lfsmcmd_parser.h"
#include "configcmd_parser.h"

static int32_t _get_opcode (int8_t *cmd)
{
    int32_t i, ret;

    ret = CMD_OP_END;
    for (i = CMD_OP_CONFIG; i < CMD_OP_END; i++) {
        if (strcmp(cmd, usercmd_opstr[i])) {
            continue;
        }

        ret = i;
        break;
    }

    return ret;
}

user_cmd_t *parse_user_cmd (int32_t argc, int8_t **argv)
{
    user_cmd_t *ucmd;
    ucmd_opcode_t opcode;

    ucmd = NULL;

    if (argc < 2) {
        syslog(LOG_INFO, "[SOFA ADM] wrong command format num arguments %d\n",
                argc);
        return ucmd;
    }

    opcode = _get_opcode(argv[1]);
    switch (opcode) {
    case CMD_OP_CONFIG:
        ucmd = _get_config_cmd(argc, argv);
        break;
    case CMD_OP_LFSM:
        ucmd = _get_lfsm_cmd(argc, argv);
        break;
    default:
        syslog(LOG_INFO, "[SOFA ADM] unknown cmd type %s\n", argv[1]);
        break;
    }

    return ucmd;
}
