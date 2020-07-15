/*
 * SPDX-License-Identifier: GPL-2.0-or-later OR Apache-2.0
 * Copyright (C) 2015-2020 Industrial Technology Research Institute
 *
 *
 *
 *  
 */

#include <linux/types.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include "../common/drv_userD_common.h"
#include "../common/sofa_common.h"
#include "../common/sofa_common_def.h"
#include "drv_lfsmcmd_handler_private.h"
#include "drv_lfsmcmd_handler.h"

#include <linux/bio.h>
#include "../sofa_feature_generic/io_common/storage_api_wrapper.h"
#include "../sofa_feature_generic/io_common/storage_export_api.h"

getGcInfo_resp_t getGcInfo(usercmd_para_t * ucmdPara)
{
    gcInfo_t gcInfo;
    getGcInfo_resp_t ret;

    ret = getGcInfo_OK;

    //TODO: check gc info structure
    gcInfo.gcSeuResv = get_gc_seu_resv();
    gcInfo.gcInfoTest64 = 0x1122334455667788;
    gcInfo.gcInfoTest = 0x12345678;

    //sofa_printk(LOG_INFO, "getGcInfo... gcInfo.gcSeuResv = %u\n",  gcInfo.gcSeuResv);

    memcpy(ucmdPara->cmd_resp, (int8_t *) & gcInfo, sizeof(gcInfo_t));

    return ret;
}

getGcInfo_resp_t getperf(usercmd_para_t * ucmdPara)
{
    perfInfo_t perfInfo;
    getperf_resp_t ret;

    ret = getperf_OK;

    //TODO: check gc info structure
    perfInfo.cal_request_iops = cal_req_iops_lfsm();
    perfInfo.cal_respond_iops = cal_resp_iops_lfsm();
    perfInfo.cal_avg_latency = 0;
    perfInfo.cal_free_list = 0;

    //sofa_printk(LOG_INFO, "getperf... info.cal_respond_iops = %llu\n",  perfInfo.cal_respond_iops);

    memcpy(ucmdPara->cmd_resp, (int8_t *) & perfInfo, sizeof(perfInfo_t));

    return ret;
}

getCapacity_resp_t getCapacity(usercmd_para_t * ucmdPara)
{
    uint64_t capSize;
    getCapacity_resp_t ret;

    ret = getCapacity_OK;

    capSize = get_storage_capacity();

    //sofa_printk(LOG_INFO, "getCapacity... capSize = %llu\n", capSize);

    memcpy(ucmdPara->cmd_resp, (int8_t *) & capSize, sizeof(uint64_t));

    return ret;
}

getPhysicalCap_resp_t getPhysicalCap(usercmd_para_t * ucmdPara)
{
    uint64_t capSize;
    getPhysicalCap_resp_t ret;

    ret = getPhysicalCap_OK;

    capSize = get_physicalCap();

    //sofa_printk(LOG_INFO, "getPhysicalCap... capSize = %llu\n", capSize);

    memcpy(ucmdPara->cmd_resp, (int8_t *) & capSize, sizeof(uint64_t));

    return ret;
}

getgroups_resp_t getgroups(usercmd_para_t * ucmdPara)
{
    uint32_t cnt_pgroup;
    getgroups_resp_t ret;

    ret = getgroups_OK;

    cnt_pgroup = get_groups();

    //sofa_printk(LOG_INFO, "getgroups... cnt_pgroup = %u\n", cnt_pgroup);

    memcpy(ucmdPara->cmd_resp, (int8_t *) & cnt_pgroup, sizeof(uint32_t));

    return ret;
}

getgroupdisks_resp_t getgroupdisks(usercmd_para_t * ucmdPara)
{
    int8_t gdisk_msg[GROUPDISK_OUTSIZE];
    uint32_t groupId;
    getgroupdisks_resp_t ret;

    ret = getgroupdisks_OK;

    memcpy((int8_t *) & groupId, ucmdPara->cmd_para, sizeof(uint32_t));

    get_groupdisks(groupId, gdisk_msg);

    //sofa_printk(LOG_INFO, "getgroupdisks, groupId=%u, gdisk_msg = %s\n", groupId, gdisk_msg);

    memcpy(ucmdPara->cmd_resp, gdisk_msg, GROUPDISK_OUTSIZE);

    return ret;
}

getsparedisks_resp_t getsparedisks(usercmd_para_t * ucmdPara)
{
    int8_t spare_msg[SPARE_OUTSIZE];
    getsparedisks_resp_t ret;

    ret = getsparedisks_OK;

    get_sparedisks(spare_msg);

    //sofa_printk(LOG_INFO, "getsparedisks... spare_msg = %s\n", spare_msg);

    memcpy(ucmdPara->cmd_resp, spare_msg, SPARE_OUTSIZE);

    return ret;
}

static int32_t _handle_getPhysicalCap_cmd(usercmd_para_t * ucmdPara)
{
    //sofa_printk(LOG_INFO, "CHRDEV INFO _handle_getPhysicalCap_cmd\n");

    ucmdPara->cmd_result = getPhysicalCap(ucmdPara);

    return 0;
}

static int32_t _handle_getCapacity_cmd(usercmd_para_t * ucmdPara)
{
    //sofa_printk(LOG_INFO, "CHRDEV INFO _handle_getCapacity_cmd\n");

    ucmdPara->cmd_result = getCapacity(ucmdPara);

    return 0;
}

static int32_t _handle_getgroups_cmd(usercmd_para_t * ucmdPara)
{
    //sofa_printk(LOG_INFO, "CHRDEV INFO _handle_getgroups_cmd\n");

    ucmdPara->cmd_result = getgroups(ucmdPara);

    return 0;
}

static int32_t _handle_getgroupdisks_cmd(usercmd_para_t * ucmdPara)
{

    uint32_t groupId;

    memcpy((int8_t *) & groupId, ucmdPara->cmd_para, sizeof(uint32_t));

    //sofa_printk(LOG_INFO, "CHRDEV INFO group disk groupId %u\n", groupId);

    ucmdPara->cmd_result = getgroupdisks(ucmdPara);

    return 0;
}

static int32_t _handle_getsparedisks_cmd(usercmd_para_t * ucmdPara)
{
    //sofa_printk(LOG_INFO, "CHRDEV INFO _handle_getsparedisks_cmd\n");

    ucmdPara->cmd_result = getsparedisks(ucmdPara);

    return 0;
}

static int32_t _handle_getGcInfo_cmd(usercmd_para_t * ucmdPara)
{
    //sofa_printk(LOG_INFO, "CHRDEV INFO _handle_getGcInfo_cmd\n");

    ucmdPara->cmd_result = getGcInfo(ucmdPara);

    return 0;
}

static int32_t _handle_getperf_cmd(usercmd_para_t * ucmdPara)
{
    //sofa_printk(LOG_INFO, "CHRDEV INFO _handle_getperf_cmd\n");

    ucmdPara->cmd_result = getperf(ucmdPara);

    return 0;
}

int32_t handle_lfsm(usercmd_para_t * ucmdPara)
{
    int32_t ret;

    ret = 0;

//    sofa_printk(LOG_INFO, "CHRDEV INFO handle lfsm cmd with subopcode %u\n",
//            ucmdPara->subopcode);

    switch (ucmdPara->subopcode) {
    case lfsm_subop_getPhysicalCap:
        ret = _handle_getPhysicalCap_cmd(ucmdPara);
        break;
    case lfsm_subop_getCapacity:
        ret = _handle_getCapacity_cmd(ucmdPara);
        break;
    case lfsm_subop_getgroups:
        ret = _handle_getgroups_cmd(ucmdPara);
        break;
    case lfsm_subop_getgroupdisks:
        ret = _handle_getgroupdisks_cmd(ucmdPara);
        break;

    case lfsm_subop_getsparedisks:
        ret = _handle_getsparedisks_cmd(ucmdPara);
        break;
    case lfsm_subop_getDiskSMART:
        ucmdPara->cmd_result = -1;
        ret = 0;
        sofa_printk(LOG_WARN,
                    "[SOFA] USERD INFO not support lfsm_subop_getDiskSMART function\n");
        break;
    case lfsm_subop_getGcInfo:
        ret = _handle_getGcInfo_cmd(ucmdPara);
        break;
    case lfsm_subop_getperf:
        ret = _handle_getperf_cmd(ucmdPara);
        break;
    default:
        sofa_printk(LOG_WARN, "CHRDEV WARN unknown lfsm cmd subopcode %d\n",
                    ucmdPara->subopcode);
        break;
    }

    return ret;
}
