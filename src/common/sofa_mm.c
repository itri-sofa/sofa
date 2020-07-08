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

#ifndef SOFA_USER_SPACE
#include <linux/types.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/swap.h>
#include <linux/vmalloc.h>
#include "sofa_common.h"
#include "sofa_mm_private.h"
#include "sofa_mm.h"

#else
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <syslog.h>
#include "sofa_mm_private.h"
#include "sofa_mm.h"

#endif

#ifndef SOFA_USER_SPACE
struct page *sofa_alloc_page(gfp_t m_flags)
{
    struct page *mypage;

    mypage = alloc_page(m_flags);

    if (unlikely(IS_ERR_OR_NULL(mypage))) {
        sofa_printk(LOG_WARN, "WARN alloc_page err ret %p\n", mypage);
        return NULL;
    }

    atomic_inc(&cnt_pages);

    return mypage;
}

void sofa_free_page(struct page *mypage)
{
    if (unlikely(IS_ERR_OR_NULL(mypage))) {
        sofa_printk(LOG_WARN, "WARN try free a NULL page\n");
        SOFA_WARN_ON(true);
        return;
    }

    __free_page(mypage);
    atomic_dec(&cnt_pages);
}

void *sofa_kmalloc(size_t m_size, gfp_t m_flags)
{
    void *ret;

    ret = kzalloc(m_size, m_flags);

    do {
        if (unlikely(IS_ERR_OR_NULL(ret))) {
            sofa_printk(LOG_WARN,
                        "WARN kmalloc err ret: %p size: %ld free pages: %llu\n",
                        ret, m_size, (uint64_t) nr_free_pages());
            SOFA_WARN_ON(true);
            ret = NULL;
            break;
        }
        //memset(ret, 0, m_size);
        atomic_inc(&cnt_kmalloc);
    } while (0);

    return ret;
}

void sofa_kfree(void *ptr)
{
    if (unlikely(IS_ERR_OR_NULL(ptr))) {
        sofa_printk(LOG_WARN, "WARN try kfree a NULL memory space\n");
        SOFA_WARN_ON(true);
        return;
    }

    kfree(ptr);
    atomic_dec(&cnt_kmalloc);
}

void *sofa_vmalloc(size_t m_size)
{
    void *ret;

    ret = vmalloc(m_size);

    do {
        if (unlikely(IS_ERR_OR_NULL(ret))) {
            sofa_printk(LOG_WARN,
                        "WARN vmalloc err ret: %p size: %ld free pages: %llu\n",
                        ret, m_size, (uint64_t) nr_free_pages());
            SOFA_WARN_ON(true);
            ret = NULL;
            break;
        }

        memset(ret, 0, m_size);
        atomic_inc(&cnt_vmalloc);
    } while (0);

    return ret;
}

void sofa_vfree(void *ptr)
{
    if (unlikely(IS_ERR_OR_NULL(ptr))) {
        sofa_printk(LOG_WARN, "WARN try vfree a NULL memory space\n");
        SOFA_WARN_ON(true);
        return;
    }

    vfree(ptr);
    atomic_dec(&cnt_vmalloc);
}
#else

void *sofa_malloc(size_t m_size)
{
    void *ret;

    ret = malloc(m_size);

    if (NULL == ret) {
        syslog(LOG_ERR, "SOFA MM WARN fail alloc mem\n");
    } else {
        cnt_malloc++;
    }

    return ret;
}

void sofa_free(void *ptr)
{
    if (NULL != ptr) {
        free(ptr);
        cnt_malloc--;
    }
}

#endif

void init_sofa_mm(void)
{
#ifndef SOFA_USER_SPACE
    atomic_set(&cnt_kmalloc, 0);
    atomic_set(&cnt_vmalloc, 0);
    atomic_set(&cnt_pages, 0);
#else
    cnt_malloc = 0;
#endif
}

void rel_sofa_mm(void)
{
#ifndef SOFA_USER_SPACE
    int32_t cnt;

    cnt = atomic_read(&cnt_kmalloc);
    if (0 != cnt) {
        sofa_printk(LOG_ERR, "ERR: mem leak of kmalloc %d (!=0)\n", cnt);
        SOFA_WARN_ON(true);
    }

    cnt = atomic_read(&cnt_vmalloc);
    if (0 != cnt) {
        sofa_printk(LOG_ERR, "ERR: mem leak of vmalloc %d (!=0)\n", cnt);
        SOFA_WARN_ON(true);
    }

    cnt = atomic_read(&cnt_pages);
    if (0 != cnt) {
        sofa_printk(LOG_ERR, "ERR: mem leak of alloc_page %d (!=0)\n", cnt);
        SOFA_WARN_ON(true);
    }
#else

    if (0 != cnt_malloc) {
        syslog(LOG_ERR, "[SOFA] MM ERROR mem leak of malloc %d (!=0)\n",
               cnt_malloc);
    }
#endif
}
