/*
 * SPDX-License-Identifier: GPL-2.0-or-later OR Apache-2.0
 * Copyright (C) 2015-2020 Industrial Technology Research Institute
 *
 *
 *
 *  
 */
#include <linux/types.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/wait.h>
#include "config/lfsm_setting.h"
#include "config/lfsm_feature_setting.h"
#include "common/common.h"
#include "common/rmutex.h"
#include "config/lfsm_cfg.h"
#include "io_manager/mqbiobox.h"
#include "io_manager/biocbh.h"
#include "nextprepare.h"
#include "diskgrp.h"
#include "system_metadata/Dedicated_map.h"
#include "lfsm_core.h"          //no need
#include "pgcache.h"

/*
 * Description: Update data in page cache, if page cache is NULL, alloc memory
 */
int32_t pgcache_update(spgcache_t * pgc, sector64 lpn, struct bio_vec *pvec_src,
                       bool isForceUpdate)
{
    int8_t *src;
    int32_t i, idx;

    idx = (lpn * PGCACHE_BIGPRIME) % MAX_PGCACHE_HOOK;
    mutex_lock(&pgc->lock);
    if ((!isForceUpdate) && (pgc->ar_lpn[idx] != lpn)) {
        goto l_end;
    }
    pgc->ar_lpn[idx] = lpn;
    if (NULL == pgc->ar_fpage[idx]) {
        pgc->ar_fpage[idx] = kmalloc(FLASH_PAGE_SIZE, GFP_KERNEL);
    }

    for (i = 0; i < MEM_PER_FLASH_PAGE; i++) {
        src = kmap(pvec_src[i].bv_page);
        memcpy(pgc->ar_fpage[idx] + (i << PAGE_SIZE_ORDER), src, PAGE_SIZE);
        kunmap(pvec_src[i].bv_page);
    }

  l_end:
    mutex_unlock(&pgc->lock);

    return 0;
}

/*
 * Description: pre-load data to page cache
 *              lpn : in flash page (fpage)
 *              pvec_dest : in memory page (4KB)
 */
int32_t pgcache_load(spgcache_t * pgc, sector64 lpn, struct bio_vec *pvec_dest)
{
    int8_t *dest;
    int32_t i, ret, idx;

    ret = -ENXIO;
    idx = (lpn * PGCACHE_BIGPRIME) % MAX_PGCACHE_HOOK;

    mutex_lock(&pgc->lock);
    if (lpn == pgc->ar_lpn[idx]) {
        for (i = 0; i < MEM_PER_FLASH_PAGE; i++) {
            dest = kmap(pvec_dest[i].bv_page);
            memcpy(dest, pgc->ar_fpage[idx] + (i << PAGE_SIZE_ORDER),
                   PAGE_SIZE);
            kunmap(pvec_dest[i].bv_page);
        }
        pgc->preload_count++;
        pgc->preload_hit++;
        ret = 0;
    }

    //    printk("preload count=%d, preload hit=%d\n", pgc->preload_count, pgc->preload_hit);
    mutex_unlock(&pgc->lock);

    return ret;
}

/*
 * Description: init page cache for partial IO
 */
int32_t pgcache_init(spgcache_t * pgc)
{
    if (IS_ERR_OR_NULL(pgc)) {
        LOGERR("page cache is null when init\n");
        return -ENOMEM;
    }

    memset(pgc->ar_fpage, 0, sizeof(int8_t *) * MAX_PGCACHE_HOOK);
    memset(pgc->ar_lpn, -1, sizeof(sector64) * MAX_PGCACHE_HOOK);
    pgc->preload_count = 0;
    pgc->preload_hit = 0;
    mutex_init(&pgc->lock);
    return 0;
}

/*
 * Description: free all data pages in page cache data structure
 */
void pgcache_destroy(spgcache_t * pgc)
{
    int32_t i;

    for (i = 0; i < MAX_PGCACHE_HOOK; i++) {
        if (!IS_ERR_OR_NULL(pgc->ar_fpage[i])) {
            kfree(pgc->ar_fpage[i]);
        }
    }

    mutex_destroy(&pgc->lock);
}
