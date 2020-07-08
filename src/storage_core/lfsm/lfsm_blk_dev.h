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

#ifndef STORAGE_CORE_LFSM_BLK_DEV_H_
#define STORAGE_CORE_LFSM_BLK_DEV_H_

extern void remove_disk(lfsm_dev_t * mydev);
extern int32_t create_disk(lfsm_dev_t * mydev, sector64 totalPages);
extern int32_t init_blk_device(int8_t * blk_dev_n);
extern void rm_blk_device(int32_t blk_majorN);
#endif /* STORAGE_CORE_LFSM_BLK_DEV_H_ */
