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
#include <string.h>
#include <sys/types.h>
#include <stdint.h>
#include <syslog.h>
#include <stdbool.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "user_cmd.h"
#include "usercmd_handler.h"
#include "sofa_adm_main.h"
#include "usercmd_handler_private.h"
#include "sock_util.h"
#include "configcmd_handler.h"
#include "lfsmcmd_handler.h"

extern int32_t process_lfsmcmd_resp(int32_t sk_fd, user_cmd_t *ucmd,
        resp_hder_t *resp);

int32_t gen_common_header (user_cmd_t *ucmd, int8_t *buff, int32_t size)
{
    int8_t *ptr;
    int32_t ret, VAL_INT32;
    int16_t VAL_INT16;

    ret = 0;

    ptr = buff;
    VAL_INT32 = htonl(mnum_s);
    memcpy(ptr, &VAL_INT32, sizeof(uint32_t));
    ptr += sizeof(uint32_t);
    ret += sizeof(uint32_t);

    VAL_INT16 = htons(ucmd->opcode);
    memcpy(ptr, &VAL_INT16, sizeof(uint16_t));
    ptr += sizeof(uint16_t);
    ret += sizeof(uint16_t);

    VAL_INT16 = htons(ucmd->subopcode);
    memcpy(ptr, &VAL_INT16, sizeof(uint16_t));
    ptr += sizeof(uint16_t);
    ret += sizeof(uint16_t);

    VAL_INT32 = htonl(ucmd->tx_id);
    memcpy(ptr, &VAL_INT32, sizeof(uint32_t));
    ptr += sizeof(uint32_t);
    ret += sizeof(uint32_t);

    VAL_INT32 = htonl(size);
    memcpy(ptr, &VAL_INT32, sizeof(uint32_t));
    ptr += sizeof(uint32_t);
    ret += sizeof(uint32_t);

    ptr += size; //fill packet end at the end of buffer
    VAL_INT32 = htonl(mnum_e);
    memcpy(ptr, &VAL_INT32, sizeof(uint32_t));

    return ret;
}

static int32_t gen_packet_buffer (user_cmd_t *ucmd, int8_t **buff)
{
    int32_t ret;

    ret = 0;
    switch (ucmd->opcode) {
    case CMD_OP_CONFIG:
        syslog(LOG_INFO, "[SOFA ADM] not support CMD_OP_CONFIG now\n");
        break;
    case CMD_OP_LFSM:
        ret = gen_lfsm_packet_buffer(ucmd, buff);
        break;
    default:
        syslog(LOG_ERR, "[SOFA ADM] unknown command opcode %d\n", ucmd->opcode);
        break;
    }

    return ret;
}

static void _resp_hder2hOrder (resp_hder_t *resp)
{
    resp->mnum_s = ntohl(resp->mnum_s);
    resp->opcode = ntohs(resp->opcode);
    resp->subopcode = ntohs(resp->subopcode);
    resp->reqID = ntohl(resp->reqID);
    resp->result = ntohs(resp->result);
    resp->parasize = ntohl(resp->parasize);
}

static int32_t _chk_resp_hder (user_cmd_t *ucmd, resp_hder_t *resp)
{
    int32_t ret;

    ret = -1;
    do {
        if (resp->mnum_s != mnum_s) {
            syslog(LOG_INFO, "[SOFA ADM] resp header start error\n");
            break;
        }

        if (resp->opcode != ucmd->opcode) {
            syslog(LOG_INFO, "[SOFA ADM] resp opcode error\n");
            break;
        }

        if (resp->subopcode != ucmd->subopcode) {
            syslog(LOG_INFO, "[SOFA ADM] resp subopcode error\n");
            break;
        }

        if (resp->reqID != ucmd->tx_id) {
            syslog(LOG_INFO, "[SOFA ADM] resp reqID error\n");
            break;
        }

        ret = 0;
    } while (0);

    return ret;
}

static int32_t process_cmd_resp (int32_t sk_fd, user_cmd_t *ucmd,
        resp_hder_t *resp)
{
    int32_t ret;

    ret = 0;
    switch (ucmd->opcode) {
    case CMD_OP_CONFIG:
        syslog(LOG_INFO, "[SOFA ADM] not support CMD_OP_CONFIG now\n");
        break;
    case CMD_OP_LFSM:
        ret = process_lfsmcmd_resp(sk_fd, ucmd, resp);
        break;
    default:
        syslog(LOG_ERR, "[SOFA ADM] unknown command opcode %d\n", ucmd->opcode);
        break;
    }

    return ret;
}

/*
 * Send command to sofa user daemon
 */
static int32_t _process_ucmd_remote (user_cmd_t *ucmd)
{
    resp_hder_t resp;
    int32_t sk_fd, ret, buff_size;
    int8_t *buff;
    int i;

    ucmd->tx_id = get_reqID();
    if ((sk_fd = socket_connect(LOCAL_IP, LOCAL_PORT)) == -1) {
        syslog(LOG_INFO, "[SOFA ADM] fail connect to %s:%d\n", LOCAL_IP, LOCAL_PORT);
        return -1;
    }

    buff_size = gen_packet_buffer(ucmd, &buff);

    if (buff_size <= 0) {
        syslog(LOG_INFO, "[SOFA ADM] fail gen packet buffer for cmd op %d %d\n",
                ucmd->opcode, ucmd->subopcode);
        return -1;
    }

    if (socket_send(sk_fd, buff, buff_size)) {
        syslog(LOG_INFO, "[SOFA ADM] fail send request\n");
        return -1;
    }

    //Wait res
    if (socket_rcv(sk_fd, (int8_t *)&resp, sizeof(resp_hder_t))) {
        syslog(LOG_INFO, "[SOFA ADM] fail receive response\n");
        return -1;
    }

    _resp_hder2hOrder(&resp);

    if (_chk_resp_hder(ucmd, &resp)) {
        syslog(LOG_INFO, "[SOFA ADM] resp header error\n");
        return -1;
    }

    //post process
    ret = process_cmd_resp(sk_fd, ucmd, &resp);

    close(sk_fd);
    free(buff);
    return ret;
}

/*
 * process command at local machine
 * current purpose: update configuration file content
 */
static int32_t _process_ucmd_local (user_cmd_t *ucmd, bool is_append )
{
    return process_cfgcmd(ucmd , is_append );
}

int32_t process_user_cmd (user_cmd_t *ucmd)
{
    int32_t ret;

    ret = 0;

    if (ucmd->opcode == CMD_OP_CONFIG &&
            ucmd->subopcode == cfg_subop_updatefile  ) {
        ret = _process_ucmd_local(ucmd , false );
    }else if (ucmd->opcode == CMD_OP_CONFIG && 
            ucmd->subopcode == cfg_subop_update_append_file  ) { 
        ret = _process_ucmd_local(ucmd , true );
    } else {
        ret = _process_ucmd_remote(ucmd);
    }

    return ret;
}

void free_user_cmd (user_cmd_t *ucmd)
{
    switch (ucmd->opcode) {
    case CMD_OP_CONFIG:
        free_cfgcmd_resp(ucmd);
        break;
    case CMD_OP_LFSM:
        free_lfsmcmd_resp(ucmd);
        break;
    default:
        syslog(LOG_ERR, "[SOFA ADM] unknown command opcode %d\n", ucmd->opcode);
        break;
    }
}

void show_cmd_result (user_cmd_t *ucmd)
{
    switch (ucmd->opcode) {
    case CMD_OP_CONFIG:
        show_cfgcmd_result(ucmd);
        break;
    case CMD_OP_LFSM:
        show_lfsmcmd_result(ucmd);
        break;
    default:
        syslog(LOG_ERR, "[SOFA ADM] unknown command opcode %d\n", ucmd->opcode);
        break;
    }
}

void init_cmd_handler (void)
{
    mnum_s = 0xa5a5a5a5;
    mnum_e = 0xdeadbeef;
}
