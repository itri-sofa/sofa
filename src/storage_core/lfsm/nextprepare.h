/*
 * SPDX-License-Identifier: GPL-2.0-or-later OR Apache-2.0
 * Copyright (C) 2015-2020 Industrial Technology Research Institute
 *
 *
 *
 *  
 */
#ifndef NEXTPREPARE_INC_09812038109283
#define NEXTPREPARE_INC_09812038109283

struct EUProperty;

typedef struct SNextPrepare {
    struct EUProperty *eu_main;
    struct EUProperty *eu_backup;
    atomic_t fg_next_dirty;
} SNextPrepare_t;

extern int32_t SNextPrepare_alloc(SNextPrepare_t * next, int32_t eu_usage);
extern void SNextPrepare_renew(SNextPrepare_t * pNext, int32_t usage);
extern void SNextPrepare_get_new(SNextPrepare_t * pnext,
                                 struct EUProperty **pret_peu_main,
                                 struct EUProperty **pret_peu_backup,
                                 int32_t eu_usage);
extern void SNextPrepare_put_old(SNextPrepare_t * pnext,
                                 struct EUProperty *peu_main,
                                 struct EUProperty *peu_backup);
extern void SNextPrepare_rollback(SNextPrepare_t * pnext,
                                  struct EUProperty *peu_main,
                                  struct EUProperty *peu_backup);

#endif //  NEXTPREPARE_INC_09812038109283
