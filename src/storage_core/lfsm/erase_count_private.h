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

#ifndef LFSM_ERASE_COUNT_PRIVATE_H_
#define LFSM_ERASE_COUNT_PRIVATE_H_

int8_t *EU_usage_type[EU_END_TAG] = {
    "EU_FREE",
    "EU_DATA",
    "EU_UPDATELOG",
    "EU_DEDICATE",
    "EU_ERASETAB",
    "EU_HPEU",
    "EU_PMT",
    "EU_SUPERMAP",
    "EU_UNKNOWN",
    "EU_GC"
};

#endif /* LFSM_ERASE_COUNT_PRIVATE_H_ */
