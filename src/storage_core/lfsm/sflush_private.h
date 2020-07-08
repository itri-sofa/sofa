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

#ifndef SFLUSH_PRIVATE_H_
#define SFLUSH_PRIVATE_H_

static int32_t sflush_exec(lfsm_dev_t * td, struct sflushops *ops,
                           sector64 s_lpno, int32_t len);
static int32_t sflush_estimate(MD_L2P_item_t * ar_ppq, sector64 lpno_s,
                               int32_t page_num);
struct sflushops ssflushops = {
    .name = "SFLUSH",
    .variables = NULL,
    .init = sflush_init,
    .estimate = sflush_estimate,
    .exec = sflush_exec,
    .waitdone = sflush_waitdone,
    .destroy = sflush_destroy
};

#endif /* SFLUSH_PRIVATE_H_ */
