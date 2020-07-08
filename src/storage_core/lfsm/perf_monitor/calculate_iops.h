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

#ifndef CALCULATE_IOPS_H_
#define CALCULATE_IOPS_H_

#define ENABLE_CAL_IOPS

extern atomic_t cal_request_iops;
extern atomic_t cal_respond_iops;

#ifdef ENABLE_CAL_IOPS
#define INC_RESP_IOPS() increase_resp_iops()
#define INC_REQ_IOPS() increase_req_iops()
#else
#define INC_RESP_IOPS() {}
#define INC_REQ_IOPS() {}
#endif

extern void increase_resp_iops(void);
extern void increase_req_iops(void);

#endif /* CALCULATE_IOPS_H_ */
