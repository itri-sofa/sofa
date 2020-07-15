/*
 * SPDX-License-Identifier: GPL-2.0-or-later OR Apache-2.0
 * Copyright (C) 2015-2020 Industrial Technology Research Institute
 *
 *
 *
 *  
 */

#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/types.h>
#include <linux/bio.h>
#include "storage_api_wrapper.h"
#include "storage_export_api.h"
#include "../../common/sofa_config.h"

int32_t storage_set_config(uint32_t type, uint32_t val, int8_t * valStr,
                           int8_t * setting, uint32_t set_len)
{
    int32_t ret;

    ret = 0;

    switch (type) {
    case config_reinstall:
        ret = lfsm_set_config(cfglfsm_reinstall, val, NULL, NULL, 0);
        break;
    case config_cn_ssds:
        ret = lfsm_set_config(cfglfsm_cn_ssds, val, NULL, setting, set_len);
        break;
    case config_ssds_hpeu:
        ret = lfsm_set_config(cfglfsm_ssds_hpeu, val, NULL, NULL, 0);
        break;
    case config_cn_pgroup:
        ret = lfsm_set_config(cfglfsm_cn_pgroup, val, NULL, NULL, 0);
        break;
    case config_lfsm_ioT:
        ret = lfsm_set_config(cfglfsm_io_threads, val, NULL, setting, set_len);
        break;
    case config_lfsm_bhT:
        ret = lfsm_set_config(cfglfsm_bh_threads, val, NULL, setting, set_len);
        break;
    case config_lfsm_gcrT:
        ret = lfsm_set_config(cfglfsm_gc_rthreads, val, NULL, NULL, 0);
        break;
    case config_lfsm_gcwT:
        ret = lfsm_set_config(cfglfsm_gc_wthreads, val, NULL, NULL, 0);
        break;
    case config_lfsm_seuRsv:
        ret = lfsm_set_config(cfglfsm_seu_reserved, val, NULL, NULL, 0);
        break;
    case config_osdisk_name:
        ret = lfsm_set_config(cfglfsm_osdisk, 0, valStr, NULL, 0);
        break;
    case config_gc_chk_intvl:
        ret = lfsm_set_config(cfglfsm_gc_chk_intvl, val, NULL, NULL, 0);
        break;
    case config_gc_off_upper_ratio:
        ret = lfsm_set_config(cfglfsm_gc_off_upper_ratio, val, NULL, NULL, 0);
        break;
    case config_gc_on_lower_ratio:
        ret = lfsm_set_config(cfglfsm_gc_on_lower_ratio, val, NULL, NULL, 0);
        break;
    default:
        break;
    }

    return ret;
}

int32_t storage_init(void)
{
    return lfsm_init_driver();
}

int32_t submit_IO_request(uint64_t sect_s, uint32_t sect_len, struct bio *mybio)
{
    return lfsm_submit_bio(sect_s, sect_len, mybio);
}

int32_t submit_MD_request(uint64_t lb_src, uint32_t lb_len, uint64_t lb_dest)
{
    return lfsm_submit_MD_request(lb_src, lb_len, lb_dest);
}

//NOTE: capacity in bytes
uint64_t get_storage_capacity(void)
{
    return lfsm_get_disk_capacity();
}

//gc seu reserve %
uint32_t get_gc_seu_resv(void)
{
    return lfsm_get_gc_seu_resv();
}

uint64_t get_physicalCap(void)
{
    return lfsm_get_physicalCap();
}

uint32_t get_groups(void)
{
    return lfsm_get_groups();
}

int8_t get_groupdisks(uint32_t groupId, int8_t * gdisk_msg)
{
    return lfsm_get_groupdisks(groupId, gdisk_msg);
}

int8_t get_sparedisks(int8_t * gdisk_msg)
{
    return lfsm_get_sparedisks(gdisk_msg);
}

uint64_t cal_resp_iops_lfsm(void)
{
    return lfsm_cal_resp_iops();
}

uint64_t cal_req_iops_lfsm(void)
{
    return lfsm_cal_req_iops();
}
