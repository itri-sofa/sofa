/*
 * SPDX-License-Identifier: GPL-2.0-or-later OR Apache-2.0
 * Copyright (C) 2015-2020 Industrial Technology Research Institute
 *
 *
 *
 *  
 */
#ifndef _DBMTAPOOL_H_
#define _DBMTAPOOL_H_

/* DBMTAPOOL: Direct BMT Array Pool */

// suppose the iops is about 100K and a kmalloc spends 5ms, 
// then there are 100K*5ms pages need to be handled.
// The assumption is under the worse case that one user command is for one page  

//#define dbmtapool_printk(...)  printk("[%s] ", __FUNCTION__);printk(__VA_ARGS__)
#define dbmtapool_printk(...)

#define DBMTAPOOL_ALLOC_HIGHNUM (10)
#define DBMTAPOOL_ALLOC_LOWNUM (8)

//#define DBMTAPOOL_ALLOC_MAXNUM 1000000
#define DBMTAPOOL_EXTRA_BKS 10240

#define DBMTE_NUM  (FLASH_PAGE_SIZE/sizeof(struct D_BMT_E))

typedef struct dbmt_entry_bucket_pool {
    struct kmem_cache *dbmtE_bk_mempool;
    atomic_t dbmtE_bk_used;
    int64_t max_bk_size;
} dbmtE_bk_pool_t;

enum EDBMTA_OP_DIRTYBIT         // operation to be applied on the dirty bit of dbmta
{
    DIRTYBIT_CLEAR = 0,
    DIRTYBIT_SET = 1,
    DIRTYBIT_NOCHANGE = 2
};

#define IS_SET_DIRTY(X) ((X)?(IS_DIRTY_WRITE(X)?DIRTYBIT_SET:DIRTYBIT_CLEAR):DIRTYBIT_NOCHANGE)

struct D_BMT_E {
    sector64 pbno:60;           // 8B with pending mark
    sector64 fake_pbno:1;       //value = 1, pbno is another lpn, value = 0, pbno is true ppn
    sector64 pending_state:2;
    sector64 dirty:1;           // flush to disk marker bit
};

//Direct block mapping entry bucket
typedef struct dbmt_entry_bucket {
    struct D_BMT_E BK_dbmtE[DBMTE_NUM];
} dbmtE_bucket_t;

extern void dbmtE_BKPool_destroy(dbmtE_bk_pool_t * pDbmtapool);
extern int32_t dbmtE_BKPool_init(dbmtE_bk_pool_t * pDbmtapool,
                                 int64_t max_bk_size);
extern dbmtE_bucket_t *dbmtaE_BK_alloc(dbmtE_bk_pool_t * pDbmtapool);
extern void dbmtaE_BK_free(dbmtE_bk_pool_t * pDbmtapool,
                           dbmtE_bucket_t * pdbmta);
extern int32_t dump_dbmtE_BKPool_usage(dbmtE_bk_pool_t * pDbmtapool,
                                       int8_t * buffer);

#endif //_DBMTEPOOL_H_
