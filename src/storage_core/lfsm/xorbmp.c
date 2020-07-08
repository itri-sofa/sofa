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
//    Name: xorbmp
//    Background 1:
//         As shown in the below figure, although most bits in 8-bytes spare-area
//        are used for storing LPN, there are (RDDCY_ECC_MARKER_BIT_NUM) bits in
//        the spare-area reserved for data redundancy. We call these bits as
//        stripe-bits.
//         |--------- data-area ------------------|---spare-area --------------]
//         |------------data----------------------|-- stripe-bits --|----LPN---]
//         For a page storing user data, these bits are zero.
//         For a page storing parity-check data, we use these bits to store the
//        length of its corresponding stripe.
//    Background 2:
//         If user plans to read a page from a crashed drive before SOFA finishes
//        the whole rebuilding process, SOFA needs to real-time rebuild the page
//        for user. to rebuild the page, SOFA identify its corresponding stripe.
//        Then, get the crash page by xoring other pages included in the stripe.
//    Overview:
//         Allocate a on-mem table to keeps the stripe-bits of all pages in one SEU.
//        The table is used in the case of Background 2 to help SOFA
//        finds out the stripe related to the crash page.
//
//        To save memory space, one byte is divided into 2 4-bits units to store
//        the stripe-bits of 2 pages
//
//    Key function name:
//        1. xorbmp_get
//        2. xorbmp_load_ifneed
//    Related component:
//        active_metadata@EUProperty
//        metabioc.c/h
///////////////////////////////////////////////////////////////////////////////

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
#include "xorbmp.h"
#include "io_manager/io_common.h"
#include "devio.h"
#include "EU_access.h"
#include "mdnode.h"
#include "xorbmp_private.h"

void xorbmp_init(sxorbmp_t * xb)
{
    mutex_init(&xb->lock);
    xb->vector = 0;
    return;
}

static void xorbmp_free_A(sxorbmp_t * xb)
{
    if (xb->vector) {
        track_kfree(xb->vector, (FPAGE_PER_SEU >> 3), M_XORBMP);
        xorbmp_cn_alloced--;
    }
    xb->vector = 0;
}

void xorbmp_destroy(sxorbmp_t * xb)
{
    xorbmp_free_A(xb);
    mutex_destroy(&xb->lock);
}

static int32_t xorbmp_alloc(sxorbmp_t * xb)
{
    if (NULL == (xb->vector = (int8_t *)
                 track_kmalloc(FPAGE_PER_SEU >> 3, GFP_KERNEL, M_XORBMP))) {
        LOGERR("alloc vector fail\n");
        return -ENOMEM;
    }

    xorbmp_cn_alloced++;
    return 0;
}

void xorbmp_free(sxorbmp_t * xb)
{
    mutex_lock(&xb->lock);
    if (xb->vector) {
        track_kfree(xb->vector, (FPAGE_PER_SEU >> 3), M_XORBMP);
        xorbmp_cn_alloced--;
    }
    xb->vector = 0;
    mutex_unlock(&xb->lock);
}

static int32_t xorbmp_load_from_disk(sxorbmp_t * xb, sector64 pbno_eu)
{
    sector64 *buf;
    int32_t i, ret;
    uint8_t res;

    LOGINFO("read metadata of eu disk %d start_pos=%lld\n",
            eu_find(pbno_eu)->disk_num, pbno_eu);
    if (NULL == (buf = (sector64 *)
                 track_kmalloc(METADATA_SIZE_IN_FLASH_PAGES * FLASH_PAGE_SIZE,
                               GFP_KERNEL, M_XORBMP))) {
        LOGERR("alloc buffer failure\n");
        return -ENOMEM;
    }

    if ((ret = xorbmp_alloc(xb)) < 0) {
        goto l_end;
    }

    if (!devio_reread_pages(pbno_eu + SECTORS_PER_SEU - METADATA_SIZE_IN_SECTOR,
                            (int8_t *) buf, METADATA_SIZE_IN_FLASH_PAGES)) {
        ret = -EIO;
        goto l_iofail;
    }

    //translate from 8 byte to 4 bit vector.
#if RDDCY_ECC_MARKER_BIT_NUM > 4
#error the module did not support RDDCY_ECC_MARKER_BIT_NUM > 4
#endif

    for (i = 0; i < FPAGE_PER_SEU; i++) {
        if (SPAREAREA_IS_EMPTY(buf[i]))
            res = 0;
        else
            res = 1;

        xb->vector[i >> 3] |= res << (i & 0x07);
    }
    goto l_end;
  l_iofail:
    xorbmp_free_A(xb);
  l_end:
    track_kfree(buf, METADATA_SIZE_IN_FLASH_PAGES * FLASH_PAGE_SIZE, M_XORBMP);
    return ret;
}

static int32_t xorbmp_load_from_mem(sxorbmp_t * xb, struct EUProperty *eu)
{
    int32_t i, ret;
    uint8_t res;

    if ((ret = xorbmp_alloc(xb)) < 0) {
        return ret;
    }

    memset(xb->vector, 0, (FPAGE_PER_SEU >> 3));
    if (NULL == (eu->act_metadata.mdnode)) {
        return -ENOMEM;
    }

    for (i = 0; i < eu->log_frontier >> SECTORS_PER_FLASH_PAGE_ORDER; i++) {
        if (SPAREAREA_IS_EMPTY(eu->act_metadata.mdnode->lpn[i]))
            res = 0;
        else
            res = 1;
        xb->vector[i >> 3] |= res << (i & 0x07);
//        printk("%s: cn=%d loaded lbno=%x xb->vector[%d]=%02x\n",__FUNCTION__, cn, pRetItem->mdata.lbno,cn>>1,xb->vector[cn>>1]);
    }
    return 0;
}

// We can get the xorbmp table of a SEU from memory directly if the SEU is actived.
// Otherwise, we have to load the table from disk
// At the moment which the SEU is turning from active to inactive and the on-memory metadata is committing to disk,
// xorbmp_load_from_mem() will return -EAGAIN and then the function retry
// in the second round, fg_metadata_ondisk should change to true,
// and we use xorbmp_load_from_disk() to get data from disk
static int32_t xorbmp_load_ifneed(sxorbmp_t * xb, struct EUProperty *eu)
{
    int32_t ret, cn_retry;

//    printk("%s: called eu->fg_metadata_ondisk=%d\n",__FUNCTION__,eu->fg_metadata_ondisk);
    if (xb->vector) {
        return 0;
    }

    ret = 0;
    cn_retry = 0;
    do {
        if (ret == -EAGAIN) {
            set_current_state(TASK_UNINTERRUPTIBLE);
            schedule_timeout(250);  // sleep 1 second
            set_current_state(TASK_INTERRUPTIBLE);
            if (cn_retry == 10) {
                return -EBUSY;
            }
        }

        if (NULL != (eu->act_metadata.mdnode)) {
            if ((ret = xorbmp_load_from_mem(xb, eu)) == -EAGAIN) {
                LOGWARN("rx -EAGAIN %d\n", cn_retry);
                cn_retry++;
                schedule();
            }
        } else {
            ret = xorbmp_load_from_disk(xb, eu->start_pos);
        }
    } while (ret == -EAGAIN);

    return ret;
}

// One can use the function to query the stripe-bits for the page of (eu, idx_fpage)
// The function use xorbmp_load_ifneed() to pre-load all stripe-bits from a SEU where the page is located.
int32_t xorbmp_get(struct EUProperty *eu, int32_t idx_fpage)
{
    sxorbmp_t *xb;
    int32_t ret;

    if (NULL == eu) {
        return -EINVAL;
    }

    xb = &eu->xorbmp;
    if (idx_fpage >= FPAGE_PER_SEU) {
        return -EPERM;
    }

    mutex_lock(&xb->lock);

    if ((ret = xorbmp_load_ifneed(xb, eu)) < 0) {
        mutex_unlock(&xb->lock);
        return ret;
    }

    ret = (int32_t) ((xb->vector[idx_fpage >> 3] >> (idx_fpage & 0x07)) & 0x01);
    mutex_unlock(&xb->lock);
    return ret;                 // return sz_stripe
}
