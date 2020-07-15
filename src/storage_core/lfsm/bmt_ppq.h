/*
 * SPDX-License-Identifier: GPL-2.0-or-later OR Apache-2.0
 * Copyright (C) 2015-2020 Industrial Technology Research Institute
 *
 *
 *
 *  
 */
#define PPQ_CACHE_T_HIGH 10000  //start to do the cache swap out
#define PPQ_CACHE_T_LOW  8000   //
#define PPQ_CACHE_T_DIFF PPQ_CACHE_T_HIGH-PPQ_CACHE_T_LOW
#define PBNO_SPECIAL_NOT_IN_CACHE 9
#define PBNO_SPECIAL_BAD_PAGE 8

#define PBNO_SPECIAL_NEVERWRITE 0

#define ABMTE_PENDING 1
#define ABMTE_NONPENDING 0

struct A_BMT_E {
    sector64 lbno;
    sector64 pbno;
    int run_length;             //max = 8*512B/4(sizeof(BMT)) = 824
    struct list_head ppq_abmte;
    unsigned short pending;     // 1:pending A_BMT_E 0:non_pending A_BMT_E
};
