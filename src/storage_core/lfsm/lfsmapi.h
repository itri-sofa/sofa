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

#ifndef LFSMAPI_H_
#define LFSMAPI_H_

extern int32_t lfsm_init(lfsm_cfg_t * myCfg, int32_t ul_off, int32_t ronly,
                         int64_t lfsm_auth_key);
extern void lfsm_cleanup(lfsm_cfg_t * myCfg, bool isNormalExit);

#endif /* LFSMAPI_H_ */
