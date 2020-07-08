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
