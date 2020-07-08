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

#ifndef _LFSM_CFG_H_
#define _LFSM_CFG_H_

#define MAX_DEV_PATH 16
#define MAX_SUPPORT_SSD 64
#define MAX_STR_LEN 64
#define MAX_SUPPORT_Thread    64

#define MAX_SETTING_LEN      128
#define GC_PARA_RATIO_BASE  1000

#define LFSM_VERSION    "1.0.00"

struct lfsm_testCfg {
    bool crashSimu;
};

typedef struct lfsm_config {
    uint32_t resetData;
    uint32_t cnt_ssds;
    int8_t ssd_path[MAX_SUPPORT_SSD][MAX_DEV_PATH];
    uint32_t ssds_hpeu;
    uint32_t cnt_pgroup;
    uint32_t cnt_io_threads;
    int32_t ioT_vcore_map[MAX_SUPPORT_Thread];
    uint32_t cnt_bh_threads;
    int32_t bhT_vcore_map[MAX_SUPPORT_Thread];
    uint32_t cnt_gc_rthreads;
    uint32_t cnt_gc_wthreads;
    uint32_t gc_seu_reserved;
    uint32_t gc_chk_intvl;      //interval for checking free EUs and start GC, unit: ms
    uint32_t gc_upper_ratio;    //ratio of free EUs to start GC
    uint32_t gc_lower_ratio;    //ratio of free EUs to stop GC
    int8_t lfsm_ver[MAX_STR_LEN];
    int8_t osdisk_name[MAX_STR_LEN];
    int32_t max_disk_partition;
    uint32_t mem_bioset_size;
    struct lfsm_testCfg testCfg;
} lfsm_cfg_t;

extern lfsm_cfg_t lfsmCfg;

extern int32_t set_lfsm_config(uint32_t cfgItem, uint32_t val, int8_t * strVal,
                               int8_t * sub_setting, uint32_t set_len);
extern void init_lfsm_config(void);
extern int32_t show_lfsm_config(int8_t * buffer, int32_t buff_size);
extern void exec_lfsm_config_cmdstr(int8_t * buffer, int32_t buff_size);
#endif /* _LFSM_CFG_H_ */
