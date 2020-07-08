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
