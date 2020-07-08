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
