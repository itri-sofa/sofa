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
#include <string.h>
#include <arpa/inet.h>
#include "userD_common_def.h"
#include "../common/drv_userD_common.h"
#include "../common/sofa_mm.h"
#include "lfsm_req_handler_private.h"
#include "lfsm_req_handler.h"
#include "calculate_perf.h"

/* Command search function */
int8_t *CmdSearch(const int8_t Cmd[], const int8_t Str[], int8_t * cmdBuf,
                  int32_t bufSize)
{
    FILE *fp;
    int32_t length;

    /* write information to CommandBuffer */
    fp = popen(Cmd, "r");       /*Issue the command. */
    memset(cmdBuf, 0, bufSize);
    /* Read a CommandBuffer */
    do {
        length = fread(cmdBuf, sizeof(int8_t), bufSize - 1, fp);
        if (length > 0) {
            cmdBuf[length - 1] = '\0';
        }
    } while (length > 0);
    pclose(fp);

    //syslog(LOG_INFO, "%s\n", cmdBuf);

    /* if find device */
    return strstr(cmdBuf, Str);
}

static int32_t _do_getPhysicalCap(int32_t ioctlfd, int8_t * para_buff,
                                  int32_t * resp_size, int8_t ** resp_buff)
{
    usercmd_para_t para;
    int32_t ret;

    ret = -1;

    //src_vol_id = ntohl(*((int32_t *)para_buff));

    //syslog(LOG_INFO, "SOFA INFO lfsm getPhysicalCap \n");

    para.opcode = CMD_OP_LFSM;
    para.subopcode = lfsm_subop_getPhysicalCap;
    ret = ioctl(ioctlfd, IOCTL_USER_CMD, (unsigned long)(&para));

    if (ret) {                  //
        *resp_size = 0;
    } else {
        if (para.cmd_result) {  //TODO: compare with return code
            *resp_size = 0;
            //syslog(LOG_INFO, "SOFA INFO lfsm getPhysicalCap *resp_size = %d\n", *resp_size);
        } else {
            *resp_size = sizeof(uint64_t);  //TODO Maybe we should define the cmd and resp format
            //syslog(LOG_INFO, "SOFA INFO lfsm getPhysicalCap *resp_size = %d\n", *resp_size);
            *resp_buff = sofa_malloc(*resp_size);
            memcpy(*resp_buff, para.cmd_resp, *resp_size);
        }
    }

    return ret;
}

static int32_t _do_getCapacity(int32_t ioctlfd, int8_t * para_buff,
                               int32_t * resp_size, int8_t ** resp_buff)
{
    usercmd_para_t para;
    int32_t ret;

    ret = -1;

    //src_vol_id = ntohl(*((int32_t *)para_buff));

    //syslog(LOG_INFO, "SOFA INFO lfsm getCapacity \n");

    para.opcode = CMD_OP_LFSM;
    para.subopcode = lfsm_subop_getCapacity;
    ret = ioctl(ioctlfd, IOCTL_USER_CMD, (unsigned long)(&para));

    if (ret) {                  //
        *resp_size = 0;
    } else {
        if (para.cmd_result) {  //TODO: compare with return code
            *resp_size = 0;
            //syslog(LOG_INFO, "SOFA INFO lfsm getCapacity *resp_size = %d\n", *resp_size);
        } else {
            *resp_size = sizeof(uint64_t);  //TODO Maybe we should define the cmd and resp format
            //syslog(LOG_INFO, "SOFA INFO lfsm getCapacity *resp_size = %d\n", *resp_size);
            *resp_buff = sofa_malloc(*resp_size);
            memcpy(*resp_buff, para.cmd_resp, *resp_size);
        }
    }

    return ret;
}

static int32_t _do_getgroups(int32_t ioctlfd, int8_t * para_buff,
                             int32_t * resp_size, int8_t ** resp_buff)
{
    usercmd_para_t para;
    int32_t ret;

    ret = -1;

    //src_vol_id = ntohl(*((int32_t *)para_buff));

    //syslog(LOG_INFO, "SOFA INFO lfsm getgroups \n");

    para.opcode = CMD_OP_LFSM;
    para.subopcode = lfsm_subop_getgroups;
    ret = ioctl(ioctlfd, IOCTL_USER_CMD, (unsigned long)(&para));

    if (ret) {                  //
        *resp_size = 0;
    } else {
        if (para.cmd_result) {  //TODO: compare with return code
            *resp_size = 0;
            //syslog(LOG_INFO, "SOFA INFO lfsm getgroups *resp_size = %d\n", *resp_size);
        } else {
            *resp_size = sizeof(uint32_t);  //TODO Maybe we should define the cmd and resp format
            //syslog(LOG_INFO, "SOFA INFO lfsm getgroups *resp_size = %d\n", *resp_size);
            *resp_buff = sofa_malloc(*resp_size);
            memcpy(*resp_buff, para.cmd_resp, *resp_size);
        }
    }

    return ret;
}

static int32_t _do_getgroupdisks(int32_t ioctlfd, int8_t * para_buff,
                                 int32_t * resp_size, int8_t ** resp_buff)
{
    usercmd_para_t para;
    int32_t ret;
    uint32_t groupId;

    ret = -1;

    groupId = ntohl(*((uint32_t *) para_buff));

    //syslog(LOG_INFO, "SOFA INFO lfsm getgroupdisks with groupId %u\n", groupId);

    para.opcode = CMD_OP_LFSM;
    para.subopcode = lfsm_subop_getgroupdisks;
    memcpy(para.cmd_para, (int8_t *) (&groupId), sizeof(uint32_t));
    ret = ioctl(ioctlfd, IOCTL_USER_CMD, (unsigned long)(&para));

    if (ret) {                  //
        *resp_size = 0;
    } else {
        if (para.cmd_result) {  //TODO: compare with return code
            *resp_size = 0;
            //syslog(LOG_INFO, "SOFA INFO lfsm getgroupdisks *resp_size = %d\n", *resp_size);
        } else {
            *resp_size = GROUPDISK_OUTSIZE; //TODO Maybe we should define the cmd and resp format
            //syslog(LOG_INFO, "SOFA INFO lfsm getgroupdisks *resp_size = %d\n", *resp_size);
            *resp_buff = sofa_malloc(*resp_size);
            memcpy(*resp_buff, para.cmd_resp, *resp_size);
        }
    }

    return ret;
}

static int32_t _do_getsparedisks(int32_t ioctlfd, int8_t * para_buff,
                                 int32_t * resp_size, int8_t ** resp_buff)
{
    usercmd_para_t para;
    int32_t ret;

    ret = -1;

    //src_vol_id = ntohl(*((int32_t *)para_buff));

    //syslog(LOG_INFO, "SOFA INFO lfsm getsparedisks \n");

    para.opcode = CMD_OP_LFSM;
    para.subopcode = lfsm_subop_getsparedisks;
    ret = ioctl(ioctlfd, IOCTL_USER_CMD, (unsigned long)(&para));

    if (ret) {                  //
        *resp_size = 0;
    } else {
        if (para.cmd_result) {  //TODO: compare with return code
            *resp_size = 0;
            //syslog(LOG_INFO, "SOFA INFO lfsm getsparedisks *resp_size = %d\n", *resp_size);
        } else {
            *resp_size = SPARE_OUTSIZE; //TODO Maybe we should define the cmd and resp format
            //syslog(LOG_INFO, "SOFA INFO lfsm getsparedisks *resp_size = %d\n", *resp_size);
            *resp_buff = sofa_malloc(*resp_size);
            memcpy(*resp_buff, para.cmd_resp, *resp_size);
        }
    }

    return ret;
}

static int32_t _do_getDiskSMART(int32_t ioctlfd, int8_t * para_buff,
                                int32_t * resp_size, int8_t ** resp_buff)
{
    int8_t cmd[CMDSIZE];
    int8_t searchCmd[CMDSIZE];
    int8_t *cmdBuf;
    int8_t *replyBuf;
    usercmd_para_t para;
    int32_t ret, i;
    uint32_t cmdBufSize, len;
    int8_t *ptr1;

    cmdBufSize = SMART_OUTSIZE;
    ret = -1;
    len = 0;

    cmdBuf = sofa_malloc(SMART_OUTSIZE);
    replyBuf = sofa_malloc(SMART_OUTSIZE);

    //syslog(LOG_INFO, "SOFA INFO lfsm getDiskSMART\n");

    para.opcode = CMD_OP_LFSM;
    para.subopcode = lfsm_subop_getDiskSMART;

    snprintf(searchCmd, sizeof(searchCmd), "Device Model");
    snprintf(cmd, sizeof(cmd), "smartctl -i /dev/%s", para_buff);
    //syslog(LOG_INFO, "%s\n", cmd);
    ptr1 = CmdSearch(cmd, searchCmd, cmdBuf, cmdBufSize);
    if (ptr1 != NULL) {         //find string
        len += sprintf(replyBuf + len, "%s;", ptr1);
    } else {
        syslog(LOG_INFO, "SSD SMART Failed !!");
        len +=
            sprintf(replyBuf + len,
                    "SSD SMART failed on cmd: smartctl -i /dev/%s;", para_buff);
    }

    snprintf(searchCmd, sizeof(searchCmd), "SMART overall-health");
    snprintf(cmd, sizeof(cmd), "smartctl -H /dev/%s", para_buff);
    //syslog(LOG_INFO, "%s\n", cmd);
    ptr1 = CmdSearch(cmd, searchCmd, cmdBuf, cmdBufSize);
    if (ptr1 != NULL) {         //find string
        len += sprintf(replyBuf + len, "%s;", ptr1);
    } else {
        syslog(LOG_INFO, "SSD SMART Failed !!");
        len +=
            sprintf(replyBuf + len,
                    "SSD SMART failed on cmd: smartctl -H /dev/%s;", para_buff);
    }

    snprintf(searchCmd, sizeof(searchCmd), "ID#");
    snprintf(cmd, sizeof(cmd), "smartctl -A /dev/%s", para_buff);
    //syslog(LOG_INFO, "%s\n", cmd);
    ptr1 = CmdSearch(cmd, searchCmd, cmdBuf, cmdBufSize);
    if (ptr1 != NULL) {         //find string
        len += sprintf(replyBuf + len, "%s;", ptr1);
    } else {
        syslog(LOG_INFO, "SSD SMART Failed !!");
        len +=
            sprintf(replyBuf + len,
                    "SSD SMART failed on cmd: smartctl -A /dev/%s;", para_buff);
    }

    ret = 0;
    para.cmd_result = 0;

    if (ret) {                  //
        *resp_size = 0;
    } else {
        if (para.cmd_result) {  //TODO: compare with return code
            *resp_size = 0;
        } else {
            *resp_size = sizeof(diskSMART_t);   //TODO Maybe we should define the cmd and resp format
//            syslog(LOG_INFO,
//                    "SOFA INFO lfsm getDiskSMART *resp_size = %d\n", *resp_size);
            *resp_buff = sofa_malloc(*resp_size);
            memcpy(*resp_buff, replyBuf, *resp_size);
        }
    }

    sofa_free(cmdBuf);
    sofa_free(replyBuf);

    return ret;
}

static int32_t _do_getGcInfo(int32_t ioctlfd, int8_t * para_buff,
                             int32_t * resp_size, int8_t ** resp_buff)
{
    usercmd_para_t para;
    int32_t ret;

    ret = -1;

    //src_vol_id = ntohl(*((int32_t *)para_buff));

    //syslog(LOG_INFO, "SOFA INFO lfsm getGcInfo \n");

    para.opcode = CMD_OP_LFSM;
    para.subopcode = lfsm_subop_getGcInfo;
    ret = ioctl(ioctlfd, IOCTL_USER_CMD, (unsigned long)(&para));

    if (ret) {                  //
        *resp_size = 0;
    } else {
        if (para.cmd_result) {  //TODO: compare with return code
            *resp_size = 0;
            //syslog(LOG_INFO, "SOFA INFO lfsm getGcInfo *resp_size = %d\n", *resp_size);
        } else {
            *resp_size = sizeof(gcInfo_t);  //TODO Maybe we should define the cmd and resp format
            //syslog(LOG_INFO, "SOFA INFO lfsm getGcInfo *resp_size = %d\n", *resp_size);
            *resp_buff = sofa_malloc(*resp_size);
            memcpy(*resp_buff, para.cmd_resp, *resp_size);
        }
    }

    return ret;
}

static int32_t _do_getperf(int32_t ioctlfd, int8_t * para_buff,
                           int32_t * resp_size, int8_t ** resp_buff)
{
    usercmd_para_t para;
    int32_t ret;
    uint32_t request_length;
    int8_t *replyBuf;

    ret = -1;

    request_length = ntohl(*((uint32_t *) para_buff));

    //syslog(LOG_INFO, "SOFA INFO lfsm _do_getperf with request_length %u\n",
    //        request_length);

    replyBuf = sofa_malloc(PERF_OUTSIZE);
    memset(replyBuf, 0, PERF_OUTSIZE);

    para.opcode = CMD_OP_LFSM;
    para.subopcode = lfsm_subop_getperf;

    get_perf_data(replyBuf, request_length);

    ret = 0;
    para.cmd_result = 0;

    if (ret) {                  //
        *resp_size = 0;
    } else {
        if (para.cmd_result) {  //TODO: compare with return code
            *resp_size = 0;
        } else {
            *resp_size = PERF_OUTSIZE;  //TODO Maybe we should define the cmd and resp format
            //syslog(LOG_INFO,
            //        "SOFA INFO lfsm _do_getperf *resp_size = %d\n", *resp_size);
            *resp_buff = sofa_malloc(*resp_size);
            memcpy(*resp_buff, replyBuf, *resp_size);
        }
    }

    sofa_free(replyBuf);
    return ret;
}

/*
 * return value: the command handle results
 * para: size: the data size in para_buffer
 *
 */
int32_t handle_lfsm_req(int32_t ioctlfd, int16_t subopcode,
                        int8_t * para_buff, int32_t * resp_size,
                        int8_t ** resp_buff)
{
    int32_t ret;

//    syslog(LOG_INFO, "[SOFA] USERD handle lfsm req with subopcode %d\n",
//            subopcode);

    switch (subopcode) {
    case lfsm_subop_getPhysicalCap:
        ret = _do_getPhysicalCap(ioctlfd, para_buff, resp_size, resp_buff);
        break;
    case lfsm_subop_getCapacity:
        ret = _do_getCapacity(ioctlfd, para_buff, resp_size, resp_buff);
        break;
    case lfsm_subop_getgroups:
        ret = _do_getgroups(ioctlfd, para_buff, resp_size, resp_buff);
        break;
    case lfsm_subop_getgroupdisks:
        ret = _do_getgroupdisks(ioctlfd, para_buff, resp_size, resp_buff);
        break;
    case lfsm_subop_getsparedisks:
        ret = _do_getsparedisks(ioctlfd, para_buff, resp_size, resp_buff);
        break;
    case lfsm_subop_getDiskSMART:
        ret = _do_getDiskSMART(ioctlfd, para_buff, resp_size, resp_buff);
        break;
    case lfsm_subop_getGcInfo:
        ret = _do_getGcInfo(ioctlfd, para_buff, resp_size, resp_buff);
        break;
    case lfsm_subop_getperf:
        ret = _do_getperf(ioctlfd, para_buff, resp_size, resp_buff);
        break;
    default:
        syslog(LOG_INFO, "[SOFA] USERD INFO wrong lfsm subopcode %d\n",
               subopcode);
        ret = -1;
        break;
    }

    return ret;
}
