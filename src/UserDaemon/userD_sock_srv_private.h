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

#ifndef USERD_SOCK_SRV_PRIVATE_H_
#define USERD_SOCK_SRV_PRIVATE_H_

#define MAX_SOCK_CLIENT 20
#define MAX_RETRY_BUILD_SKSRV   90
#define RETRY_PERIOD    1       //unit second
#define MNUM_S  0xa5a5a5a5
#define MNUM_E  0xdeadbeef

typedef struct req_header {
    uint32_t mNum_s;
    uint16_t opCode;
    uint16_t subopCode;
    uint32_t reqID;
    uint32_t size;              //size of parameters
} __attribute__((packed)) req_header_t;

typedef struct resp_header {
    uint32_t mNum_s;
    uint16_t opCode;
    uint16_t subopCode;
    uint32_t reqID;
    uint16_t result;
    uint16_t reserved;
    uint32_t size;              //size of parameters
} __attribute__((packed)) resp_header_t;

typedef struct _userD_sksrv {
    int32_t running;
    int32_t sock_fd;
    int32_t sock_port;
    int32_t client_sock_fd[MAX_SOCK_CLIENT];
    pthread_mutex_t sock_fd_lock;
} userD_sksrv_t;

userD_sksrv_t sofa_sksrv;

#endif /* USERD_SOCK_SRV_PRIVATE_H_ */
