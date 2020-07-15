/*
 * SPDX-License-Identifier: GPL-2.0-or-later OR Apache-2.0
 * Copyright (C) 2015-2020 Industrial Technology Research Institute
 *
 *
 *
 *  
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
