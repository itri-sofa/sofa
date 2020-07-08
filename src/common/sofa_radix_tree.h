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

#ifndef _SOFA_RADIX_TREE_H_
#define _SOFA_RADIX_TREE_H_

struct kmem_cache *sofa_kmem_cache_create_common(const char *name, size_t size,
                                                 size_t align,
                                                 unsigned long flags,
                                                 void (*ctor)(void *),
                                                 void (*dtor)(void *));

#ifdef __KERNEL__
#define _RADIX_TREE_MAP_SHIFT    6
#else
#define _RADIX_TREE_MAP_SHIFT    3  /* For more stressful testing */
#endif

#define _RADIX_TREE_MAP_SIZE (1ULL << _RADIX_TREE_MAP_SHIFT)
#define _RADIX_TREE_MAP_MASK (_RADIX_TREE_MAP_SIZE-1)

struct sofa_radix_tree_node {
    uint32_t count;
    void *slots[_RADIX_TREE_MAP_SIZE];
};
typedef struct sofa_radix_tree_node sofa_radix_tree_node_t;

/* root tags are stored in gfp_mask, shifted by __GFP_BITS_SHIFT */
struct sofa_radix_tree_root {
    uint32_t height;
    gfp_t gfp_mask;
    struct sofa_radix_tree_node *rnode;
};
typedef struct sofa_radix_tree_root sofa_radix_tree_root_t;

#define SOFA_RADIX_TREE_INIT(mask)   {                   \
    .height = 0,                            \
    .gfp_mask = (mask),                     \
    .rnode = NULL,                          \
}

#define SOFA_RADIX_TREE(name, mask) \
    struct sofa_radix_tree_root name = DMS_RADIX_TREE_INIT(mask)

#define SOFA_INIT_RADIX_TREE(root, mask)                 \
do {                                    \
    (root)->height = 0;                     \
    (root)->gfp_mask = (mask);                  \
    (root)->rnode = NULL;                       \
} while (0)

extern int sofa_radix_tree_insert(sofa_radix_tree_root_t *, uint64_t, void *,
                                  void **);
//void *sofa_radix_tree_lookup(sofa_radix_tree_root_t *, unsigned long long);
//void **sofa_radix_tree_lookup_slot(sofa_radix_tree_root_t *, unsigned long long);
extern void *sofa_radix_tree_delete(sofa_radix_tree_root_t *, uint64_t);
extern uint32_t sofa_radix_tree_gang_lookup(sofa_radix_tree_root_t * root,
                                            void **results,
                                            uint64_t first_index,
                                            uint32_t max_items);
////int sofa_radix_tree_preload( gfp_t gfp_mask );
extern void sofa_radix_tree_init(void);
extern void sofa_radix_tree_exit(void);
//void sofa_radix_tree_dump(sofa_radix_tree_node_t *node, int height);

/*static inline void radix_tree_preload_end (void)
{
    preempt_enable();
}
*/

extern atomic_t sofa_node_count;

#endif /* _SOFA_RADIX_TREE_H_ */
