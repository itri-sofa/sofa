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
#include <linux/types.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/bio.h>
#include "config/lfsm_setting.h"
#include "config/lfsm_feature_setting.h"
#include "config/lfsm_cfg.h"
#include "common/common.h"
#include "common/rmutex.h"
#include "io_manager/mqbiobox.h"
#include "io_manager/biocbh.h"
#include "nextprepare.h"
#include "diskgrp.h"
#include "system_metadata/Dedicated_map.h"
#include "lfsm_core.h"
#include "mdnode.h"
#include "spare_bmt_ul.h"
#include "mdnode_private.h"
#include "metalog.h"

struct kmem_cache *mdnode_pool_create(void)
{
    struct kmem_cache *pMdpool;
    pMdpool =
        kmem_cache_create("md_pool", sizeof(smd_node_t), 0, SLAB_HWCACHE_ALIGN,
                          NULL);
    atomic_set(&count, 0);
    atomic_set(&num, 0);
    return pMdpool;
}

void mdnode_pool_destroy(struct kmem_cache *pMdpool)
{
    if (pMdpool) {
        kmem_cache_destroy(pMdpool);
    }
}

int32_t mdnode_set_state_copy(smd_t * pmd, wait_queue_head_t * pwq)
{
    int32_t ret;

  l_again:
    ret = atomic_cmpxchg(&pmd->fg_state, md_exist, md_copying);
    switch (ret) {
    case md_empty:
        return -ENOENT;
    case md_exist:
        return 0;
    case md_copying:

        LOGINFO("wait stat is md_coping \n");
        wait_event_interruptible((*pwq),
                                 atomic_read(&pmd->fg_state) != md_copying);
        LOGINFO("wake up stat is md_coping \n");

        goto l_again;

    default:
        break;
    }

    return 0;
}

void mdnode_set_state(smd_t * pmd, int32_t mdstate)
{
    atomic_set(&pmd->fg_state, mdstate);
}

int32_t mdnode_alloc(smd_t * pmd, struct kmem_cache *pMdpool)
{
    int32_t i;

    if (NULL != pmd->mdnode) {
        return -ENOENT;
    }

    if (atomic_cmpxchg(&pmd->fg_state, md_empty, md_exist) != md_empty) {
        LOGERR("alloc fail: pmd->mdnode %p state %d\n", pmd->mdnode,
               atomic_read(&pmd->fg_state));
        return -ENOENT;
    }

    if (NULL == (pmd->mdnode = kmem_cache_alloc(pMdpool, GFP_KERNEL))) {
        return -ENOMEM;
    }

    mdnode_set_state(pmd, md_exist);
    for (i = 0; i < FPAGE_PER_SEU; i++) {
        pmd->mdnode->lpn[i] = TAG_METADATA_EMPTY;
    }

    return 0;
}

int32_t mdnode_free(smd_t * pmd, struct kmem_cache *pMdpool)
{
    int32_t ret;
    if ((ret = atomic_cmpxchg(&pmd->fg_state, md_exist, md_empty)) != md_exist) {
        if (ret != md_copying) {
            LOGERR("have been free: pmd->mdnode %p state %d\n",
                   pmd->mdnode, atomic_read(&pmd->fg_state));
        }
        return -EBUSY;
    }
    kmem_cache_free(pMdpool, pmd->mdnode);
    pmd->mdnode = NULL;
    return 0;
}

void mdnode_set_lpn(smd_node_t * pNode, sector64 lpn, int32_t idx)
{
    if (idx >= FPAGE_PER_SEU) {
        LOGERR("idx %d over flow\n", idx);
        return;
    }
    pNode->lpn[idx] = lpn;
}

static void mdnode_free_ifneed(struct kmem_cache *pMdpool,
                               struct EUProperty *eu)
{
    if (NULL == eu->act_metadata.mdnode) {
        return;
    }
    mdnode_free(&eu->act_metadata, pMdpool);
}

int32_t mdnode_free_all(lfsm_dev_t * td)
{
    uint64_t freenum;
    int32_t i;

    freenum = grp_ssds.capacity >> (SECTORS_PER_SEU_ORDER);
    if (pbno_eu_map == 0) {
        return 0;
    }

    for (i = 0; i < freenum; i++) {
        mdnode_free_ifneed(td->pMdpool, pbno_eu_map[i]);
    }
    return 0;
}

void mdnode_init(smd_t * pmd)
{
    pmd->mdnode = NULL;
    atomic_set(&pmd->fg_state, md_empty);
}

int32_t mdnode_item_added(smd_node_t * pNode)
{
    int32_t i;

    if (NULL == pNode) {
        return -EEXIST;
    }

    for (i = 0; i < FPAGE_PER_SEU; i++) {
        if (pNode->lpn[i] == TAG_METADATA_EMPTY) {
            break;
        }
    }

    return i;
}

static bool mdnode_check_existA(struct EUProperty *eu)
{
    if (NULL == eu->act_metadata.mdnode) {
        return false;
    }
    LOGINFO("Disk[%d] eu %llu act(%d) onssd(%d) list(%d)\n", eu->disk_num,
            eu->start_pos, eu_is_active(eu), eu->fg_metadata_ondisk,
            atomic_read(&eu->state_list));
    return true;
}

int32_t mdnode_hpeu_check_exist(struct HPEUTab *phpeutab,
                                struct EUProperty **ar_peu, int32_t id_faildisk)
{
    int32_t i;
    for (i = 0; i < phpeutab->cn_disk; i++) {
        mdnode_check_existA(ar_peu[i]);
    }
    return 0;
}

int32_t mdnode_check_exist_all(void)
{
    uint64_t freenum;
    int32_t i;

    freenum = grp_ssds.capacity >> (SECTORS_PER_SEU_ORDER);
    if (NULL == pbno_eu_map) {
        return 0;
    }
    for (i = 0; i < freenum; i++) {
        mdnode_check_existA(pbno_eu_map[i]);
    }
    return 0;
}

int32_t mdnode_get_metadata(int8_t * buffer, sector64 addr, int32_t size)
{
    struct EUProperty *peu;
    int32_t idx;

    idx = 0;
    if (NULL == (peu = pbno_return_eu_idx(addr, &idx))) {
        return -EPERM;
    }

    if (NULL == peu->act_metadata.mdnode) {
        if (pos_latest != peu->start_pos) {
            LOGWARN("no metalog for eu %llu, setting all metadata to 0xff\n",
                    peu->start_pos);
        }
        pos_latest = peu->start_pos;
        //dump_stack();
        memset(buffer, 0xff, size);
        return 0;
    }

    if (pos_latest != peu->start_pos) {
        LOGWARN("eu %llu metadata from mlog\n", peu->start_pos);
    }

    pos_latest = peu->start_pos;

    memcpy(buffer, &peu->act_metadata.mdnode->lpn[idx], size);
    return 0;
}
