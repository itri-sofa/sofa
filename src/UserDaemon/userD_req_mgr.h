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

#ifndef USERD_REQ_MGR_H_
#define USERD_REQ_MGR_H_

int32_t handle_req(int32_t ioctlfd, int16_t opcode, int16_t subopcode,
                   int8_t * para_buff, int32_t * resp_size,
                   int8_t ** resp_buff);
int32_t init_reqs_mgr(void);
int32_t rel_reqs_mgr(void);

#endif /* USERD_REQ_MGR_H_ */
