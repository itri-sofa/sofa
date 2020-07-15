/*
 * Copyright (c) 2015-2020 Industrial Technology Research Institute.
 *
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 *
 *
 *
 *
 *
 *
 *  
 */

#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/sched.h>
#include <linux/preempt.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/percpu.h>
#include <linux/slab.h>
#include <linux/notifier.h>
#include <linux/cpu.h>
#include <linux/gfp.h>
#include <linux/string.h>
#include <linux/bitops.h>

#include "sofa_radix_tree.h"

struct sofa_radix_tree_path {
    sofa_radix_tree_node_t *node;
    int32_t offset;
};
typedef struct sofa_radix_tree_path sofa_radix_tree_path_t;

//#define RADIX_TREE_INDEX_BITS  (8 /* CHAR_BIT */ * sizeof(uint64_t))
//#define RADIX_TREE_MAX_PATH (RADIX_TREE_INDEX_BITS/_RADIX_TREE_MAP_SHIFT + 2)

static uint64_t height_to_maxindex[RADIX_TREE_MAX_PATH] __read_mostly;

/*
 * Radix tree node cache.
 */
typedef struct kmem_cache kmem_cache_t;
static kmem_cache_t *radix_tree_node_cachep;

/*
 * Per-cpu pool of preloaded nodes
 */
struct radix_tree_preload {
    int32_t nr;
    struct sofa_radix_tree_node *nodes[RADIX_TREE_MAX_PATH];
};
DEFINE_PER_CPU(struct radix_tree_preload, radix_tree_preloads) = { 0, };

static gfp_t root_gfp_mask(sofa_radix_tree_root_t * root)
{
    return root->gfp_mask & __GFP_BITS_MASK;
}

//static atomic_t node_count;
atomic_t sofa_node_count;

/*
 * This assumes that the caller has performed appropriate preallocation, and
 * that the caller has pinned this thread of control to the current CPU.
 */
static sofa_radix_tree_node_t *radix_tree_node_alloc(sofa_radix_tree_root_t *
                                                     root)
{
    int32_t i;
    sofa_radix_tree_node_t *ret;
    struct radix_tree_preload *rtp;

    gfp_t gfp_mask = root_gfp_mask(root);

    ret = kmem_cache_alloc(radix_tree_node_cachep, gfp_mask);

    if (ret == NULL && !(gfp_mask & GFP_KERNEL)) {
        rtp = &get_cpu_var(radix_tree_preloads);

        if (rtp->nr) {
            ret = rtp->nodes[rtp->nr - 1];
            rtp->nodes[rtp->nr - 1] = NULL;
            rtp->nr--;
        }
    }

    if (ret != NULL) {
        for (i = 0; i < _RADIX_TREE_MAP_SIZE; i++) {
            ret->slots[i] = NULL;
        }

        ret->count = 0;
        //memset(ret, 0, sizeof(struct radix_tree_node));
        atomic_inc(&sofa_node_count);
    }

    return ret;
}

static void radix_tree_node_free(sofa_radix_tree_node_t * node)
{
    if (node != NULL) {
        atomic_dec(&sofa_node_count);
        kmem_cache_free(radix_tree_node_cachep, node);
    }
}

/*
 * Load up this CPU's radix_tree_node buffer with sufficient objects to
 * ensure that the addition of a single element in the tree cannot fail.  On
 * success, return zero, with preemption disabled.  On error, return -ENOMEM
 * with preemption not disabled.
 */
/*static int radix_tree_preload (gfp_t gfp_mask)
{
    struct radix_tree_preload *rtp;
    sofa_radix_tree_node_t *node;
    int ret = -ENOMEM;

    preempt_disable();
    rtp = &__get_cpu_var(radix_tree_preloads);

    while (rtp->nr < ARRAY_SIZE(rtp->nodes)) {
        preempt_enable();
        node = kmem_cache_alloc(radix_tree_node_cachep, gfp_mask);

        if (node == NULL) {
            goto out;
        }

        preempt_disable();
        rtp = &__get_cpu_var(radix_tree_preloads);

        if (rtp->nr < ARRAY_SIZE(rtp->nodes)) {
            rtp->nodes[rtp->nr++] = node;
        } else {
            kmem_cache_free(radix_tree_node_cachep, node);
        }
    }

    ret = 0;
out:
    return ret;
}*/

/*
 *  Return the maximum key which can be store into a
 *  radix tree with height HEIGHT.
 */
static uint64_t radix_tree_maxindex(uint32_t height)
{
    return height_to_maxindex[height];
}

/*
 *  Extend a radix tree so it can store key @index.
 */
static int radix_tree_extend(sofa_radix_tree_root_t * root, uint64_t index)
{
    sofa_radix_tree_node_t *node;
    uint32_t height;

    /* Figure out what the height should be.  */
    height = root->height + 1;

    while (index > radix_tree_maxindex(height)) {
        height++;
    }

    if (root->rnode == NULL) {
        root->height = height;
        goto out;
    }

    do {
        if (!(node = radix_tree_node_alloc(root))) {
            return -ENOMEM;
        }

        /* Increase the height.  */
        node->slots[0] = root->rnode;

        node->count = 1;
        root->rnode = node;
        root->height++;
    } while (height > root->height);

  out:
    return 0;
}

/**
 *  radix_tree_insert    -    insert into a radix tree
 *  @root:      radix tree root
 *  @index:     index key
 *  @item:      item to insert
 *
 *  Insert an item into the radix tree at position @index.
 */
int32_t sofa_radix_tree_insert(sofa_radix_tree_root_t * root, uint64_t index,
                               void *item, void **item_replaced)
{
    sofa_radix_tree_node_t *node = NULL, *slot;
    uint32_t height, shift;
    int32_t offset, error;

    /* Make sure the tree is high enough.  */
    if (index > radix_tree_maxindex(root->height)) {
        error = radix_tree_extend(root, index);

        if (error) {
            return error;
        }
    }

    slot = root->rnode;
    height = root->height;
    shift = (height - 1) * _RADIX_TREE_MAP_SHIFT;

    offset = 0;                 /* uninitialised var warning */

    while (height > 0) {
        if (slot == NULL) {
            /* Have to add a child node.  */
            if (!(slot = radix_tree_node_alloc(root))) {
                return -ENOMEM;
            }

            if (node) {
                node->slots[offset] = slot;
                //rcu_assign_pointer(node->slots[offset], slot); //Lego 20130329 : New version using this
                node->count++;
            } else {
                root->rnode = slot;
            }

            //rcu_assign_pointer(root->rnode, slot); //Lego 20130329 : New version using this
        }

        /* Go a level down */
        offset = (index >> shift) & _RADIX_TREE_MAP_MASK;
        node = slot;
        slot = node->slots[offset];
        shift -= _RADIX_TREE_MAP_SHIFT;
        height--;
    }

    if (slot != NULL) {
        if (item_replaced != NULL) {
            *item_replaced = slot;
        }
    }

    if (node) {
        if (slot == NULL) {
            node->count++;
        }

        node->slots[offset] = item;
        //rcu_assign_pointer(node->slots[offset], item); //Lego 20130329 : New version using this
    } else {
        root->rnode = item;
        //rcu_assign_pointer(root->rnode, radix_tree_ptr_to_direct(item)); //Lego 20130329 : New version using this
    }

    return 0;
}

/*
static void **__lookup_slot (sofa_radix_tree_root_t *root, uint64_t index)
{
    uint32_t height, shift;
    sofa_radix_tree_node_t **slot;

    height = root->height;

    if (index > radix_tree_maxindex(height)) {
        return NULL;
    }

    if (height == 0 && root->rnode) {
        return (void **)&root->rnode;
    }

    shift = (height - 1) * _RADIX_TREE_MAP_SHIFT;
    slot = &root->rnode;

    while (height > 0) {
        if (*slot == NULL) {
            return NULL;
        }

        slot = (sofa_radix_tree_node_t **)
               ((*slot)->slots +
               ((index >> shift) & _RADIX_TREE_MAP_MASK));
        shift -= _RADIX_TREE_MAP_SHIFT;
        height--;
    }

    return (void **)slot;
}*/

/**
 *  radix_tree_lookup_slot    -    lookup a slot in a radix tree
 *  @root:      radix tree root
 *  @index:     index key
 *
 *  Lookup the slot corresponding to the position @index in the radix tree
 *  @root. This is useful for update-if-exists operations.
 */
/*static void **radix_tree_lookup_slot (sofa_radix_tree_root_t *root,
                               uint64_t index)
{
    return __lookup_slot(root, index);
}
*/
/**
 *  radix_tree_lookup    -    perform lookup operation on a radix tree
 *  @root:      radix tree root
 *  @index:     index key
 *
 *  Lookup the item at the position @index in the radix tree @root.
 */
/*static void *radix_tree_lookup (sofa_radix_tree_root_t *root,
                         uint64_t index)
{
    void **slot;

    slot = __lookup_slot(root, index);
    return slot != NULL ? *slot : NULL;
}
*/
/*static void radix_tree_dump (sofa_radix_tree_node_t *node, int height)
{
    unsigned long i;

    if (!node) {
        return;
    }

    printk(KERN_ERR"count %4d\n", node->count);

    for (i = 0; i < _RADIX_TREE_MAP_SIZE; ++i) {
        if (node->slots[i]) {
            if (height == 1) {
                printk(KERN_ERR "leaf %2ld %p\n", i, node->slots[i]);
            } else {
                printk(KERN_ERR "node %2d %p\n", height, node->slots[i]);
                radix_tree_dump(node->slots[i], height - 1);
            }
        }
    }
}*/

static uint32_t __lookup(sofa_radix_tree_root_t * root, void **results,
                         uint64_t index, uint32_t max_items,
                         uint64_t * next_index)
{
    uint32_t nr_found = 0;
    uint32_t shift, height;
    sofa_radix_tree_node_t *slot;
    uint64_t i;

    height = root->height;

    if (height == 0) {
        if (root->rnode && index == 0) {
            results[nr_found++] = root->rnode;
        }

        goto out;
    }

    shift = (height - 1) * _RADIX_TREE_MAP_SHIFT;
    slot = root->rnode;

    for (; height > 1; height--) {

        for (i = (index >> shift) & _RADIX_TREE_MAP_MASK;
             i < _RADIX_TREE_MAP_SIZE; i++) {
            if (slot->slots[i] != NULL) {
                break;
            }

            index &= ~((1ULL << shift) - 1);
            index += 1ULL << shift;

            if (index == 0) {
                goto out;       /* 32-bit wraparound */
            }
        }

        if (i == _RADIX_TREE_MAP_SIZE) {
            goto out;
        }

        shift -= _RADIX_TREE_MAP_SHIFT;
        slot = slot->slots[i];
    }

    /* Bottom level: grab some items */
    for (i = index & _RADIX_TREE_MAP_MASK; i < _RADIX_TREE_MAP_SIZE; i++) {
        index++;

        if (slot->slots[i]) {
            results[nr_found++] = slot->slots[i];

            if (nr_found == max_items) {
                goto out;
            }
        }
    }

  out:
    *next_index = index;
    return nr_found;
}

/**
 *  radix_tree_gang_lookup - perform multiple lookup on a radix tree
 *  @root:      radix tree root
 *  @results:   where the results of the lookup are placed
 *  @first_index:   start the lookup from this key
 *  @max_items: place up to this many items at *results
 *
 *  Performs an index-ascending scan of the tree for present items.  Places
 *  them at *@results and returns the number of items which were placed at
 *  *@results.
 *
 *  The implementation is naive.
 */
uint32_t sofa_radix_tree_gang_lookup(sofa_radix_tree_root_t * root,
                                     void **results, uint64_t first_index,
                                     uint32_t max_items)
{
    const uint64_t max_index = radix_tree_maxindex(root->height);
    uint64_t cur_index = first_index;
    uint32_t ret = 0;

    while (ret < max_items) {
        uint32_t nr_found;
        uint64_t next_index;    /* Index of next search */

        if (cur_index > max_index) {
            break;
        }

        nr_found =
            __lookup(root, results + ret, cur_index, max_items - ret,
                     &next_index);
        ret += nr_found;

        if (next_index == 0) {
            break;
        }

        cur_index = next_index;
    }

    return ret;
}

/**
 *  radix_tree_shrink    -    shrink height of a radix tree to minimal
 *  @root       radix tree root
 */
static void radix_tree_shrink(sofa_radix_tree_root_t * root)
{
    /* try to shrink tree height */
    while (root->height > 0 && root->rnode->count == 1 && root->rnode->slots[0]) {
        sofa_radix_tree_node_t *to_free = root->rnode;

        root->rnode = to_free->slots[0];
        root->height--;
        /* must only free zeroed nodes into the slab */
        to_free->slots[0] = NULL;
        to_free->count = 0;
        radix_tree_node_free(to_free);
    }
}

/**
 *  radix_tree_delete    -    delete an item from a radix tree
 *  @root:      radix tree root
 *  @index:     index key
 *
 *  Remove the item at @index from the radix tree rooted at @root.
 *
 *  Returns the address of the deleted item, or NULL if it was not present.
 */
void *sofa_radix_tree_delete(sofa_radix_tree_root_t * root, uint64_t index)
{
    sofa_radix_tree_path_t path[RADIX_TREE_MAX_PATH], *pathp = path;
    sofa_radix_tree_node_t *slot = NULL;
    unsigned int height, shift;
    int32_t offset;

    height = root->height;

    if (index > radix_tree_maxindex(height)) {
        goto out;
    }

    slot = root->rnode;

    if (height == 0 && root->rnode) {
        root->rnode = NULL;
        goto out;
    }

    shift = (height - 1) * _RADIX_TREE_MAP_SHIFT;
    pathp->node = NULL;

    do {
        if (slot == NULL) {
            goto out;
        }

        pathp++;
        offset = (index >> shift) & _RADIX_TREE_MAP_MASK;
        pathp->offset = offset;
        pathp->node = slot;
        slot = slot->slots[offset];
        shift -= _RADIX_TREE_MAP_SHIFT;
        height--;
    } while (height > 0);

    if (slot == NULL) {
        goto out;
    }

    /* Now free the nodes we do not need anymore */
    while (pathp->node) {
        pathp->node->slots[pathp->offset] = NULL;
        pathp->node->count--;

        if (pathp->node->count) {
            if (pathp->node == root->rnode) {
                radix_tree_shrink(root);
            }

            goto out;
        }

        radix_tree_node_free(pathp->node);
        pathp--;
    }

    root->height = 0;
    root->rnode = NULL;
  out:
    return slot;
}

static void radix_tree_node_ctor(void *node)
{
    memset(node, 0, sizeof(sofa_radix_tree_node_t));
}

struct kmem_cache *sofa_kmem_cache_create_common(const char *name, size_t size,
                                                 size_t align,
                                                 unsigned long flags,
                                                 void (*ctor)(void *),
                                                 void (*dtor)(void *))
{
    return kmem_cache_create(name, size, align, flags, ctor);
}

static uint64_t __maxindex(uint32_t height)
{
    uint32_t tmp = height * _RADIX_TREE_MAP_SHIFT;
    uint64_t index = (~0ULL >> (RADIX_TREE_INDEX_BITS - tmp - 1)) >> 1;

    if (tmp >= RADIX_TREE_INDEX_BITS) {
        index = ~0ULL;
    }

    return index;
}

static void radix_tree_init_maxindex(void)
{
    uint32_t i;

    for (i = 0; i < ARRAY_SIZE(height_to_maxindex); i++) {
        height_to_maxindex[i] = __maxindex(i);
    }
}

void sofa_radix_tree_init(void)
{
    radix_tree_node_cachep =
        sofa_kmem_cache_create_common("SOFA/radix_tree_node",
                                      sizeof(sofa_radix_tree_node_t), 0,
                                      SLAB_PANIC, radix_tree_node_ctor, NULL);
    radix_tree_init_maxindex();
    atomic_set(&sofa_node_count, 0);
    //hotcpu_notifier(radix_tree_callback, 0);
}

void sofa_radix_tree_exit(void)
{
    struct radix_tree_preload *rtp;
    int32_t cpu, i;

    printk(KERN_ERR "sofa_node_count %d\n", atomic_read(&sofa_node_count));

    if (!radix_tree_node_cachep) {
        return;
    }

    for_each_present_cpu(cpu) {
        rtp = &per_cpu(radix_tree_preloads, cpu);
        printk(KERN_ERR "%s: %d per_cpu %lx\n", __func__, cpu,
               (unsigned long)rtp);
        printk(KERN_ERR "nr %d\n", rtp->nr);

        for (i = 0; i < rtp->nr; ++i) {
            kmem_cache_free(radix_tree_node_cachep, rtp->nodes[i]);
        }
    }
    kmem_cache_destroy(radix_tree_node_cachep);
    //remove_hotcpu_notifier(radix_tree_callback, 0);
}
