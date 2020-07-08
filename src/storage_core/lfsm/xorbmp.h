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
