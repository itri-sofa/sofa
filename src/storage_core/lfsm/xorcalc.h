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
#ifndef XORBLK_INC_917239172398172389
#define XORBLK_INC_917239172398172389

#include <linux/async_tx.h>
#include <linux/module.h>
#include <linux/gfp.h>
#include <linux/raid/xor.h>
#include <linux/jiffies.h>

#if KERNEL_XOR == 1
#include <asm/xor.h>
#endif

extern int32_t xorcalc_outside_stripe_lock(struct bio_container *ecc_bioc);

static inline void lfsm_xor_block(u8 * dest, u8 * src, size_t len)
{
    int32_t i;
    uint64_t *d = (uint64_t *) dest;
    uint64_t *s = (uint64_t *) src;
    len >>= 3;
    for (i = 0; i < len; i++) {
        d[i] ^= s[i];
    }
}

static inline void xorcalc_xor_blocks(uint32_t src_count, uint32_t bytes,
                                      void *dest, void **srcs)
{
    int32_t cn_res_pages, cn_done_pages, cn_curr_pages, xor_batch_number;

#if KERNEL_XOR == 1
    xor_batch_number = 4;
#else
    xor_batch_number = 1;
#endif

    cn_res_pages = src_count;
    cn_done_pages = 0;
    cn_curr_pages = 0;
    while (cn_res_pages > 0) {
        if (cn_res_pages > xor_batch_number) {
            cn_curr_pages = xor_batch_number;
        } else {
            cn_curr_pages = cn_res_pages;
        }

#if KERNEL_XOR == 1
        xor_blocks(cn_curr_pages, PAGE_SIZE, dest, &(srcs[cn_done_pages])); // no return value
#else
        lfsm_xor_block(dest, srcs[cn_done_pages], PAGE_SIZE);
#endif
        cn_res_pages -= cn_curr_pages;
        cn_done_pages += cn_curr_pages;
    }

    return;
}
#endif
