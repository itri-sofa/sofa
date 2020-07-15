/*
 * SPDX-License-Identifier: GPL-2.0-or-later OR Apache-2.0
 * Copyright (C) 2015-2020 Industrial Technology Research Institute
 *
 *
 *
 *  
 */
#ifndef CONFLICT_INC_1092830192839123
#define CONFLICT_INC_1092830192839123

#define CONFLICT_HIGH_INC_ORDER 16
#define CONFLICT_HIGH_INC (1<<CONFLICT_HIGH_INC_ORDER)
#define CONFLICT_LOW_MASK (CONFLICT_HIGH_INC-1)
#define CONFLICT_COUNTER_ZERO(CN,ID_PPQ) (((ID_PPQ)%MAX_CONFLICT_HOOK)? \
                (((CN)&CONFLICT_LOW_MASK)==0):((((CN)>>CONFLICT_HIGH_INC_ORDER)&CONFLICT_LOW_MASK)==0))
#define CONFLICT_COUNTER_ALLZERO(CN) (CN==0)
#define CONFLICT_COUNTER_INC_RETURN(PBIOC, ID_PPQ) atomic_add_return(((ID_PPQ)%MAX_CONFLICT_HOOK)?1:CONFLICT_HIGH_INC, &(PBIOC)->conflict_pages)
#define CONFLICT_COUNTER_DEC_RETURN(PBIOC, ID_PPQ) atomic_sub_return(((ID_PPQ)%MAX_CONFLICT_HOOK)?1:CONFLICT_HIGH_INC, &(PBIOC)->conflict_pages)

extern int32_t conflict_bioc_login(struct bio_container *pbioc, lfsm_dev_t * td,
                                   int32_t isOption);
extern int32_t conflict_bioc_logout(struct bio_container *pbioc,
                                    lfsm_dev_t * td);
extern int32_t conflict_gc_abort_helper(struct bio_container *bioc,
                                        lfsm_dev_t * td);

extern int32_t conflict_bioc_mark_waitSEU(struct bio_container *pbioc,
                                          lfsm_dev_t * td);
extern int32_t conflict_bioc_mark_nopending(struct bio_container *pbioc,
                                            lfsm_dev_t * td);
extern void conflict_vec_dumpall(lfsm_dev_t * td, int32_t cn);

#endif
