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
#include <linux/slab.h>         // kmalloc
#include <linux/spinlock.h>     // spinlock
#include <linux/bio.h>
#include "common.h"
#include "mem_manager.h"
#include "mem_manager_private.h"

/*
 * Description: initialize memory manager module
 */
int32_t init_memory_manager(uint32_t bio_pool_size)
{
    int32_t i, ret;

    memset(&memMgr, 0, sizeof(mem_manager_t));

    LOGINFO("initialize memory manager with bioset size %u\n", bio_pool_size);
    if ((ret =
         bioset_init(&memMgr.bioset, bio_pool_size, 0,
                     BIOSET_NEED_BVECS)) != 0) {
        LOGERR("memory manager fail alloc bioset ret = %d\n", ret);
        return ret;
    }

    ret = 0;
    for (i = 0; i < CN_MEMTRACK_TAG; i++) {
        atomic64_set(&memMgr.memory_consumed[i], 0);
    }

    atomic64_set(&memMgr.page_consumed, 0);
    atomic64_set(&memMgr.bio_consumed, 0);
    atomic64_set(&memMgr.total, 0);
    atomic64_set(&memMgr.lastshow, GAP_MEM_MONITOR);

    return ret;
}

int32_t dump_memory_usage(int8_t * buffer, int32_t max_buffer_size)
{
#ifdef ENABLE_MEM_TRACKER
    int32_t i;
#endif
    int32_t len;

    len = 0;

#ifdef ENABLE_MEM_TRACKER
    len += sprintf(buffer + len, "General Memory Usage:\n");
    for (i = 0; i < CN_MEMTRACK_TAG; i++) {
        sprintf(buffer + len, "[%15s]: %8lld bytes\n",
                nm_memcost_type[i],
                (int64_t) atomic64_read(&memMgr.memory_consumed[i]));

        if (max_buffer_size - 80 > len) {
            break;
        }
    }
#else
    len +=
        sprintf(buffer + len,
                "General Memory Usage: not enable memory tracker now\n");
#endif

    return len;
}

void print_memory_usage(void)
{
    int32_t i;
    printk("-------------------------------------\n");
    printk("Memory consumption profile (in KBytes)\n");
    printk("-------------------------------------\n");

    for (i = 0; i < CN_MEMTRACK_TAG; i++) {
        LOGINFO("id %2d [%20s]: %20lld bytes\n", i,
                nm_memcost_type[i],
                (int64_t) atomic64_read(&memMgr.memory_consumed[i]));
    }
    printk("Total memory usaged :%lld KB\n",
           (int64_t) atomic64_read(&memMgr.total) >> 10);
    printk("-------------------------------------\n");
    printk("misc consumption profile\n");
    printk("-------------------------------------\n");
    LOGINFO("# of Memory Page: %lld\n",
            (int64_t) atomic64_read(&memMgr.page_consumed));
    LOGINFO("# of bio: %lld\n", (int64_t) atomic64_read(&memMgr.bio_consumed));
}

int32_t rel_memory_manager(void)
{
    LOGINFO("release memory manager\n");
    bioset_exit(&memMgr.bioset);

    print_memory_usage();

    return 0;
}

struct page *track_alloc_page(gfp_t flags)
{
    struct page *ret;

    ret = alloc_page(flags);

#ifdef ENABLE_MEM_TRACKER
    if (ret) {
        atomic64_inc(&memMgr.page_consumed);
    }
#endif

    return ret;
}

void track_free_page(struct page *page)
{
    __free_page(page);

#ifdef ENABLE_MEM_TRACKER
    atomic64_dec(&memMgr.page_consumed);
#endif
}

void *track_kmalloc(size_t size, gfp_t flags, int32_t section)
{
    void *ret;
#ifdef ENABLE_MEM_TRACKER
    uint64_t total_size;
#endif

    ret = kmalloc(size, flags);

#ifdef ENABLE_MEM_TRACKER
    if (ret) {
        atomic64_add(size, &memMgr.memory_consumed[section]);
        total_size = atomic64_add_return(size, &memMgr.total);

        if (total_size > atomic64_read(&memMgr.lastshow)) {
            LOGINFO("kmalloc cost > %lld B\n", total_size);
            atomic64_set(&memMgr.lastshow, (total_size + GAP_MEM_MONITOR));
        }
    }
#endif

    return ret;
}

void *track_vmalloc(size_t size, gfp_t flags, int32_t section)
{
    void *ret;
#ifdef ENABLE_MEM_TRACKER
    uint64_t total_size;
#endif

    ret = vmalloc(size);

#ifdef ENABLE_MEM_TRACKER
    if (ret) {
        atomic64_add(size, &memMgr.memory_consumed[section]);
        total_size = atomic64_add_return(size, &memMgr.total);

        if (total_size > atomic64_read(&memMgr.lastshow)) {
            LOGERR("vmalloc cost > %lld\n", total_size);
            atomic64_set(&memMgr.lastshow, (total_size + GAP_MEM_MONITOR)); //10MB
        }
    }
#endif

    return ret;
}

/*
** This is a wrapper function for kfree. Used to keep track of the memory consumption by LFSM.
**
** @ptr     : the pointer to be freed.
** @section    : Which section of the code is requesting the memory.
**
** Returns always 1
*/
void track_kfree(void *ptr, int32_t size, int32_t section)
{
    kfree(ptr);
    //ptr = NULL; //Lego: comment this, pass ptr value, caller still has ptr value, save 1 CPU instruction

#ifdef ENABLE_MEM_TRACKER
    atomic64_sub(size, &memMgr.memory_consumed[section]);
    atomic64_sub(size, &memMgr.total);
#endif
}

void track_vfree(void *ptr, int32_t size, int32_t section)
{
    vfree(ptr);
    //ptr = NULL;

#ifdef ENABLE_MEM_TRACKER
    atomic64_sub(size, &memMgr.memory_consumed[section]);
    atomic64_sub(size, &memMgr.total);
#endif
}

struct bio *track_bio_alloc(gfp_t flags, int32_t vecs)
{
    struct bio *ret;

    ret = bio_alloc_bioset(flags, vecs, &memMgr.bioset);
    if (ret == NULL) {
        return NULL;
    }

    ret->bi_pool = &memMgr.bioset;

#ifdef ENABLE_MEM_TRACKER
    atomic64_inc(&memMgr.bio_consumed);
#endif

    return ret;
}

void track_bio_put(struct bio *bio)
{
    if (atomic_read(&bio->__bi_cnt) != 1) {
        dump_stack();
        LOGERR("bi_cnt = %d \n", atomic_read(&bio->__bi_cnt));
    }
    bio_put(bio);

#ifdef ENABLE_MEM_TRACKER
    atomic64_dec(&memMgr.bio_consumed);
#endif

    return;
}
