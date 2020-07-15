/*
 * Copyright (c) 2015-2020 Industrial Technology Research Institute.
 *
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 *
 *
 *
 *
 *
 *
 *  
 */

#ifndef LFSM_REQ_HANDLER_PRIVATE_H_
#define LFSM_REQ_HANDLER_PRIVATE_H_

#define CMDSIZE 128

#define SMART_OUTSIZE 8192
#define SPARE_OUTSIZE 1024
#define GROUPDISK_OUTSIZE 1024
#define PERF_OUTSIZE 4096
#define MAX_DEV_PATH 16

typedef struct gcInfo {
    uint32_t gcSeuResv;
    uint64_t gcInfoTest64;
    uint32_t gcInfoTest;
} gcInfo_t;

typedef struct diskSMART {
    int8_t msgInfo[SMART_OUTSIZE];
} diskSMART_t;

#endif /* LFSM_REQ_HANDLER_PRIVATE_H_ */
