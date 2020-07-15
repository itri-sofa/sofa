/*
 * Copyright (c) 2015-2020 Industrial Technology Research Institute.
 *
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 *
 *
 *
 *
 *
 *
 *  
 */

#ifndef SOFA_CONFIG_H_
#define SOFA_CONFIG_H_

#define NULL_CONFIG_VAL 0xffffffffffffffffL
#define DRV_FN_MAX_LEN  64

#define MAX_DEV_PATH 16
#define MAX_SUPPORT_SSD 64

//NOTE: size of devlist should large than MAX_SUPPORT_SSD
extern int8_t *devlist[];

typedef enum {
    config_reinstall = 0,
    config_cn_ssds,
    config_ssds_hpeu,
    config_cn_pgroup,
    config_lfsm_ioT,
    config_lfsm_bhT,
    config_lfsm_gcrT,
    config_lfsm_gcwT,
    config_lfsm_seuRsv,
    config_max_vols,
    config_max_volcap,
    config_vdisk_name,
    config_osdisk_name,
    config_gc_chk_intvl,
    config_gc_off_upper_ratio,
    config_gc_on_lower_ratio,
    config_max_disk_part,
#ifdef SOFA_USER_SPACE
    config_srv_port,
    config_lfsmdrvM_fn,
    config_sofadrvM_fn,
    config_iosched,
    config_sofa_dir,
    config_hba_irqsmp_enable,
    config_hba_intrname,
#endif
    config_end,
} config_item_index;

#ifndef SOFA_USER_SPACE
typedef struct sofa_config {
    int16_t reinstall;
    int32_t num_ssds;
    int8_t *ssds_setting;
    int32_t num_ssds_hpeu;
    int32_t num_pgroups;
    int32_t lfsm_ioT;
    int8_t *lfsm_ioT_setting;
    int32_t lfsm_bhT;
    int8_t *lfsm_bhT_setting;
    int32_t lfsm_gcreadT;
    int32_t lfsm_gcwriteT;
    int32_t lfsm_seu_rsv;
    uint32_t lfsm_gc_chk_intvl;
    uint32_t lfsm_gc_off_upper_ratio;
    uint32_t lfsm_gc_on_lower_ratio;
    int32_t max_vols;
    int64_t max_volcaps_bytes;
    int8_t vdisk_name[DRV_FN_MAX_LEN];
} sofa_cfg_t;

extern sofa_cfg_t sofacfg;
#endif

extern void show_sofa_config(void);
extern void get_config_name_val(config_item_index index, int8_t ** name,
                                int64_t * intVal, int8_t ** strVal,
                                int8_t ** strValset);
extern void set_config_item_index(config_item_index index, int8_t * newStrVal,
                                  int64_t newIntVal);
extern void set_config_item(int8_t * name, int8_t * val, int8_t * valSet);
extern int32_t get_config_idx_numItems(config_item_index index);
extern int32_t get_config_name_val_allItems(config_item_index index,
                                            int8_t ** name, int64_t ** intVal,
                                            int8_t ** strVal,
                                            int8_t ** strValSet);

#ifdef SOFA_USER_SPACE

#define ITRI_IO_SCHED   "noop2"

extern void config_sofa_UsrD_pre(void);
extern void config_sofa_UsrD_post(void);
extern void config_sofa_UsrD_exit(void);
extern int32_t config_sofa_drv(int32_t chrdev_fd);
extern int32_t load_sofa_config(int8_t * config_file);
extern void save_sofa_config(void);
#endif

extern void init_sofa_config(void);
extern void rel_sofa_config(void);

#endif /* SOFA_CONFIG_H_ */
