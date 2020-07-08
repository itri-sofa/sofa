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
///////////////////////////////////////////////////////////////////////////////
//    Overview:
//        Herein pgroup means protection group. For an active SEU of each temperature, pgroup
//        allocates an active stripes named stripe@pgroup to handle the current write operation of the temperature
//    Key function name:
//        lfsm_pgroup_selector(): select a pgroup for a write operation.
//            If the write is a user data write, it uses lfsm_pgroup_select_normal() to select a pgroup
//            If it is a gc write, it uses lfsm_pgroup_select_GC(). The major difference between the 2 selectors is
//        lfsm_pgroup_select_normal(): pick the pgroup with smaller number of total
//                                valid pages to balance the number of valid pages between pgroups
//        lfsm_pgroup_select_GC(): We use differnt policy to select pgroup for gc write. The policy is selecting either
//                                the pgroup where the source data is located or the pgroup with larger number of free SEUs
//                                to balance the free SEUs between pgroups
//        lfsm_pgroup_retref_hpeuid(): Return the id of the current active HPEU according to a specific disk id and temperature
///////////////////////////////////////////////////////////////////////////////////
#include <linux/types.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/wait.h>
#include "config/lfsm_setting.h"
#include "config/lfsm_feature_setting.h"
#include "config/lfsm_cfg.h"
#include "common/common.h"
#include "common/mem_manager.h"
#include "common/rmutex.h"
#include "io_manager/mqbiobox.h"
#include "io_manager/biocbh.h"
#include "nextprepare.h"
#include "diskgrp.h"
#include "system_metadata/Dedicated_map.h"
#include "lfsm_core.h"          //no need
#include "EU_access.h"
#include "stripe.h"
#include "pgroup.h"
#include "io_manager/io_write.h"
#include "io_manager/io_common.h"
#include "pgroup_private.h"
#include "system_metadata/Super_map.h"
#include "perf_monitor/ssd_perf_mon.h"

// return max_birthday,
// check max_birthday except id_fail_pgroup
static sector64 lfsm_pgroup_get_max_birthday(spgroup_t * ar_grp,
                                             int32_t temperature,
                                             int32_t id_pgroup_skip)
{
    int32_t i;
    sector64 max_birthday;

    max_birthday = 0;
    for (i = 0; i < lfsmdev.cn_pgroup; i++) {
        if (i == id_pgroup_skip) {
            continue;
        }
        if (atomic64_read(&ar_grp[i].ar_birthday[temperature]) > max_birthday) {
            max_birthday = atomic64_read(&ar_grp[i].ar_birthday[temperature]);
        }
    }

    return max_birthday;
}

// this function only called by HPEU_alloc_and_add, the outside is protected by whole_lock
sector64 lfsm_pgroup_birthday_next(lfsm_dev_t * td, int32_t id_pgroup,
                                   int32_t temperature)
{
    atomic64_inc(&td->ar_pgroup[id_pgroup].ar_birthday[temperature]);
    atomic64_set(&td->ar_pgroup[id_pgroup].ar_birthday[temperature],
                 lfsm_pgroup_get_max_birthday(td->ar_pgroup, temperature, -1));
    return atomic64_read(&td->ar_pgroup[id_pgroup].ar_birthday[temperature]);
}

int32_t lfsm_pgroup_lookup(sector64 pbno)
{
    struct EUProperty *eu;
    eu = eu_find(pbno);
    if (eu > 0) {
        return PGROUP_ID(eu->disk_num);
    } else {
        return -ENODEV;
    }
}

static int32_t lfsm_pgroup_select_GC(int32_t cn_pgroup, spgroup_t * ar_grp,
                                     struct bio_container *bioc,
                                     int32_t id_grp_selected)
{
    int32_t i, id_grp_src;
    for (i = 0; i < cn_pgroup; i++) {
        if (ar_grp[id_grp_selected].state == epg_good) {
            id_grp_src = PGROUP_ID(eu_find(bioc->source_pbn)->disk_num);
            if (id_grp_src == id_grp_selected) {
                return id_grp_selected;
            } else if (gc[ar_grp[id_grp_selected].min_id_disk]->
                       free_list_number > MIN_RESERVE_EU) {
                return id_grp_selected;
            }
        }
        id_grp_selected = (id_grp_selected + 1) % cn_pgroup;
    }
    return -ENODEV;
}

static int32_t lfsm_pgroup_select_normal(int32_t cn_pgroup, spgroup_t * ar_grp,
                                         struct bio_container *bioc,
                                         int32_t id_grp_selected, bool isRetry)
{
    int32_t i, id_min, cn_min_valid;

    id_min = -1;
    cn_min_valid = 0x7fffffff;

    for (i = 0; i < cn_pgroup; i++) {
        if (ar_grp[id_grp_selected].state == epg_good) {
            if ((isRetry == true) ||
                (atomic64_read(&ar_grp[id_grp_selected].cn_valid_page) <=
                 ar_grp[i].ideal_valid_page + FPAGE_PER_SEU)) {
                return id_grp_selected;
            }
            //// calculate min cn_valid id_grp
            if (atomic64_read(&ar_grp[id_grp_selected].cn_valid_page) <
                cn_min_valid) {
                id_min = id_grp_selected;
                cn_min_valid =
                    atomic64_read(&ar_grp[id_grp_selected].cn_valid_page);
            }
        }
        id_grp_selected = (id_grp_selected + 1) % cn_pgroup;
    }
    //// (all is fail disk) || (valid of all disk are bound, and min valid is over 1/2reserve)
    if ((i == cn_pgroup && id_min == -1)
        || (cn_min_valid > ar_grp[id_min].max_valid_page)) {
        LOGERR("cn_min %d\n", cn_min_valid);
        return -ENODEV;
    } else {
        atomic_inc(&grp_ssds.ar_cn_selected[id_min]);
        return id_min;
    }

    return id_grp_selected;
}

int32_t lfsm_pgroup_selector(spgroup_t * ar_grp, int32_t temperature,
                             sector64 pbno_orig, struct bio_container *bio_c,
                             bool isRetry)
{
    int32_t pgrp_selected, pgrp_last_good, i, n2;

// FIXME: Weafon Dirty solution....
    pgrp_selected =
        (lfsm_atomic_inc_return(&lfsm_arpgroup_wcn[temperature])) %
        lfsmdev.cn_pgroup;
    pgrp_last_good = -1;
//    printk("Ding(%s): pgrp_selected=%d. temperature=%d\n", __FUNCTION__, pgrp_selected, temperature);

    if (temperature != TEMP_LAYER_NUM - 1) {
        goto l_no_hot;
    }

    for (i = 0; i < lfsmdev.cn_pgroup; i++) {
        n2 = (pgrp_selected + 1) % lfsmdev.cn_pgroup;
        if (ar_grp[pgrp_selected].state == epg_good) {
            pgrp_last_good = pgrp_selected;
            if ((ar_grp[n2].state == epg_good)
                && ((isRetry == true)
                    ||
                    (atomic64_read
                     (&ar_grp[pgrp_selected].ar_birthday[TEMP_LAYER_NUM - 1]) <=
                     atomic64_read(&ar_grp[n2].
                                   ar_birthday[TEMP_LAYER_NUM - 1])))
                && ((isRetry == true)
                    || (atomic64_read(&ar_grp[pgrp_selected].cn_valid_page) <=
                        ar_grp[i].ideal_valid_page + FPAGE_PER_SEU))) {
                break;
            }
        }
        pgrp_selected = (pgrp_selected + 1) % lfsmdev.cn_pgroup;
    }

    if (ar_grp[pgrp_selected].state != epg_good)
        pgrp_selected = pgrp_last_good;

    if (i < lfsmdev.cn_pgroup) {
        return pgrp_selected;
    }

  l_no_hot:

    if (bio_c->write_type == TP_GC_IO) {
        return lfsm_pgroup_select_GC(lfsmdev.cn_pgroup, ar_grp, bio_c,
                                     pgrp_selected);
    } else {
        return lfsm_pgroup_select_normal(lfsmdev.cn_pgroup, ar_grp, bio_c,
                                         pgrp_selected, isRetry);
    }
}

void lfsm_pgroup_valid_page_init(spgroup_t * ar_grp, sector64 logi_capacity,
                                 sector64 plus_capacity)
{
    int32_t i;
    for (i = 0; i < lfsmdev.cn_pgroup; i++) {
        ar_grp[i].ideal_valid_page = logi_capacity;
        ar_grp[i].max_valid_page = logi_capacity + plus_capacity / 2;
    }
}

struct lfsm_stripe *lfsm_pgroup_stripe_lock(lfsm_dev_t * td, int32_t id_pgroup,
                                            int32_t temperature)
{
    spgroup_t *ppg;

    if ((id_pgroup < 0) || (id_pgroup >= td->cn_pgroup)) {
        return NULL;
    }

    if (temperature >= TEMP_LAYER_NUM) {
        return NULL;
    }

    ppg = &td->ar_pgroup[id_pgroup];
    mutex_lock(&ppg->stripe[temperature].lock);
    return &ppg->stripe[temperature];
}

int32_t *lfsm_pgroup_retref_hpeuid(lfsm_dev_t * td, int32_t id_disk,
                                   int32_t temperature)
{
    int32_t id_pgroup;

    id_pgroup = PGROUP_ID(id_disk);
    if ((id_pgroup < 0) || (id_pgroup >= td->cn_pgroup)) {
        goto l_fail;
    }

    if ((temperature < 0) || (temperature >= TEMP_LAYER_NUM)) {
        goto l_fail;
    }

    if ((id_disk < 0) || (id_disk >= td->cn_ssds)) {
        goto l_fail;
    }

    // printk("%s() return id_group=%d temperature=%d id_hpeu=%d\n", __func__,id_pgroup,temperature, td->ar_pgroup[id_pgroup].stripe[temperature].id_hpeu);
    return &(td->ar_pgroup[id_pgroup].stripe[temperature].id_hpeu);

  l_fail:
    LOGERR("false id_group=%d temperature=%d id_disk = %d\n", id_pgroup,
           temperature, id_disk);

    return NULL;
}

int32_t lfsm_pgroup_set_hpeu(lfsm_dev_t * td, int32_t id_disk,
                             int32_t temperature, int32_t id_hpeu)
{
    int32_t id_pgroup;

    id_pgroup = PGROUP_ID(id_disk);
    if ((id_pgroup < 0) || (id_pgroup >= td->cn_pgroup)) {
        return -EINVAL;
    }
    if ((temperature < 0) || (temperature >= TEMP_LAYER_NUM)) {
        return -EINVAL;
    }

    td->ar_pgroup[id_pgroup].stripe[temperature].id_hpeu = id_hpeu;
    return 0;
}

int32_t lfsm_pgroup_get_temperature(lfsm_dev_t * td, struct lfsm_stripe *psp)
{
    int32_t t;
    if ((psp->id_pgroup < 0) || (psp->id_pgroup >= td->cn_pgroup)) {
        LOGERR("invalid psp %p id_pgroup %d\n", psp, psp->id_pgroup);
        return -EINVAL;
    }
    // ar_pgroup must be allocated by kmalloc (continue memory)
    t = (psp - td->ar_pgroup[psp->id_pgroup].stripe);
    if ((t < 0) || (t >= TEMP_LAYER_NUM)) {
        LOGERR("fail to get temperature from psp %p\n", psp);
        return -EINVAL;
    }

    return t;
}

int32_t pgroup_disk_all_active(int32_t id_pgroup, diskgrp_t * pgrp,
                               int32_t cn_ssds_per_hpeu)
{
    int32_t i;
    for (i = (id_pgroup * cn_ssds_per_hpeu); i < cn_ssds_per_hpeu; i++) {
        if (pgrp->ar_drives[i].actived == 0) {
            return -EINVAL;
        }
    }

    return 0;
}

int32_t pgroup_birthday_reassign(spgroup_t * ar_grp, int32_t id_pgroup)
{
    int32_t t;

    if ((id_pgroup < 0) || (id_pgroup >= lfsmdev.cn_pgroup)) {
        LOGERR("invalid id_pgroup %d\n", id_pgroup);
        return -EINVAL;
    }

    for (t = 0; t < TEMP_LAYER_NUM; t++) {
        atomic64_set(&ar_grp[id_pgroup].ar_birthday[t],
                     lfsm_pgroup_get_max_birthday(ar_grp, t, id_pgroup));
    }
    return 0;
}

static int32_t pgroup_init(spgroup_t * ppg, int32_t id_pgroup)
{
    sector64 i;
    ppg->state = epg_good;
    init_waitqueue_head(&ppg->wq);

    for (i = 0; i < TEMP_LAYER_NUM; i++) {
        if (!stripe_init(&ppg->stripe[i], id_pgroup)) {
            return -ENOMEM;
        }
        // +1 to prevent signed number issue.
        atomic64_set(&ppg->ar_birthday[i],
                     (i << (64 - (TEMP_LAYER_NUM_ORDER + 1))));
    }
    atomic64_set(&ppg->cn_valid_page, 0);
    ppg->max_valid_page = 10240;
    ppg->ideal_valid_page = 10240;
    atomic_set(&ppg->cn_gcing, 0);

    return 0;
}

/*
 * Description: alloc and initial protection group
 * return : 0 : init ok
 *         non 0 : init fail
 */
int32_t lfsm_pgroup_init(lfsm_dev_t * td)
{
    int32_t i;

    td->ar_pgroup =
        (spgroup_t *) track_kmalloc(sizeof(spgroup_t) * td->cn_pgroup,
                                    GFP_KERNEL, M_PGROUP);
    if (IS_ERR_OR_NULL(td->ar_pgroup)) {
        LOGERR(" fail alloc mem to protect group\n");
        return -ENOMEM;
    }

    memset(td->ar_pgroup, 0, sizeof(spgroup_t) * td->cn_pgroup);

    for (i = 0; i < TEMP_LAYER_NUM; i++) {
        atomic_set(&lfsm_arpgroup_wcn[i], -1);
    }

    spin_lock_init(&lfsm_pgroup_birthday_lock);

    ppool_wctrl =
        kmem_cache_create("ppool_wctrl", sizeof(struct lfsm_stripe_w_ctrl), 0,
                          SLAB_HWCACHE_ALIGN, NULL);

    LOGINFO("Create wctrl_pool %p\n", ppool_wctrl);

    if (IS_ERR_OR_NULL(ppool_wctrl)) {
        LOGERR("fail create ppool_wctrl %p \n", ppool_wctrl);
        return -ENOSPC;
    }

    for (i = 0; i < td->cn_pgroup; i++) {
        if (pgroup_init(&td->ar_pgroup[i], i) < 0) {
            LOGERR(" fail init protect group %d\n", i);
            return -ENOMEM;
        }
    }
    atomic_set(&lfsm_pgroup_cn_padding_page, 0);
    return 0;
}

static int32_t pgroup_destroy(spgroup_t * ppg)
{
    int32_t i;
    if (IS_ERR_OR_NULL(ppg)) {
        goto l_skip;
    }
    for (i = 0; i < TEMP_LAYER_NUM; i++) {
        stripe_destroy(&ppg->stripe[i]);
    }

  l_skip:
    return 0;
}

void lfsm_pgroup_destroy(lfsm_dev_t * td)
{
    int32_t i;

    if (IS_ERR_OR_NULL(td->ar_pgroup)) {
        return;
    }

    for (i = 0; i < td->cn_pgroup; i++) {
        pgroup_destroy(&td->ar_pgroup[i]);
    }

    if (ppool_wctrl) {
        kmem_cache_destroy(ppool_wctrl);
    }

    ppool_wctrl = NULL;

    if (td->ar_pgroup) {
        track_kfree(td->ar_pgroup, sizeof(spgroup_t) * td->cn_pgroup, M_PGROUP);
    }

    return;
}
