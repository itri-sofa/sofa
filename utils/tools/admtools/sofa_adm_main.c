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
#include <stdint.h>
#include <pthread.h>
#include <string.h>
#include <syslog.h>
#include "sofa_adm_main_private.h"
#include "sofa_adm_main.h"
#include "user_cmd.h"
#include "usercmd_parser.h"
#include "usercmd_handler.h"

static void show_configCMD_help (void)
{
    printf("[Only if existing, item is updated.] sofaadm --config --updatefile [cfgfn=file name] itemName itemValue\n"); 
    printf("[If unexisting, item is added.     ] sofaadm --config --update_append_file [cfgfn=file name] itemName itemValue\n");
}

static void show_lfsmCMD_help (void)
{
    printf("sofaadm --lfsm --getgcinfo\n");
    printf("sofaadm --lfsm --getsmart <sda or sdb or sdc or...>\n");
    printf("sofaadm --lfsm --getsparedisk\n");
    printf("sofaadm --lfsm --getgroupdisk <group id>\n");
    printf("sofaadm --lfsm --getgroup\n");
    printf("sofaadm --lfsm --getcap\n");
    printf("sofaadm --lfsm --getphycap\n");
    printf("sofaadm --lfsm --getperf <request times>\n");
    printf("sofaadm --lfsm --getstatus\n");
    printf("sofaadm --lfsm --getproperties\n");
    printf("sofaadm --lfsm --activate\n");
    printf("sofaadm --lfsm --setgcratio <on value> <off value>\n");
    printf("sofaadm --lfsm --setgcreserve <reserve value>\n");
}

static void show_help (void)
{
    show_configCMD_help();
    show_lfsmCMD_help();
}

uint32_t get_reqID (void)
{
    uint32_t ret;

    pthread_mutex_lock(&reqID_lock);
    ret = ++cnt_reqID;
    pthread_mutex_unlock(&reqID_lock);

    return ret;
}

int32_t main (int32_t argc, int8_t **argv)
{
    user_cmd_t *req;

    cnt_reqID = 0;
    if (argc < MIN_NUM_PARA) {
        show_help();
        return -1;
    }

    init_cmd_handler();

    req = parse_user_cmd(argc, argv);
    if (req == NULL) {
        show_help();
        return -1;
    }

    //for lfsm local handle only
    if (req->opcode == CMD_OP_LFSM && req->subopcode > lfsm_subop_getperf) {
        free(req);
        return 0;
    }

    if (process_user_cmd(req)) {
        syslog(LOG_INFO, "[SOFA ADM] execute command error\n");
    } else {
        show_cmd_result(req);
    }

    free_user_cmd(req);

    return 0;
}
