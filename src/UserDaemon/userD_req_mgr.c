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
#include <pthread.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <stdint.h>
#include "userD_errno.h"
#include "../common/userspace_list.h"
#include "userD_req_mgr_private.h"
#include "userD_req_mgr.h"
#include "../common/drv_userD_common.h"
#include "lfsm_req_handler.h"

/*
 * return value: the command handle results
 * para: size: the data size in para_buffer
 *
 */
int32_t handle_req(int32_t ioctlfd, int16_t opcode, int16_t subopcode,
                   int8_t * para_buff, int32_t * resp_size, int8_t ** resp_buff)
{
    int32_t ret;

    *resp_buff = NULL;
    //syslog(LOG_INFO, "[SOFA] USERD handle req with opcode %d\n", opcode);

    switch (opcode) {
    case CMD_OP_CONFIG:
        ret = -ENOSYS;
        *resp_size = 0;
        break;
    case CMD_OP_LFSM:
        ret =
            handle_lfsm_req(ioctlfd, subopcode, para_buff, resp_size,
                            resp_buff);
        break;
    case CMD_OP_END:
        ret = -ENOSYS;
        *resp_size = 0;
        break;
    default:
        break;
    }

    return ret;
}

int32_t handle_req_async(int16_t opcode, int16_t subopcode, int8_t * para_buff)
{
    return -ENOSYS;
}

int32_t init_reqs_mgr(void)
{
    int32_t ret;

    ret = 0;
    syslog(LOG_INFO, "[SOFA] USERD INFO initial user daemon request manager\n");
    userD_reqMgr.running = 0;

    INIT_LIST_HEAD(&userD_reqMgr.req_pendq);
    INIT_LIST_HEAD(&userD_reqMgr.req_pendq);
    userD_reqMgr.cnt_reqs = 0;
    userD_reqMgr.cnt_wreqs = 0;

    pthread_mutex_init(&userD_reqMgr.reqs_qlock, NULL);

    userD_reqMgr.running = 1;

    return ret;
}

int32_t rel_reqs_mgr(void)
{
    userD_reqMgr.running = 0;
    if (userD_reqMgr.cnt_reqs != 0 || userD_reqMgr.cnt_wreqs != 0) {
        syslog(LOG_ERR, "SOFA WARN linger requests (%u %u) != (0, 0) \n",
               userD_reqMgr.cnt_reqs, userD_reqMgr.cnt_wreqs);
    }

    return 0;
}
