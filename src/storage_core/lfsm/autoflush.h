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
#ifndef __HYBRID_H
#define __HYBRID_H

#include "bmt.h"

typedef struct sflushops {
    int8_t *name;
    void *variables;
    bool (*init)(struct lfsm_dev_struct * td, struct sflushops * afops,
                 int32_t id_user);
     int32_t(*estimate) (MD_L2P_item_t * ar_ppq, sector64 lpno_s, int32_t len);
     int32_t(*exec) (struct lfsm_dev_struct * td, struct sflushops * ops,
                     sector64 lpno_s, int32_t len);
    bool (*waitdone)(struct sflushops * ops);
    bool (*destroy)(struct sflushops * ops);
} sflushops_t;

// should be used outside bmt_lookup
#define  PBNO_ON_SSD(pbno)  (pbno != PBNO_SPECIAL_NEVERWRITE)

#endif
