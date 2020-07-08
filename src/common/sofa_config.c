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

#ifndef SOFA_USER_SPACE
#include <linux/types.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/bio.h>
#include "sofa_common.h"
#else
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <syslog.h>
#include <pthread.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include "userspace_list.h"
#endif
#include "sofa_config.h"
#include "sofa_config_private.h"
#include "sofa_mm.h"
#include "sofa_common_def.h"
#include "drv_userD_common.h"

#ifndef SOFA_USER_SPACE
#include "../sofa_feature_generic/io_common/storage_api_wrapper.h"
#endif

int8_t ssd_devPath[MAX_SUPPORT_SSD][MAX_DEV_PATH];
int32_t cnt_ssds;
int8_t *devlist[] = {
    "a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "l", "m",
    "n", "o", "p", "q", "r", "s", "t", "u", "v", "w", "x", "y", "z",

    "aa", "ab", "ac", "ad", "ae", "af", "ag", "ah", "ai", "aj", "ak", "al",
        "am",
    "an", "ao", "ap", "aq", "ar", "as", "at", "au", "av", "aw", "ax", "ay",
        "az",

    "ba", "bb", "bc", "bd", "be", "bf", "bg", "bh", "bi", "bj", "bk", "bl",
        "bm",
    "bn", "bo", "bp", "bq", "br", "bs", "bt", "bu", "bv", "bw", "bx", "by",
        "bz",
};

static void _show_sofa_cfgItem(config_item_t * cfgItem)
{
    int8_t strbuff[128];
    int32_t len;

    len = 0;
    len +=
        sprintf(strbuff + len, "[SOFA] CFG INFO: name: %16s ", cfgItem->name);

    switch (cfgItem->type_val) {
    case VAL_TYPE_BOOL:
        len +=
            sprintf(strbuff + len, "value: %lld (Type=bool)", cfgItem->intVal);
        break;
    case VAL_TYPE_INT32:
        len +=
            sprintf(strbuff + len, "value: %lld (Type=INT32)", cfgItem->intVal);
        break;
    case VAL_TYPE_INT64:
        len +=
            sprintf(strbuff + len, "value: %lld (Type=INT64)", cfgItem->intVal);
        break;
    case VAL_TYPE_STR:
        len += sprintf(strbuff + len, "value: %s (Type=str)", cfgItem->strVal);
        break;
    }

    if (cfgItem->len_valSet > 0) {
        len += sprintf(strbuff + len, "setting: %s", cfgItem->val_set);
    }
#ifndef SOFA_USER_SPACE
    sofa_printk(LOG_INFO, "%s\n", strbuff);
#else
    syslog(LOG_INFO, "%s\n", strbuff);
#endif
}

void show_sofa_config(void)
{
    config_item_t *cur, *next;

#ifndef SOFA_USER_SPACE
    sofa_printk(LOG_INFO, "CFG INFO Configuration as following:\n");
#else
    syslog(LOG_INFO, "[SOFA] CFG INFO Configuration as following:\n");
#endif

    list_for_each_entry_safe(cur, next, &sofa_cfglist, list) {
        _show_sofa_cfgItem(cur);
    }
}

#ifndef SOFA_USER_SPACE
static void _set_sofacfg_val(config_item_t * item)
{
    switch (item->index) {
    case config_reinstall:
        sofacfg.reinstall = item->intVal;
        storage_set_config(item->index, item->intVal, item->strVal,
                           item->val_set, item->len_valSet);
        break;
    case config_cn_ssds:
        sofacfg.num_ssds = item->intVal;
        sofacfg.ssds_setting = item->val_set;
        storage_set_config(item->index, item->intVal, item->strVal,
                           item->val_set, item->len_valSet);
        break;
    case config_ssds_hpeu:
        sofacfg.num_ssds_hpeu = item->intVal;
        storage_set_config(item->index, item->intVal, item->strVal,
                           item->val_set, item->len_valSet);
        break;
    case config_cn_pgroup:
        sofacfg.num_pgroups = item->intVal;
        storage_set_config(item->index, item->intVal, item->strVal,
                           item->val_set, item->len_valSet);
        break;
    case config_lfsm_ioT:
        sofacfg.lfsm_ioT = item->intVal;
        sofacfg.lfsm_ioT_setting = item->val_set;
        storage_set_config(item->index, item->intVal, item->strVal,
                           item->val_set, item->len_valSet);
        break;
    case config_lfsm_bhT:
        sofacfg.lfsm_bhT = item->intVal;
        sofacfg.lfsm_bhT_setting = item->val_set;
        storage_set_config(item->index, item->intVal, item->strVal,
                           item->val_set, item->len_valSet);
        break;
    case config_lfsm_gcrT:
        sofacfg.lfsm_gcreadT = item->intVal;
        storage_set_config(item->index, item->intVal, item->strVal,
                           item->val_set, item->len_valSet);
        break;
    case config_lfsm_gcwT:
        sofacfg.lfsm_gcwriteT = item->intVal;
        storage_set_config(item->index, item->intVal, item->strVal,
                           item->val_set, item->len_valSet);
        break;
    case config_lfsm_seuRsv:
        sofacfg.lfsm_seu_rsv = item->intVal;
        storage_set_config(item->index, item->intVal, item->strVal,
                           item->val_set, item->len_valSet);
        break;
    case config_max_vols:
        sofacfg.max_vols = item->intVal;
        break;
    case config_max_volcap:
        //NOTE: value in config_item_t MB, in sofacfg is bytes
        sofacfg.max_volcaps_bytes = (item->intVal << EXPON_MB_BYTES);
        break;
    case config_vdisk_name:
        strcpy(sofacfg.vdisk_name, item->strVal);
        break;
    case config_osdisk_name:
        storage_set_config(item->index, 0, item->strVal, item->val_set,
                           item->len_strVal);
        break;
    case config_gc_chk_intvl:
        sofacfg.lfsm_gc_chk_intvl = item->intVal;
        storage_set_config(item->index, item->intVal, item->strVal,
                           item->val_set, item->len_valSet);
        break;
    case config_gc_off_upper_ratio:
        sofacfg.lfsm_gc_off_upper_ratio = item->intVal;
        storage_set_config(item->index, item->intVal, item->strVal,
                           item->val_set, item->len_valSet);
        break;
    case config_gc_on_lower_ratio:
        sofacfg.lfsm_gc_on_lower_ratio = item->intVal;
        storage_set_config(item->index, item->intVal, item->strVal,
                           item->val_set, item->len_valSet);
        break;
    default:
        sofa_printk(LOG_INFO, "CFG INFO unknown cfg index %u\n", item->index);
        break;
    }
}
#else
static void _set_sofacfg_val(config_item_t * item)
{
    return;
}

static int32_t config_ioctl(int32_t chrdev_fd, int8_t * name, uint64_t val,
                            int8_t * valSet)
{
    int32_t ret;
    cfg_para_t cpara;

    memset(&cpara, 0, sizeof(cfg_para_t));

    memset(cpara.name, '\0', CFGPARA_NAME_LEN);
    cpara.name_len = strlen(name);
    strncpy(cpara.name, name, cpara.name_len);

    memset(cpara.value, '\0', CFGPARA_VAL_LEN);
    sprintf(cpara.value, "%llu", val);
    cpara.value_len = strlen(cpara.value);

    memset(cpara.valSet, '\0', CFGPAGA_VALSET_LEN);
    if (valSet != NULL) {
        cpara.valSet_len = strlen(valSet);
        strncpy(cpara.valSet, valSet, cpara.valSet_len);
    }

    ret = ioctl(chrdev_fd, IOCTL_CONFIG_DRV, (unsigned long)(&cpara));

    return ret;
}

static int32_t config_ioctl_str(int32_t chrdev_fd, int8_t * name, int8_t * val,
                                int8_t * valSet)
{
    int32_t ret;
    cfg_para_t cpara;

    memset(&cpara, 0, sizeof(cfg_para_t));

    memset(cpara.name, '\0', CFGPARA_NAME_LEN);
    cpara.name_len = strlen(name);
    strncpy(cpara.name, name, cpara.name_len);

    memset(cpara.value, '\0', CFGPARA_VAL_LEN);
    cpara.value_len = strlen(val);
    strncpy(cpara.value, val, cpara.value_len);

    memset(cpara.valSet, '\0', CFGPAGA_VALSET_LEN);
    if (valSet != NULL) {
        cpara.valSet_len = strlen(valSet);
        strncpy(cpara.valSet, valSet, cpara.valSet_len);
    }

    ret = ioctl(chrdev_fd, IOCTL_CONFIG_DRV, (unsigned long)(&cpara));

    return ret;
}

static int32_t _config_sofaDrv_item(int32_t chrdev_fd, config_item_t * item)
{
    int8_t *setVal_ptr;
    int32_t ret;

    ret = 0;

    if (item->len_valSet == 0) {
        setVal_ptr = NULL;
    } else {
        setVal_ptr = item->val_set;
    }

    if (VAL_TYPE_STR == item->type_val) {
        ret = config_ioctl_str(chrdev_fd, item->name, item->strVal, setVal_ptr);
    } else {
        ret = config_ioctl(chrdev_fd, item->name, item->intVal, setVal_ptr);
    }

    return ret;
}

static int32_t _is_drv_config_item(config_item_t * cfgItem)
{
    int32_t ret;

    ret = 1;
    switch (cfgItem->index) {
    case config_reinstall:
    case config_cn_ssds:
    case config_ssds_hpeu:
    case config_cn_pgroup:
    case config_lfsm_ioT:
    case config_lfsm_bhT:
    case config_lfsm_gcrT:
    case config_lfsm_gcwT:
    case config_lfsm_seuRsv:
    case config_max_vols:
    case config_max_volcap:
    case config_vdisk_name:
    case config_osdisk_name:
    case config_gc_chk_intvl:
    case config_gc_off_upper_ratio:
    case config_gc_on_lower_ratio:
        break;
    case config_srv_port:
    case config_lfsmdrvM_fn:
    case config_sofadrvM_fn:
    case config_iosched:
    case config_sofa_dir:
    case config_hba_irqsmp_enable:
    case config_hba_intrname:
        ret = 0;
        break;
    default:
        syslog(LOG_INFO, "[SOFA] USERD INFO unknwon cfg item index %d\n",
               cfgItem->index);
        ret = 0;
        break;
    }

    return ret;
}

int32_t config_sofa_drv(int32_t chrdev_fd)
{
    config_item_t *cur, *next;
    int32_t ret;

    ret = 0;

    pthread_mutex_lock(&sofa_cfglock);

    list_for_each_entry_safe(cur, next, &sofa_cfglist, list) {
        if (NULL == cur) {
            syslog(LOG_ERR, "[SOFA] USERD ERROR config list broken\n");
            continue;
        }

        if (!_is_drv_config_item(cur)) {
            continue;
        }

        if (_config_sofaDrv_item(chrdev_fd, cur)) {
            syslog(LOG_ERR, "[SOFA] USERD WARN config item fail index %d\n",
                   cur->index);
            _show_sofa_cfgItem(cur);
        }
    }

    pthread_mutex_unlock(&sofa_cfglock);

    return ret;
}

static void _remove_char_str(int8_t * str, int32_t slen, int8_t chDel)
{
    int8_t *newPos, *curPos;
    int32_t i, rlen;
    int8_t enable_cp;

    newPos = str;
    curPos = str;
    enable_cp = 0;
    rlen = slen;
    for (i = 0; i < slen; i++) {
        if (*curPos != chDel) {
            if (enable_cp) {
                *newPos = *curPos;
            }

            newPos++;
            rlen--;
            curPos++;
            continue;
        }

        if (!enable_cp) {
            enable_cp = 1;
        }
        curPos++;
    }

    if (rlen > 0) {
        memset(newPos, '\0', rlen);
    }
}

static int32_t _is_invalid_dev(int8_t * devPath)
{
    int32_t i, ret;

    ret = -1;
    for (i = 0; i < MAX_SUPPORT_SSD; i++) {
        if (strcmp(devlist[i], devPath) == 0) {
            ret = i;
            break;
        }
    }

    return ret;
}

static int32_t _set_ssd_devPath(int32_t startIndex, int8_t * devPath)
{
    int8_t leftDev[CFG_VALSET_STR_LEN];
    int8_t *token_p;
    int32_t i, id_s, id_e;

    token_p = strchr(devPath, '-');

    if (token_p == NULL) {
        strcpy(ssd_devPath[startIndex], devPath);
        return startIndex + 1;
    }

    memset(leftDev, '\0', CFG_VALSET_STR_LEN);
    strncpy(leftDev, devPath, token_p - devPath);
    id_s = _is_invalid_dev(leftDev);
    id_e = _is_invalid_dev(token_p + 1);
    if (id_s < 0 || id_e < 0) {
        syslog(LOG_ERR,
               "[SOFA] USERD ERROR set ssd path wrong not valid dev %s\n",
               devPath);
        return -1;
    }

    for (i = id_s; i <= id_e; i++) {
        strcpy(ssd_devPath[startIndex], devlist[i]);
        startIndex++;
    }

    return startIndex;
}

static int32_t _get_ssd_devPath(int8_t * setting)
{
    int8_t seg[CFG_VALSET_STR_LEN];
    int8_t *token_p, *cstr;
    int32_t cnt_devPath, setlen;

    setlen = strlen(setting);
    _remove_char_str(setting, setlen, ' ');
    setlen = strlen(setting);

    cnt_devPath = 0;
    cstr = setting;

    do {
        token_p = strchr(cstr, ',');
        memset(seg, '\0', CFG_VALSET_STR_LEN);
        if (NULL == token_p) {
            if (strlen(cstr) <= 0) {
                break;
            }
            strcpy(seg, cstr);
        } else {
            if (token_p == cstr) {
                cstr = token_p + 1;
                continue;
            }
            strncpy(seg, cstr, token_p - cstr);
            cstr = token_p + 1;
        }

        _remove_char_str(seg, strlen(seg), ',');
        cnt_devPath = _set_ssd_devPath(cnt_devPath, seg);
    } while (token_p != NULL);

    return cnt_devPath;
}

static void _config_disk_rq_aff(int32_t val)
{
    int8_t cmd[CFG_VALSET_STR_LEN];
    int32_t i, offset;

    for (i = 0; i < cnt_ssds; i++) {
        offset = 0;
        if (strncmp(ssd_devPath[i], "sd", 2) == 0) {
            offset = 2;
        }

        sprintf(cmd, "echo %d > /sys/block/sd%s/queue/rq_affinity", val,
                ssd_devPath[i] + offset);
        system(cmd);
        syslog(LOG_INFO, "[SOFA] USERD INFO set ssd rq_affinity: %s\n", cmd);
    }
}

static void _config_disk_scheduler(int8_t * sch)
{
    int8_t cmd[CFG_VALSET_STR_LEN];
    int32_t i, offset;

    for (i = 0; i < cnt_ssds; i++) {
        offset = 0;
        if (strncmp(ssd_devPath[i], "sd", 2) == 0) {
            offset = 2;
        }

        sprintf(cmd, "echo %s > /sys/block/sd%s/queue/scheduler", sch,
                ssd_devPath[i] + offset);
        system(cmd);
        syslog(LOG_INFO, "[SOFA] USERD INFO set ssd scheduler: %s\n", cmd);
    }
}

/*
 * NOTE: the procedure here seem redundant (pre-config and post-config),
 * but we found it works from experience.
 */
void config_sofa_UsrD_pre(void)
{
    int8_t ssd_path[CFG_VALSET_STR_LEN];
    config_item_t *cur, *next;
    int32_t hasPath;

    syslog(LOG_INFO, "[SOFA] USERD INFO pre-config before start sofa\n");

    memset(ssd_path, '\0', CFG_VALSET_STR_LEN);
    hasPath = 0;

    pthread_mutex_lock(&sofa_cfglock);

    list_for_each_entry_safe(cur, next, &sofa_cfglist, list) {
        if (NULL == cur) {
            syslog(LOG_ERR, "[SOFA] USERD ERROR config list broken\n");
            continue;
        }

        if (cur->index == config_cn_ssds) {
            strcpy(ssd_path, cur->val_set);
            hasPath = 1;
            break;
        }
    }

    pthread_mutex_unlock(&sofa_cfglock);

    if (hasPath) {
        /*
         * NOTE: the procedure here seem redundant (set rq_aff, set scheduler, set rq_aff),
         * but we found it works from experience.
         */
        memset(ssd_devPath, '\0',
               sizeof(int8_t) * MAX_SUPPORT_SSD * MAX_DEV_PATH);
        cnt_ssds = _get_ssd_devPath(ssd_path);
        //NOTE: Magic code here
        _config_disk_rq_aff(DEFVAL_RQ_AFFINITY);
        _config_disk_rq_aff(DEFVAL_RQ_AFFINITY);
    }
}

static void _config_lfsm_attrib(int8_t * sch, int32_t rq_aff)
{
    int8_t cmd[CFG_VALSET_STR_LEN];

    //NOTE: Magic code here
    sprintf(cmd, "echo 1 > /sys/block/lfsm/queue/rq_affinity");
    system(cmd);
    sprintf(cmd, "echo none > /sys/block/lfsm/queue/scheduler");
    system(cmd);

    sleep(5);

    sprintf(cmd, "echo %d > /sys/block/lfsm/queue/rq_affinity", rq_aff);
    system(cmd);
    syslog(LOG_INFO, "[SOFA] USERD INFO set lfsm rq_affinity: %s\n", cmd);

    sprintf(cmd, "echo %s > /sys/block/lfsm/queue/scheduler", sch);
    system(cmd);
    syslog(LOG_INFO, "[SOFA] USERD INFO set lfsm scheduler: %s\n", cmd);
}

void config_sofa_UsrD_post(void)
{
    int8_t *sched_drv, *val_sched;

    syslog(LOG_INFO, "[SOFA] USERD INFO post-config after start sofa\n");

    //NOTE: This is a magic code here when test with sofa + iscsi
    _config_disk_rq_aff(1);
    sleep(5);
    _config_disk_rq_aff(DEFVAL_RQ_AFFINITY);

    syslog(LOG_INFO, "[SOFA] USERD INFO lfsm set /dev/lfsm attribute\n");
    _config_lfsm_attrib(val_sched, DEFVAL_RQ_AFFINITY);
}

void config_sofa_UsrD_exit(void)
{
    _config_disk_rq_aff(1);
}
#endif

void set_config_item(int8_t * name, int8_t * val, int8_t * valSet)
{
    config_item_t *cur, *next;

#ifndef SOFA_USER_SPACE
    down_write(&sofa_cfglock);
#else
    pthread_mutex_lock(&sofa_cfglock);
#endif

    list_for_each_entry_safe(cur, next, &sofa_cfglist, list) {
        if (strcmp(name, cur->name) != 0) {
            continue;
        }

        if (cur->type_val == VAL_TYPE_STR) {
            memset(cur->strVal, 0, sizeof(int8_t) * CFG_VAL_STR_LEN);
            cur->len_strVal = strlen(val);
            memcpy(cur->strVal, val, sizeof(int8_t) * cur->len_strVal);
        } else {
            sscanf(val, "%lld", &(cur->intVal));
        }

        memset(cur->val_set, '\0', CFG_VALSET_STR_LEN);
        if (NULL == valSet) {
            cur->len_valSet = 0;
        } else {
            cur->len_valSet = strlen(valSet);
            memcpy(cur->val_set, valSet, sizeof(int8_t) * cur->len_valSet);
        }

        _set_sofacfg_val(cur);
        break;
    }

#ifndef SOFA_USER_SPACE
    up_write(&sofa_cfglock);
#else
    pthread_mutex_unlock(&sofa_cfglock);
#endif
}

void set_config_item_index(config_item_index index, int8_t * newStrVal,
                           int64_t newIntVal)
{
    config_item_t *cur, *next;

#ifndef SOFA_USER_SPACE
    down_write(&sofa_cfglock);
#else
    pthread_mutex_lock(&sofa_cfglock);
#endif

    list_for_each_entry_safe(cur, next, &sofa_cfglist, list) {
        if (cur->index != index) {
            continue;
        }

        if (cur->type_val == VAL_TYPE_STR) {
            cur->len_strVal = strlen(newStrVal);
            memset(cur->strVal, '\0', sizeof(int8_t) * CFG_VAL_STR_LEN);
            memcpy(cur->strVal, newStrVal, sizeof(int8_t) * cur->len_strVal);
        } else {
            cur->intVal = newIntVal;
        }

        _set_sofacfg_val(cur);
        break;
    }

#ifndef SOFA_USER_SPACE
    up_write(&sofa_cfglock);
#else
    pthread_mutex_unlock(&sofa_cfglock);
#endif
}

int32_t get_config_idx_numItems(config_item_index index)
{
    config_item_t *cur, *next;
    int32_t numItems;

    numItems = 0;
#ifndef SOFA_USER_SPACE
    down_write(&sofa_cfglock);
#else
    pthread_mutex_lock(&sofa_cfglock);
#endif

    list_for_each_entry_safe(cur, next, &sofa_cfglist, list) {
        if (cur->index != index) {
            continue;
        }

        numItems++;
    }

#ifndef SOFA_USER_SPACE
    up_write(&sofa_cfglock);
#else
    pthread_mutex_unlock(&sofa_cfglock);
#endif

    return numItems;
}

int32_t get_config_name_val_allItems(config_item_index index, int8_t ** name,
                                     int64_t ** intVal, int8_t ** strVal,
                                     int8_t ** strValSet)
{
    config_item_t *cur, *next;
    int32_t cnt_idx;

    cnt_idx = 0;
#ifndef SOFA_USER_SPACE
    down_write(&sofa_cfglock);
#else
    pthread_mutex_lock(&sofa_cfglock);
#endif

    list_for_each_entry_safe(cur, next, &sofa_cfglist, list) {
        if (cur->index != index) {
            continue;
        }

        name[cnt_idx] = cur->name;
        if (cur->type_val == VAL_TYPE_STR) {
            strVal[cnt_idx] = cur->strVal;
        } else {
            intVal[cnt_idx] = &(cur->intVal);
        }

        if (strValSet != NULL) {
            strValSet[cnt_idx] = cur->val_set;
        }

        cnt_idx++;
    }

#ifndef SOFA_USER_SPACE
    up_write(&sofa_cfglock);
#else
    pthread_mutex_unlock(&sofa_cfglock);
#endif

    return cnt_idx;
}

void get_config_name_val(config_item_index index, int8_t ** name,
                         int64_t * intVal, int8_t ** strVal,
                         int8_t ** strValSet)
{
    config_item_t *cur, *next;

#ifndef SOFA_USER_SPACE
    down_write(&sofa_cfglock);
#else
    pthread_mutex_lock(&sofa_cfglock);
#endif

    list_for_each_entry_safe(cur, next, &sofa_cfglist, list) {
        if (cur->index != index) {
            continue;
        }

        *name = cur->name;
        if (cur->type_val == VAL_TYPE_STR) {
            *strVal = cur->strVal;
        } else {
            *intVal = cur->intVal;
        }

        if (strValSet != NULL) {
            *strValSet = cur->val_set;
        }

        break;
    }

#ifndef SOFA_USER_SPACE
    up_write(&sofa_cfglock);
#else
    pthread_mutex_unlock(&sofa_cfglock);
#endif
}

#ifdef SOFA_USER_SPACE
void save_sofa_config(void)
{
    config_item_t *cur, *next;
    FILE *fp;

    if (cfg_file == NULL) {
        cfg_file = def_cfg_file;
    }

    syslog(LOG_INFO, "[SOFA] CFG INFO save config to %s\n", cfg_file);

    fp = fopen(cfg_file, "w");
    if (NULL == fp) {
        syslog(LOG_ERR, "[SOFA] CFG ERROR fail open file %s\n", cfg_file);
        return;
    }

    fprintf(fp, "<?xml version=\"1.0\"?>\n");
    fprintf(fp,
            "<?xml-stylesheet type=\"text/xsl\" href=\"configuration.xsl\"?>\n");
    fprintf(fp, "<configuration>\n");

    pthread_mutex_lock(&sofa_cfglock);
    list_for_each_entry_safe(cur, next, &sofa_cfglist, list) {
        if (strcmp(cur->name, STR_LFSM_DRVFN) == 0 || strcmp(cur->name, STR_SOFA_DRV_FN) == 0 || strcmp(cur->name, STR_SOFA_DIR) == 0 || strcmp(cur->name, STR_OSDISK_NAME) == 0 || strcmp(cur->name, STR_SRV_PORT) == 0) { // Ignore write to sofa_config.xml
            continue;
        }
        fprintf(fp, "    <property>\n");
        fprintf(fp, "        <name>%s</name>\n", cur->name);
        fprintf(fp, "        <value>");
        if (VAL_TYPE_STR != cur->type_val) {
            fprintf(fp, "%ld", cur->intVal);
        } else {
            fprintf(fp, "%s", cur->strVal);
        }
        fprintf(fp, "</value>\n");

        if (cur->len_valSet > 0) {
            if (cur->index == config_cn_ssds) {
                fprintf(fp, "        <setting>%s</setting>\n", ssdlist_UserVal);
            } else {
                fprintf(fp, "        <setting>%s</setting>\n", cur->val_set);
            }
        }

        fprintf(fp, "    </property>\n");
    }
    pthread_mutex_unlock(&sofa_cfglock);

    fprintf(fp, "</configuration>\n");
    fclose(fp);
}

static void _add_hba_cfg_item(int8_t * val_str, int32_t val_len,
                              int8_t * valSet_str, int32_t valSet_len)
{
    int8_t tmp_strVal[CFG_VAL_STR_LEN];
    int8_t tmp_strSet[CFG_VALSET_STR_LEN];
    int32_t len_strVal, len_strSet;

    memset(tmp_strVal, '\0', CFG_VAL_STR_LEN);
    strncpy(tmp_strVal, val_str, val_len);
    len_strVal = val_len;

    memset(tmp_strSet, '\0', CFG_VALSET_STR_LEN);
    strncpy(tmp_strSet, valSet_str, valSet_len);
    len_strSet = valSet_len;

    _add_config_item(&sofa_cfglist, config_hba_intrname,
                     STR_HBA_INTR_NAME, strlen(STR_HBA_INTR_NAME),
                     tmp_strVal, len_strVal, 0, tmp_strSet, len_strSet,
                     VAL_TYPE_STR);
}

static int32_t _set_cfg_val_str(int8_t * name_str, int32_t name_len,
                                int8_t * val_str, int32_t val_len,
                                int8_t * valSet_str, int32_t valSet_len)
{
    config_item_t *cur, *next;
    int32_t ret;

    ret = -1;

    if (strncmp(name_str, STR_HBA_INTR_NAME, name_len) == 0) {
        _add_hba_cfg_item(val_str, val_len, valSet_str, valSet_len);
        return 0;
    }

    pthread_mutex_lock(&sofa_cfglock);

    list_for_each_entry_safe(cur, next, &sofa_cfglist, list) {
        if (strncmp(name_str, cur->name, name_len) != 0) {
            continue;
        }

        if (VAL_TYPE_STR != cur->type_val) {
            sscanf(val_str, "%ld", &cur->intVal);
        } else {
            memset(cur->strVal, '\0', CFG_VAL_STR_LEN);
            strncpy(cur->strVal, val_str, val_len);
            cur->len_strVal = val_len;
        }

        if (valSet_len > 0) {
            memset(cur->val_set, '\0', CFG_VALSET_STR_LEN);
            strncpy(cur->val_set, valSet_str, valSet_len);
            cur->len_valSet = valSet_len;
        }

        _set_sofacfg_val(cur);
        ret = 0;
        break;
    }

    pthread_mutex_unlock(&sofa_cfglock);

    return ret;
}

static void _add_cfg_item_str(const int8_t * elem,
                              int32_t(*func) (int8_t *, int32_t, int8_t *,
                                              int32_t, int8_t *, int32_t))
{
    int8_t el[64];
    int32_t i, length;
    int8_t *el_s_name = "<name>", *el_e_name = "</name>";
    int8_t *el_s_value = "<value>", *el_e_value = "</value>";
    int8_t *el_s_set = "<setting>", *el_e_set = "</setting>";
    int8_t *name_el_start, *name_el_end, *value_el_start, *value_el_end;
    int8_t *setCont_s, *setCont_e;

    name_el_start = NULL;
    name_el_end = NULL;
    value_el_start = NULL;
    value_el_end = NULL;
    setCont_s = NULL;
    setCont_e = NULL;

    length = 0;

    name_el_start = strstr(elem, el_s_name);
    name_el_end = strstr(elem, el_e_name);
    value_el_start = strstr(elem, el_s_value);
    value_el_end = strstr(elem, el_e_value);
    setCont_s = strstr(elem, el_s_set);
    setCont_e = strstr(elem, el_e_set);

    if (name_el_start != NULL && name_el_end != NULL && value_el_start != NULL
        && value_el_end != NULL) {
        name_el_start = name_el_start + strlen(el_s_name);
        name_el_end = name_el_end - 1;
        value_el_start = value_el_start + strlen(el_s_value);
        value_el_end = value_el_end - 1;
        strncpy(el, value_el_start, value_el_end - value_el_start + 1);
        el[value_el_end - value_el_start + 1] = '\0';

        if (NULL == setCont_s || NULL == setCont_e) {
            func(name_el_start, name_el_end - name_el_start + 1, el, strlen(el),
                 NULL, 0);
        } else {
            setCont_s = setCont_s + strlen(el_s_set);
            setCont_e = setCont_e - 1;
            func(name_el_start, name_el_end - name_el_start + 1,
                 el, strlen(el), setCont_s, setCont_e - setCont_s + 1);
        }
    }
}

static int32_t _update_os_disk(int8_t * osdisk_ret)
{
    int8_t cmd[CFG_VALSET_STR_LEN];
    int8_t osdisk_name[DRV_FN_MAX_LEN];
    int8_t *nm_sofa_dir, *val_sofa_dir;
    FILE *fp;
    int32_t ret;

    memset(osdisk_name, '\0', DRV_FN_MAX_LEN);

    get_config_name_val(config_sofa_dir, &nm_sofa_dir, NULL, &val_sofa_dir,
                        NULL);

    sprintf(cmd, "%s/bin/tool/disk_tool/get_os_disk.sh", val_sofa_dir);
    fp = popen(cmd, "r");

    if (fgets(osdisk_name, CFG_VALSET_STR_LEN, fp) != NULL) {
        if (osdisk_name[0] != 0) {
            _remove_char_str(osdisk_name, strlen(osdisk_name), '\n');
            set_config_item(STR_OSDISK_NAME, osdisk_name, NULL);
            strcpy(osdisk_ret, osdisk_name);
            syslog(LOG_INFO, "[SOFA] USERD INFO get os disk: %s\n",
                   osdisk_name);
            ret = 0;
        } else {
            //strcpy(osdisk_ret, DEFVAL_OSDISK_NAME);
            syslog(LOG_ERR,
                   "[SOFA] ERROR CFG INFO fail to get os disk with cmd: %s\n",
                   cmd);
            ret = 1;
        }
    }
    pclose(fp);

    return ret;
}

static void _set_config_devpath(int8_t cnt_ssds, int8_t * devlist)
{
    config_item_t *cur, *next;

    pthread_mutex_lock(&sofa_cfglock);

    list_for_each_entry_safe(cur, next, &sofa_cfglist, list) {
        if (cur->index != config_cn_ssds) {
            continue;
        }

        cur->intVal = cnt_ssds;
        strcpy(cur->val_set, devlist);
        break;
    }
    pthread_mutex_unlock(&sofa_cfglock);

}

static void _fitler_osdisk_gut(int32_t cnt_ssds, int32_t osdiskIndex)
{
    int8_t newdevPath[CFG_VALSET_STR_LEN], *loc;
    int32_t i, data_ssd, hasfilter, offset;

    hasfilter = 0;
    data_ssd = 0;
    memset(newdevPath, '\0', CFG_VALSET_STR_LEN);
    loc = newdevPath;
    for (i = 0; i < cnt_ssds; i++) {
        offset = 0;
        if (strncmp(ssd_devPath[i], "sd", 2) == 0) {
            offset = 2;
        }
        if (strcmp(ssd_devPath[i] + offset, devlist[osdiskIndex]) == 0) {
            hasfilter = 1;
            continue;
        }

        if (data_ssd > 0) {
            *loc = ',';
            loc++;
        }

        strcpy(loc, ssd_devPath[i]);
        loc += strlen(ssd_devPath[i]);
        data_ssd++;
    }

    if (hasfilter) {
        _set_config_devpath(data_ssd, newdevPath);
    }
}

static int32_t _filter_osdisk_cnssds(void)
{
    int8_t osdisk_name[DRV_FN_MAX_LEN], osdisk_char[DRV_FN_MAX_LEN];
    int8_t *nm_ssdlist, *valset_ssdlist;
    int64_t val_ssdlist;
    int32_t diskIndex, ret;

    ret = 0;
    memset(osdisk_name, '\0', DRV_FN_MAX_LEN);
    memset(osdisk_char, '\0', DRV_FN_MAX_LEN);
    if ((ret = _update_os_disk(osdisk_name))) {
        syslog(LOG_ERR, "[SOFA] ERROR CFG INFO fail to get os disk\n");
        return ret;
    }

    get_config_name_val(config_cn_ssds, &nm_ssdlist, &val_ssdlist, NULL,
                        &valset_ssdlist);
    strcpy(ssdlist_UserVal, valset_ssdlist);

    _remove_char_str(valset_ssdlist, strlen(valset_ssdlist), ' ');
    strncpy(osdisk_char, osdisk_name + 2, strlen(osdisk_name) - 2); //NOTE: remove sd from sdX
    diskIndex = _is_invalid_dev(osdisk_char);

    if (diskIndex >= 0) {
        memset(ssd_devPath, '\0',
               sizeof(int8_t) * MAX_SUPPORT_SSD * MAX_DEV_PATH);
        cnt_ssds = _get_ssd_devPath(valset_ssdlist);
        _fitler_osdisk_gut(cnt_ssds, diskIndex);
    }

    return ret;
}

#define ch_arr_size 1024
int32_t load_sofa_config(int8_t * config_file)
{
    int8_t ch_array[ch_arr_size];
    int8_t *el_s_property = "<property>", *el_e_property = "</property>";
    FILE *fp;
    int32_t ret, index, state;
    int8_t ch;

    memset(ssdlist_UserVal, '\0', CFG_VALSET_STR_LEN);
    ret = -1;
    index = -1;
    state = 0;
    if (config_file != NULL) {
        cfg_file = config_file;
    } else {
        cfg_file = def_cfg_file;
    }

    if (cfg_file == NULL) {
        syslog(LOG_INFO, "[SOFA] CFG INFO start with no config\n");
        return 0;
    }

    syslog(LOG_INFO, "[SOFA] CFG INFO load config from %s\n", cfg_file);

    fp = fopen(cfg_file, "r");
    if (NULL == fp) {
        syslog(LOG_ERR, "[SOFA] CFG ERROR fail open file %s\n", cfg_file);
        return ret;
    }

    memset(ch_array, 0, sizeof(int8_t) * ch_arr_size);

    while ((ch = fgetc(fp)) != EOF) {
        if (0 == state && '<' == ch) {
            state = 1;
        }

        if (state > 0) {
            index++;

            if (index >= ch_arr_size) {
                syslog(LOG_ERR,
                       "[SOFA] CFG ERROR fail handle cfg file long str "
                       "(val, max) (%u %u)\n", index, ch_arr_size);
                ret = -1;
                goto LOAD_CFG_EXIT;
            }
            ch_array[index] = ch;
        }

        if ('>' == ch) {
            ch_array[index + 1] = '\0';

            if (1 == state && (strstr(ch_array, el_s_property) != NULL)) {
                state = 2;
            } else if (2 == state && (strstr(ch_array, el_e_property) != NULL)) {
                //Extract value
                _add_cfg_item_str(ch_array, _set_cfg_val_str);
                state = 0;
                index = -1;
                memset(ch_array, 0, sizeof(int8_t) * ch_arr_size);
            } else if (1 == state) {
                index = -1;
                state = 0;
                memset(ch_array, 0, sizeof(int8_t) * ch_arr_size);
            }
        }
    }

    ret = _filter_osdisk_cnssds();

  LOAD_CFG_EXIT:
    fclose(fp);
    return ret;
}
#endif

static void _add_config_item(struct list_head *head, config_item_index index,
                             int8_t * name, int32_t name_len, int8_t * val_str,
                             int32_t val_str_len, int64_t val, int8_t * valSet,
                             int32_t valSet_len, config_val_type type)
{
    config_item_t *item;

#ifndef SOFA_USER_SPACE
    item = (config_item_t *) sofa_kmalloc(sizeof(config_item_t), GFP_KERNEL);
    down_write(&sofa_cfglock);
#else
    item = (config_item_t *) sofa_malloc(sizeof(config_item_t));
    pthread_mutex_lock(&sofa_cfglock);
#endif

    INIT_LIST_HEAD(&item->list);
    memset(item->name, '\0', CFG_NAME_LEN);
    strncpy(item->name, name, name_len);
    item->len_name = name_len;

    memset(item->strVal, '\0', CFG_VAL_STR_LEN);
    if (!val_str_len) {
        item->len_strVal = 0;
    } else {
        strncpy(item->strVal, val_str, val_str_len);
        item->len_strVal = val_str_len;
    }

    memset(item->val_set, '\0', CFG_VALSET_STR_LEN);
    if (valSet_len > 0) {
        item->len_valSet = valSet_len;
        strncpy(item->val_set, valSet, valSet_len);
    }

    item->intVal = val;
    item->type_val = type;
    item->index = index;

    list_add_tail(&item->list, head);

    _set_sofacfg_val(item);

#ifndef SOFA_USER_SPACE
    up_write(&sofa_cfglock);
#else
    pthread_mutex_unlock(&sofa_cfglock);
#endif
}

void init_sofa_config(void)
{
#ifndef SOFA_USER_SPACE
    init_rwsem(&sofa_cfglock);
#else
    pthread_mutex_init(&sofa_cfglock, NULL);
#endif

    cnt_cfgItems = NUM_CONFIG_ITEM;
    INIT_LIST_HEAD(&sofa_cfglist);

    _add_config_item(&sofa_cfglist, config_reinstall, STR_REINSTALL,
                     strlen(STR_REINSTALL), "\0", 0, DEFVAL_REINSTALL, NULL, 0,
                     VAL_TYPE_INT32);
    _add_config_item(&sofa_cfglist, config_cn_ssds, STR_CN_SSDs,
                     strlen(STR_CN_SSDs), "\0", 0, DEFVAL_CN_SSDs,
                     DEFVAL_CN_SSD_LIST, strlen(DEFVAL_CN_SSD_LIST),
                     VAL_TYPE_INT32);
    _add_config_item(&sofa_cfglist, config_ssds_hpeu, STR_CN_SSDs_HPEU,
                     strlen(STR_CN_SSDs_HPEU), "\0", 0, DEFVAL_CN_SSDs_HPEU,
                     NULL, 0, VAL_TYPE_INT32);
    _add_config_item(&sofa_cfglist, config_cn_pgroup, STR_CN_PGROUP,
                     strlen(STR_CN_PGROUP), "\0", 0, DEFVAL_CN_PGROUP, NULL, 0,
                     VAL_TYPE_INT32);

    _add_config_item(&sofa_cfglist, config_lfsm_seuRsv, STR_LFSM_SEU_RESERVED,
                     strlen(STR_LFSM_SEU_RESERVED), "\0", 0,
                     DEFVAL_SEU_RESERVED, NULL, 0, VAL_TYPE_INT32);

    _add_config_item(&sofa_cfglist, config_gc_chk_intvl, STR_GC_CHK_INTVL,
                     strlen(STR_GC_CHK_INTVL), "\0", 0,
                     DEFVAL_GC_CHK_INTVL, NULL, 0, VAL_TYPE_INT64);
    _add_config_item(&sofa_cfglist, config_gc_on_lower_ratio,
                     STR_GC_ON_RATIO, strlen(STR_GC_ON_RATIO), "\0", 0,
                     DEFVAL_GC_ON_RATIO, NULL, 0, VAL_TYPE_INT64);
    _add_config_item(&sofa_cfglist, config_gc_off_upper_ratio, STR_GC_OFF_RATIO,
                     strlen(STR_GC_OFF_RATIO), "\0", 0,
                     DEFVAL_GC_OFF_RATIO, NULL, 0, VAL_TYPE_INT64);

    _add_config_item(&sofa_cfglist, config_lfsm_ioT, STR_LFSM_IOT,
                     strlen(STR_LFSM_IOT), "\0", 0,
                     DEFVAL_LFSM_IOT, DEFVAL_LFSM_IOT_LIST,
                     strlen(DEFVAL_LFSM_IOT_LIST), VAL_TYPE_INT32);
    _add_config_item(&sofa_cfglist, config_lfsm_bhT, STR_LFSM_BHT,
                     strlen(STR_LFSM_BHT), "\0", 0, DEFVAL_LFSM_BHT,
                     DEFVAL_LFSM_BHT_LIST, strlen(DEFVAL_LFSM_BHT_LIST),
                     VAL_TYPE_INT32);

    _add_config_item(&sofa_cfglist, config_osdisk_name, STR_OSDISK_NAME,
                     strlen(STR_OSDISK_NAME), DEFVAL_OSDISK_NAME,
                     strlen(DEFVAL_OSDISK_NAME), 0, NULL, 0, VAL_TYPE_STR);

#ifdef SOFA_USER_SPACE
    _add_config_item(&sofa_cfglist, config_srv_port, STR_SRV_PORT,
                     strlen(STR_SRV_PORT), "\0", 0, DEFVAL_SRV_PORT, NULL, 0,
                     VAL_TYPE_INT32);
    _add_config_item(&sofa_cfglist, config_lfsmdrvM_fn, STR_LFSM_DRVFN,
                     strlen(STR_LFSM_DRVFN), DEFVAL_LFSM_DRVFN,
                     strlen(DEFVAL_LFSM_DRVFN), 0, NULL, 0, VAL_TYPE_STR);
    _add_config_item(&sofa_cfglist, config_sofadrvM_fn, STR_SOFA_DRV_FN,
                     strlen(STR_SOFA_DRV_FN), DEFVAL_SOFA_DRV_FN,
                     strlen(DEFVAL_SOFA_DRV_FN), 0, NULL, 0, VAL_TYPE_STR);
    _add_config_item(&sofa_cfglist, config_sofa_dir, STR_SOFA_DIR,
                     strlen(STR_SOFA_DIR), DEFVAL_SOFA_DIR,
                     strlen(DEFVAL_SOFA_DIR), 0, NULL, 0, VAL_TYPE_STR);
#endif

}

static void _rel_sofa_cfgItem(config_item_t * cfgItem)
{
    list_del(&cfgItem->list);

#ifndef SOFA_USER_SPACE
    sofa_kfree(cfgItem);
#else
    sofa_free(cfgItem);
#endif
}

void rel_sofa_config(void)
{
    config_item_t *cur, *next;

#ifndef SOFA_USER_SPACE
    down_write(&sofa_cfglock);
#else
    pthread_mutex_lock(&sofa_cfglock);
#endif

    list_for_each_entry_safe(cur, next, &sofa_cfglist, list) {
        _rel_sofa_cfgItem(cur);
    }

#ifndef SOFA_USER_SPACE
    up_write(&sofa_cfglock);
#else
    pthread_mutex_unlock(&sofa_cfglock);
#endif
}
