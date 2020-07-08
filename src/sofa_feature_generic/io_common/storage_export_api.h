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

#ifndef IO_COMMON_STORAGE_EXPORT_API_H_
#define IO_COMMON_STORAGE_EXPORT_API_H_

typedef enum {
    cfglfsm_reinstall,
    cfglfsm_cn_ssds,
    cfglfsm_ssds_hpeu,
    cfglfsm_cn_pgroup,
    cfglfsm_io_threads,
    cfglfsm_bh_threads,
    cfglfsm_gc_rthreads,
    cfglfsm_gc_wthreads,
    cfglfsm_seu_reserved,
    cfglfsm_raid_info,
    cfglfsm_spare_disks,
    cfglfsm_capacity,           //capacity exported by lfsm
    cfglfsm_physical_cap,       //capacity of sum of all disks
    cfglfsm_osdisk,
    cfglfsm_gc_chk_intvl,
    cfglfsm_gc_off_upper_ratio,
    cfglfsm_gc_on_lower_ratio,
    cfglfsm_max_disk_part,
} lfsm_info_index_t;

extern int32_t lfsm_set_config(uint32_t index, uint32_t val, int8_t * strVal,
                               int8_t * sub_setting, uint32_t set_len);
extern int32_t lfsm_init_driver(void);
extern int32_t lfsm_submit_bio(uint64_t sect_s, uint32_t sect_len,
                               struct bio *mybio);
extern int32_t lfsm_submit_MD_request(uint64_t lb_src, uint32_t lb_len,
                                      uint64_t lb_dest);
extern uint64_t lfsm_get_disk_capacity(void);
extern uint32_t lfsm_get_gc_seu_resv(void);

extern int8_t lfsm_get_sparedisks(int8_t * spare_msg);
extern int8_t lfsm_get_groupdisks(uint32_t groupId, int8_t * gdisk_msg);
extern uint32_t lfsm_get_groups(void);
extern uint64_t lfsm_get_physicalCap(void);

extern uint64_t lfsm_cal_resp_iops(void);
extern uint64_t lfsm_cal_req_iops(void);

#endif /* IO_COMMON_STORAGE_EXPORT_API_H_ */
