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

#ifndef USERCMD_HANDLER_H_
#define USERCMD_HANDLER_H_

#define LOCAL_IP    "127.0.0.1"
#define LOCAL_PORT  9898

/*
 * mnum start: 4 bytes
 * opcode    : 2 bytes
 * subopcode : 2 bytes
 * request ID: 4 bytes
 * size      : 4 bytes
 * mnum end  : 4 bytes
 */
#define REQ_HEADER_SIZE (4 + 2 + 2 + 4 + 4 + 4)

typedef struct resp_hder {
    uint32_t mnum_s;
    uint16_t opcode;
    uint16_t subopcode;
    uint32_t reqID;
    uint16_t result;
    uint16_t reserved;
    uint32_t parasize;
} __attribute__((packed)) resp_hder_t;

extern int32_t gen_common_header(user_cmd_t *ucmd, int8_t *buff, int32_t size);
extern void show_cmd_result(user_cmd_t *ucmd);
extern void free_user_cmd(user_cmd_t *ucmd);
extern int32_t process_user_cmd(user_cmd_t *ucmd);
extern void init_cmd_handler(void);
#endif /* USERCMD_HANDLER_H_ */
