/*
 * SPDX-License-Identifier: GPL-2.0-or-later OR Apache-2.0
 * Copyright (C) 2015-2020 Industrial Technology Research Institute
 *
 *
 *
 *  
 */
#ifndef PGCACHE_INC_198273917239817232
#define PGCACHE_INC_198273917239817232

#define MAX_PGCACHE_HOOK 10240
#define PGCACHE_BIGPRIME 179426549U

typedef struct spgcache {
    sector64 ar_lpn[MAX_PGCACHE_HOOK];  /* LPN which has page cache */
    int8_t *ar_fpage[MAX_PGCACHE_HOOK]; /* User data in page cache */
    struct mutex lock;          /* protection of page cache concurrent access */
    uint32_t preload_count;
    uint32_t preload_hit;
} spgcache_t;

extern int32_t pgcache_init(spgcache_t * pgc);
extern void pgcache_destroy(spgcache_t * pgc);

extern int32_t pgcache_update(spgcache_t * pgc, sector64 lpn,
                              struct bio_vec *pvec_src, bool isForceUpdate);
extern int32_t pgcache_load(spgcache_t * pgc, sector64 lpn,
                            struct bio_vec *pvec_dest);
#endif //PGCACHE_INC_198273917239817232
