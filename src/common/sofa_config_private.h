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

#ifndef SOFA_CONFIG_PRIVATE_H_
#define SOFA_CONFIG_PRIVATE_H_

#define STR_REINSTALL   "lfsm_reinstall"
#define DEFVAL_REINSTALL    1

#define STR_CN_SSDs "lfsm_cn_ssds"
#define DEFVAL_CN_SSDs  2
#define DEFVAL_CN_SSD_LIST  "b-c"

#define STR_CN_SSDs_HPEU    "cn_ssds_per_hpeu"
#define DEFVAL_CN_SSDs_HPEU 1

#define STR_CN_PGROUP   "lfsm_cn_pgroup"
#define DEFVAL_CN_PGROUP    2

#define STR_LFSM_IOT    "lfsm_io_thread"
#define DEFVAL_LFSM_IOT 7
#define DEFVAL_LFSM_IOT_LIST    "-1"

#define STR_LFSM_BHT    "lfsm_bh_thread"
#define DEFVAL_LFSM_BHT 3
#define DEFVAL_LFSM_BHT_LIST    "-1"

#define STR_LFSM_GCRT    "lfsm_gc_rthread"
#define DEFVAL_LFSM_GCRT 1

#define STR_LFSM_GCWT    "lfsm_gc_wthread"
#define DEFVAL_LFSM_GCWT 1

#define STR_LFSM_SEU_RESERVED "lfsm_seu_reserved"
#define DEFVAL_SEU_RESERVED   250

//NOTE: Unit : MB, 64T
#define STR_MAX_SUPPORT_VOLCAP     "max_vol_capacity"
#define DEFVAL_MAX_SUPPORT_VOLCAP  64*1024L*1024L

#define STR_VDISK_NAME  "vdisk_name"
#define DEFVAL_VDISK_NAME   "sofa"

#define STR_OSDISK_NAME    "os_disk_name"
#define DEFVAL_OSDISK_NAME "sda"

#define STR_GC_CHK_INTVL    "lfsm_gc_chk_intvl"
#define DEFVAL_GC_CHK_INTVL 1000

#define STR_GC_OFF_RATIO    "lfsm_gc_off_upper_ratio"
#define DEFVAL_GC_OFF_RATIO 250

#define STR_GC_ON_RATIO    "lfsm_gc_on_lower_ratio"
#define DEFVAL_GC_ON_RATIO 125

#ifdef SOFA_USER_SPACE
#define STR_SRV_PORT    "service_port"
#define DEFVAL_SRV_PORT 9898

//#define STR_SOFA_DRVFN  "lfsm"
#define STR_LFSM_DRVFN  "lfsm_module_fn"
#define DEFVAL_LFSM_DRVFN  "lfsmdr"

#define STR_SOFA_DRV_FN  "sofa_module_fn"
#define DEFVAL_SOFA_DRV_FN   "sofa"

#define STR_SOFA_DIR  "sofa_deploy_dir"
#define DEFVAL_SOFA_DIR  "/usr/sofa/"

#define STR_IO_SCHED    "iosched"
#define DEFVAL_IO_SCHED "none"

#define DEFVAL_RQ_AFFINITY  1

#define STR_HBA_IRQSMP_AUTOSET "hba_irqsmp_autoset"
#define DEFVAL_HBA_IRQSMP_AUTOSET   0

#define STR_HBA_INTR_NAME    "hba_intr_name"
#endif

#define CFG_NAME_LEN 64
#define CFG_VAL_STR_LEN  64
#define CFG_VALSET_STR_LEN   128

#ifndef SOFA_USER_SPACE
#define NUM_CONFIG_ITEM 17
#else
#define NUM_CONFIG_ITEM 26

/*
 * NOTE: The reason we need ssdlist_UserVal is we will fiter out os disk name from user setting.
 *       And we will save configuration file after everything is done.
 *       The ssd list will be changed to a list without os disk.
 *       But next time, when user daemon load configuration from file, the os disk might be changed
 *       to another disk and then trigger issues
 *       Example: 1st run: os disk at /dev/sdq, and disk list is a-y
 *                after save, list will be a,b,c,d,....,p,r,s,t,u,v,w,x,y
 *                2nd run: os disk at /dev/sdl, and disk list is a,b,c,...,p,r,s,t,u,v,w,x,y
 *                we will miss some disk.
 *       We will save user's setting and save it when save configuration.
 */
int8_t ssdlist_UserVal[CFG_VALSET_STR_LEN];
#endif

typedef enum {
    VAL_TYPE_BOOL = 0,
    VAL_TYPE_INT32,
    VAL_TYPE_INT64,
    VAL_TYPE_STR,
} config_val_type;

typedef struct config_item {
    struct list_head list;
    int8_t name[CFG_NAME_LEN];
    int8_t strVal[CFG_VAL_STR_LEN];
    int64_t intVal;
    int32_t len_name;
    int32_t len_strVal;
    int8_t val_set[CFG_VALSET_STR_LEN];
    int32_t len_valSet;
    config_val_type type_val;
    config_item_index index;
} config_item_t;

struct list_head sofa_cfglist;

#ifndef SOFA_USER_SPACE
struct rw_semaphore sofa_cfglock;
sofa_cfg_t sofacfg;
#else
pthread_mutex_t sofa_cfglock = PTHREAD_MUTEX_INITIALIZER;
int8_t *def_cfg_file = "/usr/sofa/config/sofa_config.xml";
int8_t *cfg_file;

#endif

int32_t cnt_cfgItems;

static void _add_config_item(struct list_head *head, config_item_index index,
                             int8_t * name, int32_t name_len, int8_t * val_str,
                             int32_t val_str_len, int64_t val, int8_t * valSet,
                             int32_t valSet_len, config_val_type type);
#endif /* SOFA_CONFIG_PRIVATE_H_ */
