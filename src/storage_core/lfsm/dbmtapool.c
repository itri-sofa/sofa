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
#include <linux/bio.h>
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
#include "lfsm_core.h"
#include "dbmtapool.h"
#include "bmt_ppdm.h"
#include "io_manager/io_common.h"

static struct kmem_cache *dbmtE_bucket_pool_create(void)
{
    struct kmem_cache *pool;
    pool =
        kmem_cache_create("dbmta_pool", sizeof(dbmtE_bucket_t), 0,
                          SLAB_HWCACHE_ALIGN, NULL);
    return pool;
}

void dbmtE_BKPool_destroy(dbmtE_bk_pool_t * bk_pool)
{
    if (atomic_read(&bk_pool->dbmtE_bk_used) > 0) {
        LOGWARN("Unfree bk_pool dbmtE_bk_used %d\n",
                atomic_read(&bk_pool->dbmtE_bk_used));
    }

    if (bk_pool->dbmtE_bk_mempool) {
        kmem_cache_destroy(bk_pool->dbmtE_bk_mempool);
    }
}

int32_t dbmtE_BKPool_init(dbmtE_bk_pool_t * bk_pool, int64_t max_pool_size)
{
    int32_t ret;

    ret = 0;
    atomic_set(&bk_pool->dbmtE_bk_used, 0);
    bk_pool->dbmtE_bk_mempool = NULL;
    if (IS_ERR_OR_NULL(bk_pool->dbmtE_bk_mempool = dbmtE_bucket_pool_create())) {
        LOGERR("ERROR fail create dbmt entry bucket memory pool\n");
        ret = -ENOMEM;
    } else {
        bk_pool->max_bk_size = (max_pool_size + DBMTAPOOL_EXTRA_BKS);
        LOGINFO("INFO create dbmt entry bucket memory pool with max size %lld "
                "(%lld %d)\n", bk_pool->max_bk_size, max_pool_size,
                DBMTAPOOL_EXTRA_BKS);
    }

    return ret;
}

dbmtE_bucket_t *dbmtaE_BK_alloc(dbmtE_bk_pool_t * bk_pool)
{
    dbmtE_bucket_t *my_bucket;

    if (atomic_read(&bk_pool->dbmtE_bk_used) >= bk_pool->max_bk_size) {
        return NULL;
    }

    if (NULL ==
        (my_bucket = kmem_cache_alloc(bk_pool->dbmtE_bk_mempool, GFP_KERNEL))) {
        return NULL;
    }

    atomic_inc(&bk_pool->dbmtE_bk_used);
    //Tifa: memset 0 imply dbmta_unit pending_state=0(EMPTY) and dirty=0
    memset(my_bucket, 0, sizeof(dbmtE_bucket_t));
    return my_bucket;
}

void dbmtaE_BK_free(dbmtE_bk_pool_t * bk_pool, dbmtE_bucket_t * my_bucket)
{
    if ((my_bucket) && (bk_pool->dbmtE_bk_mempool)) {
        kmem_cache_free(bk_pool->dbmtE_bk_mempool, my_bucket);
        atomic_dec(&bk_pool->dbmtE_bk_used);
    }
}

int32_t dump_dbmtE_BKPool_usage(dbmtE_bk_pool_t * pDbmtapool, int8_t * buffer)
{
    int32_t len;

    len = 0;

    len += sprintf(buffer + len,
                   "DBMT Entry Bucket Pool: (max, allocated) = (%lld %d)\n",
                   pDbmtapool->max_bk_size,
                   atomic_read(&pDbmtapool->dbmtE_bk_used));

    return len;
}
