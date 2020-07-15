/*
 * SPDX-License-Identifier: GPL-2.0-or-later OR Apache-2.0
 * Copyright (C) 2015-2020 Industrial Technology Research Institute
 *
 *
 *
 *  
 */
#ifndef XORBMT_INC_917239172398172389
#define XORBMT_INC_917239172398172389

typedef struct sxorbmp {
    uint8_t *vector;
    struct mutex lock;
} sxorbmp_t;

extern void xorbmp_init(sxorbmp_t * xb);
extern void xorbmp_free(sxorbmp_t * xb);
extern void xorbmp_destroy(sxorbmp_t * xb);
extern int32_t xorbmp_get(struct EUProperty *eu, int32_t idx_fpage);

#endif
