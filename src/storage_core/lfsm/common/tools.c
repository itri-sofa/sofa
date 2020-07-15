/*
 * SPDX-License-Identifier: GPL-2.0-or-later OR Apache-2.0
 * Copyright (C) 2015-2020 Industrial Technology Research Institute
 *
 *
 *
 *  
 */
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/list.h>
#include "tools.h"

static uint32_t get_sum_bits(uint32_t src)  // get_chksum_bits
{
    int32_t i, dest;
    uint8_t r;
    uint8_t *p;
    int32_t ar_shift[8] = { 12, 2, 28, 22, 26, 18, 3, 31 };

    dest = 0;
    r = 0;
    p = (int8_t *) & src;

    for (i = 0; i < 8; i++) {
        src &= ~(1 << ar_shift[i]);
    }

    for (i = 0; i < 4; i++) {
        r += p[i];
    }

    for (i = 0; i < 8; i++) {
        dest |= (((r & (1 << i)) > 0) ? 1 : 0) << ar_shift[i];
    }

    return dest;
}

uint32_t ecompute_bits(uint32_t src)    ////encrype_bits
{
    uint32_t encryped = swbits(src, 0);
    encryped += get_sum_bits(encryped);
    return encryped;
}

uint32_t swbits(uint32_t src, int32_t dir)  //swapbits
{
    int32_t i;
    uint32_t dest;
    int32_t swap[24] = { 19, 29, 5, 8, 13, 20, 4, 30,
        15, 7, 6, 24, 16, 11, 25, 27,
        10, 23, 9, 1, 0, 14, 17, 21
    };
    dest = 0;
    for (i = 0; i < 24; i++) {
        if (dir == 0) {
            if (src & (1 << i)) {
                dest += (1 << swap[i]);
            }
        } else {
            if (src & (1 << swap[i])) {
                dest += (1 << i);
            }
        }
    }

    return dest;
}

uint64_t swbitslng(uint64_t src, int32_t dir)   // swapbits_long
{
    int32_t swap[64] =
        { 43, 1, 2, 3, 11, 5, 6, 22, 27, 10, 9, 26, 15, 13, 14, 12,
        16, 49, 18, 19, 60, 21, 7, 23, 24, 25, 4, 38, 28, 29, 30, 54,
        63, 59, 34, 51, 35, 37, 8, 39, 40, 41, 42, 0, 44, 45, 46, 47,
        48, 17, 50, 36, 62, 53, 31, 55, 56, 57, 58, 33, 20, 61, 52, 32
    };
    uint64_t dest;
    int32_t i;

    dest = 0;
    for (i = 0; i < 64; i++) {
        if (dir == 0) {
            if (src & (1ULL << i)) {
                dest |= (1ULL << swap[i]);
            }
        } else {
            if (src & (1ULL << swap[i])) {
                dest |= (1ULL << i);
            }
        }
    }

    return dest;
}

bool list_empty_check(const struct list_head *head)
{
    if ((NULL == head) || (NULL == head->next)) {
        return true;
    }

    return list_empty(head);
}

void dumphex_sector(int8_t * data, int32_t len)
{
    int32_t i = 0;

    //printk("\n-------------------------\n");
    //printk("HEX DUMP OF SECTOR memaddr=%p\n",data);
    //printk("-------------------------\n");
    while (i < len) {
        printk("0x%.2x.", (uint8_t) data[i]);
        i++;
        if (i % 16 == 0) {
            printk("\n");
        }
    }
    printk("\n");
}

bool is2order(uint32_t v)
{
    int32_t cn_bit1, i;

    cn_bit1 = 0;
    for (i = 0; i < (sizeof(v) * 8); i++) {
        if (test_bit(i, (void *)&v)) {
            cn_bit1++;
        }
    }

    if (cn_bit1 == 1) {
        return true;
    }

    return false;
}
