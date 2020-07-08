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
