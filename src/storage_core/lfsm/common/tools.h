/*
 * SPDX-License-Identifier: GPL-2.0-or-later OR Apache-2.0
 * Copyright (C) 2015-2020 Industrial Technology Research Institute
 *
 *
 *
 *  
 */
#ifndef TOOLS_INC_10928309128390812
#define TOOLS_INC_10928309128390812

static inline uint64_t do_divide(uint64_t dividend, uint64_t divisor)
{
    if (divisor <= 0) {
        return 0;
    }

    return dividend / divisor;
}

extern uint32_t ecompute_bits(uint32_t src);
extern uint32_t swbits(uint32_t src, int32_t dir);
extern uint64_t swbitslng(uint64_t src, int32_t dir);
extern bool list_empty_check(const struct list_head *head);
extern void dumphex_sector(int8_t * data, int32_t len);
extern bool is2order(uint32_t v);
#endif
