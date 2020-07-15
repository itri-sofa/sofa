/*
 * SPDX-License-Identifier: GPL-2.0-or-later OR Apache-2.0
 * Copyright (C) 2015-2020 Industrial Technology Research Institute
 *
 *
 *
 *  
 */

#ifndef STORAGE_CORE_LFSM_LFSM_EXPORT_INC_H_
#define STORAGE_CORE_LFSM_LFSM_EXPORT_INC_H_

typedef enum {
    lfsm_reinstall,
    lfsm_cn_ssds,
    lfsm_ssds_hpeu,
    lfsm_cn_pgroup,
    lfsm_io_threads,
    lfsm_bh_threads,
    lfsm_gc_rthreads,
    lfsm_gc_wthreads,
    lfsm_gc_seu_reserved,
    lfsm_raid_info,
    lfsm_spare_disks,
    lfsm_capacity,              //capacity exported by lfsm
    lfsm_physical_cap,          //capacity of sum of all disks
    lfsm_osdisk,
    lfsm_gc_chk_intvl,
    lfsm_gc_off_upper_ratio,
    lfsm_gc_on_lower_ratio,
    lfsm_max_disk_part,
} lfsm_info_index_t;

#endif /* STORAGE_CORE_LFSM_LFSM_EXPORT_INC_H_ */
