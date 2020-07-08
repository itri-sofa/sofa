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
