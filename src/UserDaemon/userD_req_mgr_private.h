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

#ifndef USERD_REQ_MGR_PRIVATE_H_
#define USERD_REQ_MGR_PRIVATE_H_

#if 0
typedef enum {
    cmd_create = 0,
    cmd_clone,
    cmd_instantiate,
    cmd_snapshot,
    cmd_restore,
    cmd_restore_by_inst,
    cmd_delete,
    cmd_extend_size,
    cmd_trim_block,
    cmd_getFulllist,
    cmd_getdeltaList,
    cmd_attach,
    cmd_detach
} req_op_code_t;
#endif

typedef struct _usrD_req_mgr {
    struct list_head req_pendq;
    struct list_head req_workq;
    pthread_mutex_t reqs_qlock;
    uint32_t cnt_reqs;
    uint32_t cnt_wreqs;
    int32_t running;
} userD_reqMgr_t;

userD_reqMgr_t userD_reqMgr;

#endif /* USERD_REQ_MGR_PRIVATE_H_ */
