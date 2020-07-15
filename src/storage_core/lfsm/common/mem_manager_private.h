/*
 * SPDX-License-Identifier: GPL-2.0-or-later OR Apache-2.0
 * Copyright (C) 2015-2020 Industrial Technology Research Institute
 *
 *
 *
 *  
 */

#ifndef LFSM_TRACKER_PRIVATE_H_
#define LFSM_TRACKER_PRIVATE_H_

//#define ENABLE_MEM_TRACKER

/*
** Memory consumption profiling structure
*/
typedef struct mem_mgr {
    struct bio_set bioset;

    /*
     * counting the memory consumption (kmalloc, vmalloc ) of each components
     * unit : bytes
     */
    atomic64_t memory_consumed[CN_MEMTRACK_TAG];

    atomic64_t page_consumed;   /* counting the page consumption */
    atomic64_t bio_consumed;    /* counting the bio consumption */
    atomic64_t total;           /* memory consumption of kmalloc + vmalloc */
    atomic64_t lastshow;        /* record last max # memory consumption */
} mem_manager_t;

mem_manager_t memMgr;

/*
** This function should/could be used as an ioctl to print out the memory consumption details.
**
** Returns always 1
*/
int8_t *nm_memcost_type[CN_MEMTRACK_TAG] = {
    "M_BMT_CACHE", "M_GC_HEAP",
    "M_GC_BITMAP", "M_IO", "M_GC",
    "M_DEP", "M_BMT", "M_OTHER", "M_BIOBOX_POOL",
    "M_HASHPOOL", "M_UNKNOWN", "M_RECOVER",
    "M_XORBMP", "M_METALOG", "M_SUPERMAP",
    "M_DDMAP", "M_DISKGRP", "M_IOCTL",
    "M_PGROUP", "M_PERF_MON"
};

#endif /* LFSM_TRACKER_PRIVATE_H_ */
