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
