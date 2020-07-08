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
