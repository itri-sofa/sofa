/*
 * SPDX-License-Identifier: GPL-2.0-or-later OR Apache-2.0
 * Copyright (C) 2015-2020 Industrial Technology Research Institute
 *
 *
 *
 *  
 */

#ifndef TRACKER_INC_01982309182093
#define TRACKER_INC_01982309182093

/* 
** Flags denoting the sections 
*/
typedef enum {
    M_BMT_CACHE = 0,
    M_GC_HEAP,
    M_GC_BITMAP,
    M_IO,
    M_GC,
    M_DEP,
    M_BMT,
    M_OTHER,
    M_BIOBOX_POOL,
    M_HASHPOOL,
    M_UNKNOWN,
    M_RECOVER,
    M_XORBMP,
    M_METALOG,
    M_SUPERMAP,
    M_DDMAP,
    M_DISKGRP,
    M_IOCTL,
    M_PGROUP,
    M_PERF_MON,
    CN_MEMTRACK_TAG,
} MEM_ALLOC_COMPT_T;

#define GAP_MEM_MONITOR (1024*1024*1024)

extern int32_t dump_memory_usage(int8_t * buffer, int32_t max_buffer_size);
extern void print_memory_usage(void);
extern int32_t init_memory_manager(uint32_t bio_pool_size);
extern int32_t rel_memory_manager(void);

extern struct page *track_alloc_page(gfp_t);
extern void track_free_page(struct page *page);

extern void *track_kmalloc(size_t, gfp_t, int32_t);
extern void track_kfree(void *, int32_t, int32_t);

extern void *track_vmalloc(size_t, gfp_t, int32_t);
extern void track_vfree(void *, int32_t, int32_t);

extern struct bio *track_bio_alloc(gfp_t flags, int32_t vecs);
extern void track_bio_put(struct bio *bio);
#endif
