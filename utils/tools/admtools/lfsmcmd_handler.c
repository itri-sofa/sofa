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
#include <arpa/inet.h>
#include <json-c/json.h>
#include <assert.h>
#include <time.h>
#include <unistd.h>
#include "sock_util.h"
#include "common_def.h"
#include "user_cmd.h"
#include "usercmd_handler.h"
#include "lfsmcmd_handler_private.h"
#include "lfsmcmd_handler.h"

/* Command search function */
int8_t *CmdSearch (const int8_t Cmd[], const int8_t Str[], int8_t *cmdBuf,
        int32_t bufSize) {
    FILE *fp;
    int32_t length;

    /* write information to CommandBuffer */
    fp = popen(Cmd, "r"); /*Issue the command.*/
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

static int32_t _gen_getPhysicalCap_packet (user_cmd_t *ucmd, int8_t **buff)
{
    lfsm_physicalCap_cmd_t *mycmd;
    int32_t buf_size, ret, VAL_INT32;
    int8_t *ptr;

    mycmd = (lfsm_physicalCap_cmd_t *)ucmd;
    buf_size = REQ_HEADER_SIZE + 4;
    *buff = (int8_t *)malloc(sizeof(int8_t)*buf_size);

    ret = gen_common_header(ucmd, *buff, 4);
    ptr = (*buff + ret);

    VAL_INT32 = htonl(0);
    memcpy(ptr, &VAL_INT32, sizeof(uint32_t));
    ptr += sizeof(uint32_t);

    return buf_size;
}

static int32_t _gen_getCapacity_packet (user_cmd_t *ucmd, int8_t **buff)
{
    lfsm_capacity_cmd_t *mycmd;
    int32_t buf_size, ret, VAL_INT32;
    int8_t *ptr;

    mycmd = (lfsm_capacity_cmd_t *)ucmd;
    buf_size = REQ_HEADER_SIZE + 4;
    *buff = (int8_t *)malloc(sizeof(int8_t)*buf_size);

    ret = gen_common_header(ucmd, *buff, 4);
    ptr = (*buff + ret);

    VAL_INT32 = htonl(0);
    memcpy(ptr, &VAL_INT32, sizeof(uint32_t));
    ptr += sizeof(uint32_t);

    return buf_size;
}

static int32_t _gen_getgroups_packet (user_cmd_t *ucmd, int8_t **buff)
{
    lfsm_groups_cmd_t *mycmd;
    int32_t buf_size, ret, VAL_INT32;
    int8_t *ptr;

    mycmd = (lfsm_groups_cmd_t *)ucmd;
    buf_size = REQ_HEADER_SIZE + 4;
    *buff = (int8_t *)malloc(sizeof(int8_t)*buf_size);

    ret = gen_common_header(ucmd, *buff, 4);
    ptr = (*buff + ret);

    VAL_INT32 = htonl(0);
    memcpy(ptr, &VAL_INT32, sizeof(uint32_t));
    ptr += sizeof(uint32_t);

    return buf_size;
}

static int32_t _gen_getgroupdisks_packet (user_cmd_t *ucmd, int8_t **buff)
{
    lfsm_groupdisks_cmd_t *mycmd;
    int32_t buf_size, ret, VAL_INT32;
    int8_t *ptr;

    mycmd = (lfsm_groupdisks_cmd_t *)ucmd;
    buf_size = REQ_HEADER_SIZE + 4;
    *buff = (int8_t *)malloc(sizeof(int8_t)*buf_size);

    ret = gen_common_header(ucmd, *buff, 4);
    ptr = (*buff + ret);

    VAL_INT32 = htonl(mycmd->groupId);
    memcpy(ptr, &VAL_INT32, sizeof(uint32_t));
    ptr += sizeof(uint32_t);

    return buf_size;
}

static int32_t _gen_getsparedisks_packet (user_cmd_t *ucmd, int8_t **buff)
{
    lfsm_sparedisks_cmd_t *mycmd;
    int32_t buf_size, ret, VAL_INT32;
    int8_t *ptr;

    mycmd = (lfsm_sparedisks_cmd_t *)ucmd;
    buf_size = REQ_HEADER_SIZE + 4;
    *buff = (int8_t *)malloc(sizeof(int8_t)*buf_size);

    ret = gen_common_header(ucmd, *buff, 4);
    ptr = (*buff + ret);

    VAL_INT32 = htonl(0);
    memcpy(ptr, &VAL_INT32, sizeof(uint32_t));
    ptr += sizeof(uint32_t);

    return buf_size;
}

static int32_t _gen_getDiskSMART_packet (user_cmd_t *ucmd, int8_t **buff)
{
    lfsm_diskSMART_cmd_t *mycmd;
    int32_t buf_size, ret, attachSize;
    int8_t *ptr;

    attachSize = sizeof(mycmd->diskName);

    mycmd = (lfsm_diskSMART_cmd_t *)ucmd;
    buf_size = REQ_HEADER_SIZE + attachSize;
    *buff = (int8_t *)malloc(sizeof(int8_t)*buf_size);

    ret = gen_common_header(ucmd, *buff, attachSize);
    ptr = (*buff + ret);

    //VAL_INT32 = htonll(mycmd->diskName);
    memcpy(ptr, mycmd->diskName, attachSize);
    ptr += attachSize;

    syslog(LOG_INFO, "[SOFA ADM] diskName=%s, attachSize=%d \n",
            mycmd->diskName, attachSize);

    return buf_size;
}


static int32_t _gen_getGcInfo_packet (user_cmd_t *ucmd, int8_t **buff)
{
	lfsm_gcInfo_cmd_t *mycmd;
    int32_t buf_size, ret, VAL_INT32;
    int8_t *ptr;

    mycmd = (lfsm_gcInfo_cmd_t *)ucmd;
    buf_size = REQ_HEADER_SIZE + 4;
    *buff = (int8_t *)malloc(sizeof(int8_t)*buf_size);

    ret = gen_common_header(ucmd, *buff, 4);
    ptr = (*buff + ret);

    VAL_INT32 = htonl(0);
    memcpy(ptr, &VAL_INT32, sizeof(uint32_t));
    ptr += sizeof(uint32_t);

    return buf_size;
}

static int32_t _gen_getperf_packet(user_cmd_t *ucmd, int8_t **buff)
{
    lfsm_perf_cmd_t *mycmd;
    int32_t buf_size, ret, VAL_INT32;
    int8_t *ptr;

    mycmd = (lfsm_perf_cmd_t *) ucmd;
    buf_size = REQ_HEADER_SIZE + 4;
    *buff = (int8_t *) malloc(sizeof(int8_t) * buf_size);

    ret = gen_common_header(ucmd, *buff, 4);
    ptr = (*buff + ret);

    VAL_INT32 = htonl(mycmd->request_size);
    memcpy(ptr, &VAL_INT32, sizeof(uint32_t));
    ptr += sizeof(uint32_t);

    return buf_size;
}

int32_t gen_lfsm_packet_buffer (user_cmd_t *ucmd, int8_t **buff)
{
    int32_t ret;

    ret = 0;
    switch (ucmd->subopcode) {
    case lfsm_subop_getPhysicalCap:
        ret = _gen_getPhysicalCap_packet(ucmd, buff);
        break;
    case lfsm_subop_getCapacity:
        ret = _gen_getCapacity_packet(ucmd, buff);
        break;
    case lfsm_subop_getgroups:
        ret = _gen_getgroups_packet(ucmd, buff);
        break;
    case lfsm_subop_getgroupdisks:
        ret = _gen_getgroupdisks_packet(ucmd, buff);
        break;
    case lfsm_subop_getsparedisks:
        ret = _gen_getsparedisks_packet(ucmd, buff);
        break;
    case lfsm_subop_getDiskSMART:
        ret = _gen_getDiskSMART_packet(ucmd, buff);
        break;
    case lfsm_subop_getGcInfo:
        ret = _gen_getGcInfo_packet(ucmd, buff);
        break;
    case lfsm_subop_getperf:
        ret = _gen_getperf_packet(ucmd, buff);
        break;
    default:
        syslog(LOG_INFO, "[SOFA ADM] unknow opcode for gen packet %d\n",
                ucmd->opcode);
        ret = -1;
        break;
    }

    return ret;
}

static int32_t _handle_resp_getsparedisksCmd (int32_t sk_fd, user_cmd_t *ucmd,
        resp_hder_t *resp)
{
    lfsm_sparedisks_cmd_t *mycmd;
    int32_t ret;

    mycmd = (lfsm_sparedisks_cmd_t *)ucmd;

    ucmd->result = resp->result;

    ret = 0;
    if (resp->parasize > 0 && resp->parasize <= SPARE_OUTSIZE) {
        if (socket_rcv(sk_fd, (int8_t *)(mycmd->ret_sparedisks), resp->parasize)) {
            syslog(LOG_ERR,
                    "[SOFA ADM] fail rcv _handle_resp_getsparedisksCmd resp\n");
            ret = -1;
        }
    } else if (resp->parasize == 0) {
        syslog(LOG_ERR,
                "[SOFA ADM] _handle_resp_getsparedisksCmd fail with result: %d\n",
                ucmd->result);
    } else {
        syslog(LOG_ERR,
                "[SOFA ADM] wrong para size in _handle_resp_getsparedisksCmd resp header\n");
        ret = -1;
    }

    return ret;
}

static int32_t _handle_resp_getgroupdisksCmd (int32_t sk_fd, user_cmd_t *ucmd,
        resp_hder_t *resp)
{
    lfsm_groupdisks_cmd_t *mycmd;
    int32_t ret;

    mycmd = (lfsm_groupdisks_cmd_t *)ucmd;

    ucmd->result = resp->result;

    ret = 0;
    if (resp->parasize > 0 && resp->parasize <= GROUPDISK_OUTSIZE) {
        if (socket_rcv(sk_fd, (int8_t *)(mycmd->ret_groupdisks), resp->parasize)) {
            syslog(LOG_ERR, "[SOFA ADM] fail rcv _handle_resp_getgroupdisksCmd resp\n");
            ret = -1;
        }
    } else if (resp->parasize == 0) {
        syslog(LOG_ERR,
                "[SOFA ADM] _handle_resp_getgroupdisksCmd fail with result: %d\n",
                ucmd->result);
    } else {
        syslog(LOG_ERR,
                "[SOFA ADM] wrong para size in _handle_resp_getgroupdisksCmd resp header\n");
        ret = -1;
    }

    return ret;
}

static int32_t _handle_resp_getgroupsCmd (int32_t sk_fd, user_cmd_t *ucmd,
        resp_hder_t *resp)
{
    lfsm_groups_cmd_t *mycmd;
    int32_t ret;

    mycmd = (lfsm_groups_cmd_t *)ucmd;

    ucmd->result = resp->result;

    ret = 0;
    if (resp->parasize == sizeof(int32_t)) {
        if (socket_rcv(sk_fd, (int8_t *)&(mycmd->ret_groups), resp->parasize)) {
            syslog(LOG_ERR, "[SOFA ADM] fail rcv _handle_resp_getgroupsCmd resp\n");
            ret = -1;
        }
    } else if (resp->parasize == 0) {
        syslog(LOG_ERR,
                "[SOFA ADM] _handle_resp_getgroupsCmd fail with result: %d\n",
                ucmd->result);
    } else {
        syslog(LOG_ERR,
                "[SOFA ADM] wrong para size in _handle_resp_getgroupsCmd resp header\n");
        ret = -1;
    }

    return ret;
}

static int32_t _handle_resp_getCapacityCmd (int32_t sk_fd, user_cmd_t *ucmd,
        resp_hder_t *resp)
{
    lfsm_capacity_cmd_t *mycmd;
    int32_t ret;

    mycmd = (lfsm_capacity_cmd_t *)ucmd;

    ucmd->result = resp->result;

    ret = 0;
    if (resp->parasize == sizeof(int64_t)) {
        if (socket_rcv(sk_fd, (int8_t *)&(mycmd->ret_cap), resp->parasize)) {
            syslog(LOG_ERR,
                    "[SOFA ADM] fail rcv _handle_resp_getCapacityCmd resp\n");
            ret = -1;
        }
    } else if (resp->parasize == 0) {
        syslog(LOG_ERR,
                "[SOFA ADM] _handle_resp_getCapacityCmd fail with result: %d\n",
                ucmd->result);
    } else {
        syslog(LOG_ERR,
                "[SOFA ADM] wrong para size in _handle_resp_getCapacityCmd resp header\n");
        ret = -1;
    }

    return ret;
}

static int32_t _handle_resp_getPhysicalCapCmd (int32_t sk_fd, user_cmd_t *ucmd,
        resp_hder_t *resp)
{
    lfsm_physicalCap_cmd_t *mycmd;
    int32_t ret;

    mycmd = (lfsm_physicalCap_cmd_t *)ucmd;

    ucmd->result = resp->result;

    ret = 0;
    if (resp->parasize == sizeof(int64_t)) {
        if (socket_rcv(sk_fd, (int8_t *)&(mycmd->ret_phycap), resp->parasize)) {
            syslog(LOG_ERR,
                    "[SOFA ADM] fail rcv _handle_resp_getPhysicalCapCmd resp\n");
            ret = -1;
        }
    } else if (resp->parasize == 0) {
        syslog(LOG_ERR,
                "[SOFA ADM] _handle_resp_getPhysicalCapCmd fail with result: %d\n",
                ucmd->result);
    } else {
        syslog(LOG_ERR,
                "[SOFA ADM] wrong para size in _handle_resp_getPhysicalCapCmd resp header\n");
        ret = -1;
    }

    return ret;
}

static int32_t _handle_resp_getDiskSMARTCmd (int32_t sk_fd, user_cmd_t *ucmd,
        resp_hder_t *resp)
{
    lfsm_diskSMART_cmd_t *mycmd;
    int32_t ret;

    mycmd = (lfsm_diskSMART_cmd_t *)ucmd;

    ucmd->result = resp->result;

    ret = 0;

    syslog(LOG_INFO,
            "[SOFA ADM] _handle_resp_getDiskSMARTCmd resp->parasize=%d\n",
            resp->parasize);
    if (resp->parasize > 0 && resp->parasize <= SMART_OUTSIZE) {
        //Wait resp data
        if (socket_rcv(sk_fd, (int8_t *)&(mycmd->msginfo), resp->parasize)) {
            syslog(LOG_INFO, "[SOFA ADM] fail receive response data part\n");
            return -1;
        }
        //use printf can see all messages, syslog cannot see all messages.
        //syslog(LOG_INFO,"[SOFA ADM] response data : \n%s\n", mycmd->msginfo);
    } else {
        syslog(LOG_ERR,
                "[SOFA ADM] wrong para size in _handle_resp_getDiskSMARTCmd\n");
        ret = -1;
    }

    return ret;
}

static int32_t _handle_resp_getGcInfoCmd (int32_t sk_fd, user_cmd_t *ucmd,
        resp_hder_t *resp)
{
    lfsm_gcInfo_cmd_t *mycmd;
    int32_t ret;
    int32_t i;

    mycmd = (lfsm_gcInfo_cmd_t *)ucmd;

    ucmd->result = resp->result;

    ret = 0;

//    syslog(LOG_INFO, "[SOFA ADM] _handle_resp_getGcInfoCmd resp->parasize=%d\n",
//            resp->parasize);

    if (resp->parasize == sizeof(gcInfo_t)) {
        //Wait resp data
        if (socket_rcv(sk_fd, (int8_t *)&(mycmd->gcInfo), resp->parasize)) {
            syslog(LOG_INFO, "[SOFA ADM] fail receive response data part\n");
            return -1;
        }
    } else {
        syslog(LOG_ERR, "[SOFA ADM] wrong para size in _handle_resp_getGcInfoCmd\n");
        ret = -1;
    }

    return ret;
}

static int32_t _handle_resp_getperfCmd (int32_t sk_fd, user_cmd_t *ucmd,
        resp_hder_t *resp)
{
    lfsm_perf_cmd_t *mycmd;
    int32_t ret;

    mycmd = (lfsm_perf_cmd_t *) ucmd;

    ucmd->result = resp->result;

    ret = 0;

    //syslog(LOG_INFO, "[SOFA ADM] _handle_resp_getperfCmd resp->parasize=%d\n",
    //        resp->parasize);
    if (resp->parasize > 0 && resp->parasize <= PERF_OUTSIZE) {
        //Wait resp data
        if (socket_rcv(sk_fd, (int8_t *) &(mycmd->ret_perf), resp->parasize)) {
            syslog(LOG_INFO, "[SOFA ADM] fail receive response data part\n");
            return -1;
        }
        //use printf can see all messages, syslog cannot see all messages.
        //syslog(LOG_INFO,"[SOFA ADM] response data : \n%s\n", mycmd->msginfo);
    } else {
        syslog(LOG_ERR,
                "[SOFA ADM] wrong para size in _handle_resp_getperfCmd\n");
        ret = -1;
    }

    return ret;
}

int32_t process_lfsmcmd_resp (int32_t sk_fd, user_cmd_t *ucmd,
        resp_hder_t *resp)
{
    int32_t ret;

    ret = 0;
    switch (ucmd->subopcode) {
    case lfsm_subop_getPhysicalCap:
        ret = _handle_resp_getPhysicalCapCmd(sk_fd, ucmd, resp);
        break;
    case lfsm_subop_getCapacity:
        ret = _handle_resp_getCapacityCmd(sk_fd, ucmd, resp);
        break;
    case lfsm_subop_getgroups:
        ret = _handle_resp_getgroupsCmd(sk_fd, ucmd, resp);
        break;
    case lfsm_subop_getgroupdisks:
        ret = _handle_resp_getgroupdisksCmd(sk_fd, ucmd, resp);
        break;
    case lfsm_subop_getsparedisks:
        ret = _handle_resp_getsparedisksCmd(sk_fd, ucmd, resp);
        break;
    case lfsm_subop_getDiskSMART:
        ret = _handle_resp_getDiskSMARTCmd(sk_fd, ucmd, resp);
        break;
    case lfsm_subop_getGcInfo:
        ret = _handle_resp_getGcInfoCmd(sk_fd, ucmd, resp);
        break;
    case lfsm_subop_getperf:
        ret = _handle_resp_getperfCmd(sk_fd, ucmd, resp);
        break;
    default:
        syslog(LOG_INFO, "[SOFA ADM] unknown sub opcode process vol cmd resp %d\n",
                ucmd->opcode);
        ret = -1;
        break;
    }

    return ret;
}

void free_lfsmcmd_resp (user_cmd_t *ucmd)
{
    switch (ucmd->subopcode) {
    case lfsm_subop_getPhysicalCap:
        free((lfsm_physicalCap_cmd_t *)ucmd);
        break;
    case lfsm_subop_getCapacity:
        free((lfsm_capacity_cmd_t *)ucmd);
        break;
    case lfsm_subop_getgroups:
        free((lfsm_groups_cmd_t *)ucmd);
        break;
    case lfsm_subop_getgroupdisks:
        free((lfsm_groupdisks_cmd_t *)ucmd);
        break;
    case lfsm_subop_getsparedisks:
        free((lfsm_sparedisks_cmd_t *)ucmd);
        break;
    case lfsm_subop_getDiskSMART:
        free((lfsm_diskSMART_cmd_t *)ucmd);
        break;
    case lfsm_subop_getGcInfo:
        free((lfsm_gcInfo_cmd_t *)ucmd);
        break;
    case lfsm_subop_getperf:
        free((lfsm_perf_cmd_t *) ucmd);
        break;
    default:
        syslog(LOG_ERR, "[SOFA ADM] unknow opcode for free %d\n", ucmd->opcode);
        free(ucmd);
        break;
    }
}

static void _show_getPhysicalCap (user_cmd_t *ucmd)
{
    lfsm_physicalCap_cmd_t *mycmd;
    json_object *jstring;
    int8_t phyCapBuf[32];
    json_object * jobj;

    jobj = json_object_new_object();
    mycmd = (lfsm_physicalCap_cmd_t *)ucmd;

    if (ucmd->result) {
        //TODO: FSS-88 Can sofa monitor handle this and paring result?
        printf("SOFA TOOL get phy capacity fail ret %d\n", ucmd->result);
    } else {
        //printf("SOFA TOOL get phy capacity success: %llu\n", mycmd->ret_phycap);
    }

    snprintf(phyCapBuf, sizeof(phyCapBuf), "%llu", mycmd->ret_phycap);

    jstring = json_object_new_string(phyCapBuf);

    /*Form the json object*/
    json_object_object_add(jobj, "physicalCapacity", jstring);

    /*Now printing the json object*/
    printf ("The json object created: %s\n", json_object_to_json_string(jobj));
}

static void _show_getCapacity (user_cmd_t *ucmd)
{
    lfsm_capacity_cmd_t *mycmd;
    json_object *jstring;
    int8_t capBuf[32];
    json_object * jobj;

    jobj = json_object_new_object();
    mycmd = (lfsm_capacity_cmd_t *)ucmd;

    if (ucmd->result) {
        //TODO: FSS-88 Can sofa monitor handle this and paring result?
        printf("SOFA TOOL get capacity fail ret %d\n", ucmd->result);
    } else {
        //printf("SOFA TOOL get capacity success: %llu\n", mycmd->ret_cap);
    }

    snprintf(capBuf, sizeof(capBuf), "%llu", mycmd->ret_cap);

    jstring = json_object_new_string(capBuf);

    /*Form the json object*/
    json_object_object_add(jobj, "capacity", jstring);

    /*Now printing the json object*/
    printf ("The json object created: %s\n", json_object_to_json_string(jobj));
}

static void _show_getgroups (user_cmd_t *ucmd)
{
    lfsm_groups_cmd_t *mycmd;
    json_object *jint, *jobj;

    jobj = json_object_new_object();
    mycmd = (lfsm_groups_cmd_t *)ucmd;

    if (ucmd->result) {
        //TODO: FSS-88 Can sofa monitor handle this and paring result?
        printf("SOFA TOOL get groups fail ret %d\n", ucmd->result);
    } else {
        //printf("SOFA TOOL get groups success: %u\n", mycmd->ret_groups);
    }

    jint = json_object_new_int(mycmd->ret_groups);

    /*Form the json object*/
    json_object_object_add(jobj, "groupNum", jint);

    /*Now printing the json object*/
    printf("The json object created: %s\n", json_object_to_json_string(jobj));
}

static int8_t **str_split (int8_t *a_str, const int8_t a_delim)
{
    int8_t **result;
    size_t count;
    int8_t *tmp, *last_comma, *token;
    int8_t delim[2];

    result = NULL;
    count = 0;
    tmp = a_str;
    last_comma = NULL;
    delim[0] = a_delim;
    delim[1] = 0;

    /* Count how many elements will be extracted. */
    while (*tmp) {
        if (a_delim == *tmp) {
            count++;
            last_comma = tmp;
        }
        tmp++;
    }

    /* Add space for trailing token. */
    count += last_comma < (a_str + strlen(a_str) - 1);

    /* Add space for terminating null string so caller
     knows where the list of returned strings ends. */
    count++;

    result = malloc(sizeof(int8_t *) * count);

    if (result) {
        size_t idx = 0;
        token = strtok(a_str, delim);

        while (token) {
            assert(idx < count);
            *(result + idx++) = strdup(token);
            token = strtok(0, delim);
        }
        assert(idx == count - 1);
        *(result + idx) = 0;
    }

    return result;
}

static void _show_getgroupdisks (user_cmd_t *ucmd)
{
    lfsm_groupdisks_cmd_t *mycmd;
    const int32_t limit = 64;
    const int32_t diskInfoRow = 3;
    int32_t i;
    int8_t string[limit][limit];
    json_object *jstring[limit];
    json_object *jarray;
    json_object **jsub_obj;
    uint32_t groupSize;
    int8_t **tokens;
    json_object *jobj;

    jobj = json_object_new_object();

    mycmd = (lfsm_groupdisks_cmd_t *)ucmd;

    if (ucmd->result) {
        //TODO: FSS-88 Can sofa monitor handle this and paring result?
        printf("SOFA TOOL get group disks fail ret %d\n", ucmd->result);
    } else {
        //printf("SOFA TOOL get group disks success: %s\n", mycmd->ret_groupdisks);
    }

    jarray = json_object_new_array();
    tokens = str_split(mycmd->ret_groupdisks, ',');

    if (tokens) {
        for (i = 0; *(tokens + i); i++) {
            if (i >= limit) {
                printf("Error!! find too many strings, limit = %d\n", limit);
                break;
            }
            sprintf(string[i], "%s", *(tokens + i));
            //printf("mycmd->ret_groupdisks=[%s]\n", string[i]);
            jstring[i] = json_object_new_string(string[i]);
            free(*(tokens + i));
        }
        free(tokens);
    }

    groupSize = atoi(string[0]);
    jsub_obj = malloc(sizeof(json_object *) * groupSize);

    for (i = 0; i < groupSize; i++) {
        jsub_obj[i] = json_object_new_object();
        json_object_object_add(jsub_obj[i], "id", jstring[i*diskInfoRow+1]);
        json_object_object_add(jsub_obj[i], "diskSize", jstring[i*diskInfoRow+2]);
        json_object_object_add(jsub_obj[i], "status", jstring[i*diskInfoRow+3]);
        json_object_array_add(jarray, jsub_obj[i]);
    }

    /*Form the json object*/
    json_object_object_add(jobj, "groupSize", jstring[0]);
    json_object_object_add(jobj, "physicalCapacity", jstring[1 + groupSize * diskInfoRow]);
    json_object_object_add(jobj, "capacity", jstring[2 + groupSize * diskInfoRow]);
    json_object_object_add(jobj, "usedSpace", jstring[3 + groupSize * diskInfoRow]);
    json_object_object_add(jobj, "disks", jarray);

    free(jsub_obj);

    /*Now printing the json object*/
    printf ("The json object created: %s\n", json_object_to_json_string(jobj));
}

static int32_t get_cn_disks(int8_t *disks){
   int32_t cn =0;
   int32_t i =0;
   if ( disks == NULL || strlen(disks) <=0 ){
        return cn;
   }
   for( i=0; i< strlen(disks) ; i++){
      if ( disks[i]  == ','){
         cn++;
      }
   }

   return cn+1;
}

static void _show_getsparedisks (user_cmd_t *ucmd)
{
    lfsm_sparedisks_cmd_t *mycmd;
    const int32_t limit = 64;
    const int32_t diskInfoRow = 3;
    int32_t i;
    int8_t string[limit][limit];
    json_object *jstring[limit];
    json_object *jarray, *jobj;
    json_object **jsub_obj;
    uint32_t spareSize;
    int8_t **tokens;

    jobj = json_object_new_object();
    mycmd = (lfsm_sparedisks_cmd_t *)ucmd;

    if (ucmd->result) {
        //TODO: FSS-88 Can sofa monitor handle this and paring result?
        printf("SOFA TOOL get spare disks fail ret %d\n", ucmd->result);
    } else {
        //printf("SOFA TOOL get spare disks success: %s\n", mycmd->ret_sparedisks);
    }

    jarray = json_object_new_array();
    tokens = str_split(mycmd->ret_sparedisks, ',');

    if (tokens) {
        for (i = 0; *(tokens + i); i++) {
            if (i >= limit) {
                printf("Error!! find too many strings, limit = %d\n", limit);
                break;
            }
            sprintf(string[i], "%s", *(tokens + i));
            //printf("mycmd->ret_sparedisks=[%s]\n", string[i]);
            jstring[i] = json_object_new_string(string[i]);
            free(*(tokens + i));
        }
        free(tokens);
    }

    spareSize = atoi(string[0]);
    jsub_obj = malloc(sizeof(json_object *) * spareSize);

    for( i = 0; i<spareSize; i++) {
        jsub_obj[i] = json_object_new_object();
        json_object_object_add(jsub_obj[i], "id", jstring[i*diskInfoRow+1]);
        json_object_object_add(jsub_obj[i], "diskSize", jstring[i*diskInfoRow+2]);
        json_object_object_add(jsub_obj[i], "status", jstring[i*diskInfoRow+3]);
        json_object_array_add(jarray, jsub_obj[i]);
    }

    /*Form the json object*/
    json_object_object_add(jobj, "disks", jarray);

    free(jsub_obj);

    /*Now printing the json object*/
    printf("The json object created: %s\n", json_object_to_json_string(jobj));
}

static void form_smart_json (int8_t **tokens, json_object *jarray)
{
    const int32_t limit = 128;
    int8_t string[limit][limit];
    json_object *jstring[limit];

    int32_t i;

    for (i = 0; *(tokens + i); i++) {
        if (i >= limit) {
            printf("Error!! find too many strings, limit = %d\n", limit);
            break;
        }

        sprintf(string[i], "%s", *(tokens + i));
        jstring[i] = json_object_new_string(string[i]);
        /*Form the json object*/
        json_object_array_add(jarray, jstring[i]);
    }
}

static void _show_getDiskSMART (user_cmd_t *ucmd)
{
    lfsm_diskSMART_cmd_t *mycmd;
    json_object *jarray, *jobj;
    int8_t tmpSplit[4096];
    int8_t **tokens, **tmp_tokens;
    int32_t j;

    jobj = json_object_new_object();
    mycmd = (lfsm_diskSMART_cmd_t *) ucmd;

    if (ucmd->result) {
        //TODO: FSS-88 Can sofa monitor handle this and paring result?
        printf("SOFA TOOL disk SMART fail ret %d\n", ucmd->result);
    } //else {
        //printf("SOFA TOOL disk SMART success info is: \n%s\n", mycmd->msginfo);
    //}

    jarray = json_object_new_array();

    tmp_tokens = str_split(mycmd->msginfo, ';');

    if (tmp_tokens) {
        for (j = 0; *(tmp_tokens + j); j++) {
            sprintf(tmpSplit, "%s", *(tmp_tokens + j));
            //printf("mycmd->msginfo=%s\n", tmpSplit);
            tokens = str_split(tmpSplit, '\n');
            if (tokens) {
                form_smart_json(tokens, jarray);
            }
        }
        free(tokens);
        free(tmp_tokens);
    }

    /*Form the json object*/
    json_object_object_add(jobj, "SMART", jarray);

    /*Now printing the json object*/
    printf("The json object created: %s\n", json_object_to_json_string(jobj));
}

static void _show_getGcInfo (user_cmd_t *ucmd)
{
    lfsm_gcInfo_cmd_t *mycmd;
    json_object *jint, *jobj;

    jobj = json_object_new_object();
    mycmd = (lfsm_gcInfo_cmd_t *)ucmd;

    if (ucmd->result) {
        //TODO: FSS-88 Can sofa monitor handle this and paring result?
        printf("SOFA TOOL get GC info fail ret %d\n", ucmd->result);
    } //else {
        //printf("SOFA TOOL get GC info success SEU reserve is: %u\n", mycmd->gcInfo.gcSeuResv);
    //}

    jint = json_object_new_int(mycmd->gcInfo.gcSeuResv);

    /*Form the json object*/
    json_object_object_add(jobj, "reserved", jint);

    /*Now printing the json object*/
    printf("The json object created: %s\n", json_object_to_json_string(jobj));
}

//TODO: Prevent magic number
static void _show_getperf (user_cmd_t *ucmd)
{
    lfsm_perf_cmd_t *mycmd;
    const int32_t limit = 600;
    const int32_t infoRow = 5;
    int32_t i;
    int8_t string[limit][64];
    json_object *jstring[limit];
    json_object *jarray, *jint;
    json_object **jsub_obj;
    uint32_t request_length;
    int8_t **tokens;
    json_object *jobj;

    jobj = json_object_new_object();

    mycmd = (lfsm_perf_cmd_t *) ucmd;

    if (ucmd->result) {
        //TODO: FSS-88 Can sofa monitor handle this and paring result?
        //printf("SOFA TOOL get perf fail ret %d\n", ucmd->result);
        jint = json_object_new_int(1);
        json_object_object_add(jobj, "errCode", jint);
        printf("The json object created: %s\n",
                json_object_to_json_string(jobj));
        return;
    } else {
        //printf("SOFA TOOL get perf success: mycmd->request_size = %d\n", mycmd->request_size);
    }

    //printf("mycmd->ret_perf=[%s]\n", mycmd->ret_perf);

    jarray = json_object_new_array();

    if (strstr(mycmd->ret_perf, ";") == NULL) {
        jint = json_object_new_int(1);
        json_object_object_add(jobj, "errCode", jint);
    } else {
        tokens = str_split(mycmd->ret_perf, ';');

        if (tokens) {
            for (i = 0; *(tokens + i); i++) {
                if (i >= limit) {
                    printf("Error!! find too many strings, limit = %d\n", limit);
                    break;
                }
                sprintf(string[i], "%s", *(tokens + i));
                //printf("split string[%d]=[%s]\n", i, string[i]);
                jstring[i] = json_object_new_string(string[i]);
                free(*(tokens + i));
            }
            free(tokens);
        }

        if (i >= limit || i < infoRow) {
            jint = json_object_new_int(0);
            json_object_object_add(jobj, "errCode", jint);
        } else {
            request_length = i / infoRow;

            //printf("i=%d, request_length=%d\n", i, request_length);

            jsub_obj = malloc(sizeof(json_object *) * request_length);

            for (i = 0; i < request_length; i++) {
                jsub_obj[i] = json_object_new_object();
                json_object_object_add(jsub_obj[i], "iops", jstring[i
                        * infoRow]);
                json_object_object_add(jsub_obj[i], "respond_iops", jstring[i * infoRow
                        + 1]);
                json_object_object_add(jsub_obj[i], "avg_latency", jstring[i
                        * infoRow + 2]);
                json_object_object_add(jsub_obj[i], "freelist", jstring[i
                        * infoRow + 3]);
                json_object_object_add(jsub_obj[i], "timestamp", jstring[i
                        * infoRow + 4]);
                json_object_array_add(jarray, jsub_obj[i]);
            }

            /*Form the json object*/
            jint = json_object_new_int(0);
            json_object_object_add(jobj, "errCode", jint);
            json_object_object_add(jobj, "data", jarray);

            free(jsub_obj);
        }
    }

    /*Now printing the json object*/
    printf("The json object created: %s\n", json_object_to_json_string(jobj));
}

void _show_checkSOFA (void)
{
    json_object *jint, *jobj;
    int8_t cmd[CMDSIZE];
    int8_t searchCmd[CMDSIZE];
    int8_t *cmdBuf, *ptr1;
    uint32_t cmdBufSize;

    jobj = json_object_new_object();

    cmdBufSize = RESP_BUF_SIZE;

    cmdBuf = malloc(RESP_BUF_SIZE);

    //syslog(LOG_INFO, "SOFA INFO lfsm _show_checkSOFA\n");

    snprintf(searchCmd, sizeof(searchCmd), "sofa service is running");
    snprintf(cmd, sizeof(cmd), "sh /usr/sofa/bin/check_sofa_service.sh");
    //syslog(LOG_INFO, "%s\n", cmd);

    ptr1 = CmdSearch(cmd, searchCmd, cmdBuf, cmdBufSize);
    if (ptr1 == NULL) {
        //syslog(LOG_INFO, "_show_checkSOFA: sofa service is not running!!");
        jint = json_object_new_int(1);
        json_object_object_add(jobj, "errCode", jint);
        printf("The json object created: %s\n",
                json_object_to_json_string(jobj));
    }

    free(cmdBuf);
}

void _show_getstatus (void)
{
    json_object *jint, *jobj, *jstring;
    int8_t cmd[CMDSIZE];
    int8_t searchCmd[CMDSIZE];
    int8_t *cmdBuf, *ptr1;
    uint32_t cmdBufSize;

    jobj = json_object_new_object();

    cmdBufSize = RESP_BUF_SIZE;

    cmdBuf = malloc(RESP_BUF_SIZE);

    //syslog(LOG_INFO, "SOFA INFO lfsm _show_getstatus\n");

    snprintf(searchCmd, sizeof(searchCmd), "sofa service is running");
    snprintf(cmd, sizeof(cmd), "sh /usr/sofa/bin/check_sofa_service.sh");
    //syslog(LOG_INFO, "%s\n", cmd);

    ptr1 = CmdSearch(cmd, searchCmd, cmdBuf, cmdBufSize);
    if (ptr1 != NULL) {
        /*sofa is running*/
        jstring = json_object_new_string("True");
        json_object_object_add(jobj, "isRunning", jstring);
        jint = json_object_new_int(0);
        json_object_object_add(jobj, "errCode", jint);

    } else {
        syslog(LOG_INFO, "_show_getstatus sofa service is not running !!");
        jstring = json_object_new_string("False");
        json_object_object_add(jobj, "isRunning", jstring);
        jint = json_object_new_int(0);
        json_object_object_add(jobj, "errCode", jint);
    }

    free(cmdBuf);

    printf("The json object created: %s\n", json_object_to_json_string(jobj));
}

void _show_getproperties (void)
{
    json_object *jint, *jobj, *jstring;
    int8_t cmd[CMDSIZE];
    int8_t searchCmd[CMDSIZE];
    int8_t *cmdBuf, *ptr1;
    uint32_t cmdBufSize;

    //syslog(LOG_INFO, "SOFA INFO lfsm _show_getproperties\n");

    jobj = json_object_new_object();
    cmdBufSize = RESP_BUF_SIZE;
    cmdBuf = malloc(RESP_BUF_SIZE);

    snprintf(searchCmd, sizeof(searchCmd), "configuration.xsl");
    snprintf(cmd, sizeof(cmd), "cat /usr/data/sofa/config/sofa_config.xml");
    ptr1 = CmdSearch(cmd, searchCmd, cmdBuf, cmdBufSize);
    if (ptr1 != NULL) {
        jint = json_object_new_int(0);
        json_object_object_add(jobj, "errCode", jint);
        jstring = json_object_new_string(cmdBuf);
        json_object_object_add(jobj, "Properties", jstring);
    } else {
        syslog(LOG_INFO, "_show_getproperties can not get file !!");
        jint = json_object_new_int(1);
        json_object_object_add(jobj, "errCode", jint);
    }

    free(cmdBuf);

    system("cat /usr/data/sofa/config/sofa_config.xml");

    //printf("The json object created: %s\n", json_object_to_json_string(jobj));
}

void _show_activate (void)
{
    json_object *jint, *jobj, *jstring;
    int8_t cmd[CMDSIZE];
    int8_t searchCmd[CMDSIZE];
    int8_t *cmdBuf, *ptr1;
    uint32_t cmdBufSize;

    //syslog(LOG_INFO, "SOFA INFO lfsm _show_activate\n");

    jobj = json_object_new_object();
    cmdBufSize = RESP_BUF_SIZE;
    cmdBuf = malloc(RESP_BUF_SIZE);

    snprintf(searchCmd, sizeof(searchCmd), "sofa service is running");
    snprintf(cmd, sizeof(cmd), "sh /usr/sofa/bin/check_sofa_service.sh");
    syslog(LOG_INFO, "%s\n", cmd);

    ptr1 = CmdSearch(cmd, searchCmd, cmdBuf, cmdBufSize);
    if (ptr1 != NULL) {
        /*sofa is running*/
        jint = json_object_new_int(0);
        json_object_object_add(jobj, "errCode", jint);

    } else {
        system("sh /usr/sofa/bin/start-sofa.sh > /dev/null");
        ptr1 = CmdSearch(cmd, searchCmd, cmdBuf, cmdBufSize);
        if (ptr1 != NULL) {
            /*sofa is running*/
            jint = json_object_new_int(0);
            json_object_object_add(jobj, "errCode", jint);
        } else {
            /*sofa activate error*/
            jint = json_object_new_int(1);
            json_object_object_add(jobj, "errCode", jint);
        }
    }

    free(cmdBuf);

    printf("The json object created: %s\n", json_object_to_json_string(jobj));
}

//TODO: using variable for /usr/data/sofa/config/sofa_config.xml
void _show_setgcratio (uint32_t on_ratio, uint32_t off_ratio)
{
    json_object *jint, *jobj, *jstring;
    int8_t cmd[CMDSIZE];
    int8_t searchCmd[CMDSIZE];
    int8_t *cmdBuf, *ptr1;
    uint32_t cmdBufSize;

    //syslog(LOG_INFO, "SOFA INFO lfsm _show_setgcratio\n");

    jobj = json_object_new_object();
    cmdBufSize = RESP_BUF_SIZE;
    cmdBuf = malloc(RESP_BUF_SIZE);

    snprintf(searchCmd, sizeof(searchCmd), "sofa service is running");
    snprintf(cmd, sizeof(cmd), "sh /usr/sofa/bin/check_sofa_service.sh");
    //syslog(LOG_INFO, "%s\n", cmd);

    ptr1 = CmdSearch(cmd, searchCmd, cmdBuf, cmdBufSize);
    if (ptr1 != NULL) {
        //sofa is running

        //set ratio
        snprintf(cmd, sizeof(cmd),
                "echo \"lfsm_gc_on_lower_ratio=%u\" > /proc/lfsm/lfsm_config",
                on_ratio);
        system(cmd);
        snprintf(cmd, sizeof(cmd),
                "echo \"lfsm_gc_off_upper_ratio=%u\" > /proc/lfsm/lfsm_config",
                off_ratio);
        system(cmd);

        //change config
        snprintf(cmd, sizeof(cmd),
                "/usr/sofa/bin/sofaadm --config --updatefile cfgfn=/usr/data/sofa/config/sofa_config.xml lfsm_gc_on_lower_ratio %u",
                on_ratio);
        system(cmd);
        snprintf(
                cmd,
                sizeof(cmd),
                "/usr/sofa/bin/sofaadm --config --updatefile cfgfn=/usr/data/sofa/config/sofa_config.xml lfsm_gc_off_upper_ratio %u",
                off_ratio);
        system(cmd);

        jint = json_object_new_int(0);
        json_object_object_add(jobj, "errCode", jint);

    } else {
        /*sofa error*/
        jint = json_object_new_int(1);
        json_object_object_add(jobj, "errCode", jint);
    }

    free(cmdBuf);

    printf("The json object created: %s\n", json_object_to_json_string(jobj));
}

void _show_setgcreserve (uint32_t reserve_value)
{
    json_object *jint, *jobj, *jstring;
    int8_t cmd[CMDSIZE];
    int8_t searchCmd[CMDSIZE];
    int8_t *cmdBuf, *ptr1;
    uint32_t cmdBufSize;
    uint64_t timestamp;
    uint64_t timestamp_diff;

    //syslog(LOG_INFO, "SOFA INFO lfsm _show_setgcreserve\n");

    jobj = json_object_new_object();
    cmdBufSize = RESP_BUF_SIZE;
    cmdBuf = malloc(RESP_BUF_SIZE);

    //check sofa restart ok
    snprintf(searchCmd, sizeof(searchCmd), "timestamp:");
    snprintf(cmd, sizeof(cmd), "cat /tmp/sofa_restart_tmp.txt | awk '{print $2}'");
    ptr1 = CmdSearch(cmd, searchCmd, cmdBuf, cmdBufSize);
    if (ptr1 != NULL) {
        timestamp = (uint64_t) time(NULL);
        timestamp_diff = timestamp - atof(ptr1 + 10);
        printf("timestamp_diff = %llu\n", timestamp_diff);

        snprintf(searchCmd, sizeof(searchCmd), "restart");
        snprintf(cmd, sizeof(cmd), "cat /tmp/sofa_restart_tmp.txt");

        ptr1 = CmdSearch(cmd, searchCmd, cmdBuf, cmdBufSize);
        if ((ptr1 != NULL) && (timestamp_diff < 300)) { //300 seconds out-of-date
            /*sofa restart*/
            free(cmdBuf);
            jint = json_object_new_int(1);
            json_object_object_add(jobj, "errCode", jint);
            printf("The json object created: %s\n", json_object_to_json_string(jobj));
            return;
        }
    }

    //check end

    snprintf(searchCmd, sizeof(searchCmd), "sofa service is running");
    snprintf(cmd, sizeof(cmd), "sh /usr/sofa/bin/check_sofa_service.sh");
    //syslog(LOG_INFO, "%s\n", cmd);

    ptr1 = CmdSearch(cmd, searchCmd, cmdBuf, cmdBufSize);
    if (ptr1 != NULL) {
        //sofa is running
        //sofa is restarting
        timestamp = (uint64_t) time(NULL);
        snprintf(cmd, sizeof(cmd),
                "echo \"restarting timestamp:%llu\" > /tmp/sofa_restart_tmp.txt", timestamp);
        system(cmd);

        //step 1 set lfsm_seu_reserved
        snprintf(cmd, sizeof(cmd),
                "/usr/sofa/bin/sofaadm --config --updatefile cfgfn=/usr/data/sofa/config/sofa_config.xml lfsm_seu_reserved %u",
                reserve_value);
        system(cmd);

        //step 2 set lfsm_reinstall = 1
        snprintf(cmd, sizeof(cmd),
                "/usr/sofa/bin/sofaadm --config --updatefile cfgfn=/usr/data/sofa/config/sofa_config.xml lfsm_reinstall 1");
        system(cmd);

        //step 3 restart sofa
        snprintf(cmd, sizeof(cmd), "sh /usr/sofa/bin/stop-sofa.sh",
                reserve_value);
        system(cmd);

        sleep(5);

        snprintf(cmd, sizeof(cmd), "sh /usr/sofa/bin/start-sofa.sh",
                reserve_value);
        system(cmd);

        system("echo running  > /tmp/sofa_restart_tmp.txt");

        //step 4 set lfsm_reinstall = 0
        snprintf(cmd, sizeof(cmd),
                "/usr/sofa/bin/sofaadm --config --updatefile cfgfn=/usr/data/sofa/config/sofa_config.xml lfsm_reinstall 0");
        system(cmd);

        jint = json_object_new_int(0);
        json_object_object_add(jobj, "errCode", jint);

    } else {
        /*sofa error*/
        jint = json_object_new_int(1);
        json_object_object_add(jobj, "errCode", jint);
    }

    free(cmdBuf);

    printf("The json object created: %s\n", json_object_to_json_string(jobj));
}

void show_lfsmcmd_result (user_cmd_t *ucmd)
{
    switch (ucmd->subopcode) {
    case lfsm_subop_getPhysicalCap:
        _show_getPhysicalCap(ucmd);
        break;
    case lfsm_subop_getCapacity:
        _show_getCapacity(ucmd);
        break;
    case lfsm_subop_getgroups:
        _show_getgroups(ucmd);
        break;
    case lfsm_subop_getgroupdisks:
        _show_getgroupdisks(ucmd);
        break;
    case lfsm_subop_getsparedisks:
        _show_getsparedisks(ucmd);
        break;
    case lfsm_subop_getDiskSMART:
        _show_getDiskSMART(ucmd);
        break;
    case lfsm_subop_getGcInfo:
        _show_getGcInfo(ucmd);
        break;
    case lfsm_subop_getperf:
        _show_getperf(ucmd);
        break;

    default:
        syslog(LOG_ERR, "[SOFA ADM] unknow opcode for show %d\n", ucmd->opcode);
        break;
    }
}
