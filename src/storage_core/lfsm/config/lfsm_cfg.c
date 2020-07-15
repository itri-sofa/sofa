/*
 * SPDX-License-Identifier: GPL-2.0-or-later OR Apache-2.0
 * Copyright (C) 2015-2020 Industrial Technology Research Institute
 *
 *
 *
 *  
 */
#include <linux/version.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/errno.h>
#include <linux/err.h>
#include "../common/common.h"
#include "../lfsm_export_inc.h"
#include "lfsm_cfg.h"
#include "lfsm_cfg_private.h"

/*
 * Caller guarantee len_V > 0 and val not NULL
 */
static int32_t _set_cfg_resetData(uint32_t val)
{
    if (val != 0) {
        lfsmCfg.resetData = true;
    } else {
        lfsmCfg.resetData = false;
    }

    return 0;
}

/*
 * unit_test: test_lfsm_cfg.c
 */
static void _remove_char_str(int8_t * str, int32_t slen, int8_t chDel)
{
    int8_t *newPos, *curPos;
    int32_t i, rlen;
    bool enable_cp;

    newPos = str;
    curPos = str;
    enable_cp = false;
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
            enable_cp = true;
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
    int8_t leftDev[MAX_SETTING_LEN];
    int8_t *token_p;
    int32_t i, id_s, id_e;

    token_p = strchr(devPath, '-');

    LOGINFO("_set_ssd_devPath() index=%d, ssd_path=%s\n", startIndex, devPath);
    if (token_p == NULL) {
        strcpy(lfsmCfg.ssd_path[startIndex], devPath);
        return startIndex + 1;
    }

    memset(leftDev, '\0', MAX_SETTING_LEN);
    strncpy(leftDev, devPath, token_p - devPath);
    id_s = _is_invalid_dev(leftDev);
    id_e = _is_invalid_dev(token_p + 1);
    if (id_s < 0 || id_e < 0) {
        LOGERR("set ssd path wrong not valid dev %s\n", devPath);
        return -1;
    }

    for (i = id_s; i <= id_e; i++) {
        strcpy(lfsmCfg.ssd_path[startIndex], devlist[i]);
        startIndex++;
    }

    return startIndex;
}

static int32_t _set_cfg_ssds(uint32_t val, int8_t * setting, uint32_t setlen)
{
    int8_t seg[MAX_SETTING_LEN];
    int8_t *token_p, *cstr;
    int32_t ret, cnt_devPath;

    if (val > MAX_SUPPORT_SSD) {
        LOGERR("ssd %d > max support ssds %d\n", lfsmCfg.cnt_ssds,
               MAX_SUPPORT_SSD);
        lfsmCfg.cnt_ssds = 0;
        return -EINVAL;
    }
    lfsmCfg.cnt_ssds = val;

    if (IS_ERR_OR_NULL(setting) || setlen == 0) {
        lfsmCfg.cnt_ssds = 0;
        LOGERR("no ssd path setting\n");
        return -EINVAL;
    }

    _remove_char_str(setting, setlen, ' ');
    setlen = strlen(setting);

    cnt_devPath = 0;
    cstr = setting;

    do {
        token_p = strchr(cstr, ',');
        memset(seg, '\0', MAX_SETTING_LEN);
        if (IS_ERR_OR_NULL(token_p)) {
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

    ret = 0;
    if (cnt_devPath != lfsmCfg.cnt_ssds) {
        ret = -EINVAL;
        LOGERR("wrong ssds setting: num ssds %d ssd from path %d\n",
               lfsmCfg.cnt_ssds, cnt_devPath);
    }

    return ret;
}

static int32_t _set_cfg_ssdhpeu(uint32_t val)
{
    int32_t ret;

    if (val > lfsmCfg.cnt_ssds) {
        ret = -EINVAL;
        LOGERR("wrong ssd_hpeu setting %d > total ssds %d\n", val,
               lfsmCfg.cnt_ssds);
    } else {
        lfsmCfg.ssds_hpeu = val;
        ret = 0;
    }

    return ret;
}

static int32_t _set_cfg_pgroup(uint32_t val)
{
    int32_t ret;

    if (val * lfsmCfg.ssds_hpeu > lfsmCfg.cnt_ssds) {
        LOGERR("wrong pgroup setting %d*%d > total ssds %d\n",
               lfsmCfg.ssds_hpeu, val, lfsmCfg.cnt_ssds);
        ret = -EINVAL;
    } else {
        lfsmCfg.cnt_pgroup = val;
        ret = 0;
    }

    return ret;
}

static int32_t _set_TVcore_map(uint32_t * t_vcore_map, int32_t startIndex,
                               const int8_t * vcore_set)
{
    int8_t leftVcore[MAX_SETTING_LEN];
    int8_t *token_p;
    uint32_t vcore;
    int32_t i, id_s, id_e;

    if (strcmp(vcore_set, "-1") == 0) {
        t_vcore_map[startIndex] = -1;
        return startIndex + 1;
    }

    token_p = strchr(vcore_set, '-');
    if (token_p == NULL) {
        sscanf(vcore_set, "%u", &vcore);
        t_vcore_map[startIndex] = vcore;
        return startIndex + 1;
    }

    memset(leftVcore, '\0', MAX_SETTING_LEN);
    strncpy(leftVcore, vcore_set, token_p - vcore_set);

    if (token_p == vcore_set || *(token_p + 1) == '\0') {
        LOGERR("set thread vcore map wrong not valid setting %s\n", vcore_set);
        return -1;
    }

    sscanf(leftVcore, "%u", &id_s);
    sscanf(token_p + 1, "%u", &id_e);

    for (i = id_s; i <= id_e; i++) {
        t_vcore_map[startIndex] = i;
        startIndex++;
    }

    return startIndex;
}

static int32_t _set_cfg_threads(uint32_t * cnt_threads, int32_t * t_vcore_map,
                                uint32_t val, int8_t * setting, uint32_t len_S)
{
    int8_t seg[MAX_SETTING_LEN];
    int8_t *cstr, *token_p;
    int32_t ret, i, cnt_ioT;

    ret = 0;

    if (val == 0) {
        LOGERR("wrong total thread setting %d\n", val);
        return -EINVAL;
    }
    *cnt_threads = val;

    if (IS_ERR_OR_NULL(setting) || len_S <= 0) {
        LOGINFO("No io_thread - vcore setting no thread has dedicated vcore\n");
        return ret;
    }

    if (strcmp(setting, "-1") == 0) {
        for (i = 0; i < MAX_SUPPORT_Thread; i++) {
            t_vcore_map[i] = -1;
        }

        return ret;
    }

    _remove_char_str(setting, len_S, ' ');
    len_S = strlen(setting);

    cnt_ioT = 0;
    cstr = setting;

    do {
        token_p = strchr(cstr, ',');
        memset(seg, '\0', MAX_SETTING_LEN);
        if (IS_ERR_OR_NULL(token_p)) {
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
        cnt_ioT = _set_TVcore_map(t_vcore_map, cnt_ioT, seg);
    } while (token_p != NULL);

    ret = 0;
    if (cnt_ioT != *cnt_threads) {
        ret = -EINVAL;
        LOGERR("wrong thread setting: num thread %d thread from map %d\n",
               *cnt_threads, cnt_ioT);
    }

    return ret;
}

int32_t set_lfsm_config(uint32_t index, uint32_t val, int8_t * strVal,
                        int8_t * sub_setting, uint32_t set_len)
{
    int32_t ret;

    ret = 0;

    switch (index) {
    case lfsm_reinstall:
        ret = _set_cfg_resetData(val);
        break;
    case lfsm_cn_ssds:
        ret = _set_cfg_ssds(val, sub_setting, set_len);
        break;
    case lfsm_ssds_hpeu:
        ret = _set_cfg_ssdhpeu(val);
        break;
    case lfsm_cn_pgroup:
        ret = _set_cfg_pgroup(val);
        break;
    case lfsm_io_threads:
        ret = _set_cfg_threads(&lfsmCfg.cnt_io_threads,
                               lfsmCfg.ioT_vcore_map, val, sub_setting,
                               set_len);
        break;
    case lfsm_bh_threads:
        ret = _set_cfg_threads(&lfsmCfg.cnt_bh_threads,
                               lfsmCfg.bhT_vcore_map, val, sub_setting,
                               set_len);
        break;
    case lfsm_gc_rthreads:
        lfsmCfg.cnt_gc_rthreads = val;
        break;
    case lfsm_gc_wthreads:
        lfsmCfg.cnt_gc_wthreads = val;
        break;
    case lfsm_gc_seu_reserved:
        lfsmCfg.gc_seu_reserved = val;
        break;
    case lfsm_gc_chk_intvl:
        lfsmCfg.gc_chk_intvl = val;
        break;
    case lfsm_gc_off_upper_ratio:
        lfsmCfg.gc_upper_ratio = val;
        break;
    case lfsm_gc_on_lower_ratio:
        lfsmCfg.gc_lower_ratio = val;
        break;
    case lfsm_max_disk_part:
        lfsmCfg.max_disk_partition = val;
        break;
    case lfsm_raid_info:
    case lfsm_spare_disks:
    case lfsm_capacity:
    case lfsm_physical_cap:
        break;
    case lfsm_osdisk:
        strcpy(lfsmCfg.osdisk_name, strVal);
        break;
    default:
        ret = -EINVAL;
        LOGERR("wrong config item to set %d\n", index);
        break;
    }

    return ret;
}

static void _init_defCfg(lfsm_cfg_t * myCfg)
{
    int32_t i;

    myCfg->resetData = DEF_REINSTALL;
    myCfg->cnt_ssds = DEF_CNSSDS;
    memset(myCfg->ssd_path, '\0',
           sizeof(int8_t) * MAX_SUPPORT_SSD * MAX_DEV_PATH);
    myCfg->ssds_hpeu = DEF_SSD_HPEU;
    myCfg->cnt_pgroup = DEF_CNT_PGROUP;
    myCfg->max_disk_partition = DEF_DISK_PARTITION;

    myCfg->cnt_io_threads = DEF_IO_THREADS;
    for (i = 0; i < MAX_SUPPORT_Thread; i++) {
        myCfg->ioT_vcore_map[i] = -1;
    }
    myCfg->cnt_bh_threads = DEF_BH_THREADS;
    for (i = 0; i < MAX_SUPPORT_Thread; i++) {
        myCfg->bhT_vcore_map[i] = -1;
    }

    myCfg->cnt_gc_rthreads = DEF_GC_RTHREADS;
    myCfg->cnt_gc_wthreads = DEF_GC_WTHREADS;
    myCfg->gc_seu_reserved = DEF_GC_SEU_RESERV;
    myCfg->gc_chk_intvl = DEF_GC_CHK_INTVL;
    myCfg->gc_upper_ratio = DEF_GC_UPPER_RATIO;
    myCfg->gc_lower_ratio = DEF_GC_LOWER_RATIO;
    myCfg->mem_bioset_size = DEF_MEM_BIOSET_SIZE;
    memset(myCfg->lfsm_ver, '\0', sizeof(int8_t) * MAX_STR_LEN);
    strncpy(myCfg->lfsm_ver, LFSM_VERSION, strlen(LFSM_VERSION));
    memset(myCfg->osdisk_name, '\0', sizeof(int8_t) * MAX_DEV_PATH);
}

void init_lfsm_config(void)
{
    _init_defCfg(&lfsmCfg);
}

int32_t show_lfsm_config(int8_t * buffer, int32_t buff_size)
{
    int32_t len, i;

    len = 0;

    len += sprintf(buffer + len, "lfsm_cn_ssds=%d", lfsmCfg.cnt_ssds);
    for (i = 0; i < lfsmCfg.cnt_ssds; i++) {
        if (i == 0) {
            len += sprintf(buffer + len, " %s", lfsmCfg.ssd_path[i]);
        } else {
            len += sprintf(buffer + len, ",%s", lfsmCfg.ssd_path[i]);
        }
    }
    len += sprintf(buffer + len, "\n");

    len += sprintf(buffer + len, "cn_ssds_per_hpeu=%d\n", lfsmCfg.ssds_hpeu);
    len += sprintf(buffer + len, "lfsm_cn_pgroup=%d\n", lfsmCfg.cnt_pgroup);

    len += sprintf(buffer + len, "lfsm_io_thread=%d", lfsmCfg.cnt_io_threads);
    for (i = 0; i < lfsmCfg.cnt_io_threads; i++) {
        if (i == 0) {
            len += sprintf(buffer + len, " %d", lfsmCfg.ioT_vcore_map[i]);
        } else {
            len += sprintf(buffer + len, ",%d", lfsmCfg.ioT_vcore_map[i]);
        }

    }
    len += sprintf(buffer + len, "\n");

    len += sprintf(buffer + len, "lfsm_bh_thread=%d", lfsmCfg.cnt_bh_threads);
    for (i = 0; i < lfsmCfg.cnt_bh_threads; i++) {
        if (i == 0) {
            len += sprintf(buffer + len, " %d", lfsmCfg.bhT_vcore_map[i]);
        } else {
            len += sprintf(buffer + len, ",%d", lfsmCfg.bhT_vcore_map[i]);
        }
    }
    len += sprintf(buffer + len, "\n");

    len +=
        sprintf(buffer + len, "lfsm_gc_rthread=%d\n", lfsmCfg.cnt_gc_rthreads);
    len +=
        sprintf(buffer + len, "lfsm_gc_wthread=%d\n", lfsmCfg.cnt_gc_wthreads);
    len +=
        sprintf(buffer + len, "lfsm_seu_reserved=%d\n",
                lfsmCfg.gc_seu_reserved);
    len +=
        sprintf(buffer + len, "lfsm_gc_chk_intvl=%d\n", lfsmCfg.gc_chk_intvl);
    len +=
        sprintf(buffer + len, "lfsm_gc_off_upper_ratio=%d\n",
                lfsmCfg.gc_upper_ratio);
    len +=
        sprintf(buffer + len, "lfsm_gc_on_lower_ratio=%d\n",
                lfsmCfg.gc_lower_ratio);
    len +=
        sprintf(buffer + len, "lfsm_max_disk_partition=%d\n",
                lfsmCfg.max_disk_partition);
    len += sprintf(buffer + len, "lfsm_ver=%s\n", lfsmCfg.lfsm_ver);
    len += sprintf(buffer + len, "osdisk_name=%s\n", lfsmCfg.osdisk_name);
    len +=
        sprintf(buffer + len, "testCfg.crashSimu=%d\n",
                lfsmCfg.testCfg.crashSimu);
    return len;
}

void exec_lfsm_config_cmdstr(int8_t * buffer, int32_t buff_size)
{
    int8_t *str_gc_intvl = "lfsm_gc_chk_intvl=";
    int8_t *str_gc_on = "lfsm_gc_on_lower_ratio=";
    int8_t *str_gc_off = "lfsm_gc_off_upper_ratio=";
    int8_t *str_crashSimu = "testCfg.crashSimu=";
    int8_t *src_ptr;
    int32_t len_gc_intvl, len_gc_on, len_gc_off, len_crashSimu, val;

    len_gc_intvl = strlen(str_gc_intvl);
    len_gc_on = strlen(str_gc_on);
    len_gc_off = strlen(str_gc_off);
    len_crashSimu = strlen(str_crashSimu);

    val = 0;

    if (strncmp(buffer, str_gc_intvl, len_gc_intvl) == 0) {
        src_ptr = buffer + len_gc_intvl;
        sscanf(src_ptr, "%d", &val);
        if (val >= 100) {
            LOGINFO("[CFG] set gc check interval %d ms\n", val);
            lfsmCfg.gc_chk_intvl = val;
        } else {
            LOGINFO("[CFG] fail set gc check interval %d ms (< 100)\n", val);
        }
    } else if (strncmp(buffer, str_gc_on, len_gc_on) == 0) {
        src_ptr = buffer + len_gc_on;
        sscanf(src_ptr, "%d", &val);
        if (val < 1000) {
            LOGINFO("[CFG] set gc on lower ratio %d\n", val);
            lfsmCfg.gc_lower_ratio = val;
        } else {
            LOGINFO("[CFG] fail set gc on lower ratio %d (> 1000)\n", val);
        }
    } else if (strncmp(buffer, str_gc_off, len_gc_off) == 0) {
        src_ptr = buffer + len_gc_off;
        sscanf(src_ptr, "%d", &val);
        if (val < 1000) {
            LOGINFO("[CFG] set gc off upper ratio %d\n", val);
            lfsmCfg.gc_upper_ratio = val;
        } else {
            LOGINFO("[CFG] fail set gc off upper ratio %d (> 1000)\n", val);
        }
    } else if (strncmp(buffer, str_crashSimu, len_crashSimu) == 0) {
        src_ptr = buffer + len_crashSimu;
        sscanf(src_ptr, "%d", &val);
        if (val < 1000) {
            LOGINFO("[CFG] set testCfg.crashSimu %d\n", val);
            lfsmCfg.testCfg.crashSimu = val;
        } else {
            LOGINFO("[CFG] fail set testCfg.crashSimu %d (> 1000)\n", val);
        }
    } else {
        LOGINFO("[CFG] Unknown or forbidden command: %s\n", buffer);
    }
}
