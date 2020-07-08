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
#ifndef INC_SYSAPI_LINUXKERNEL_239429110980128
#define INC_SYSAPI_LINUXKERNEL_239429110980128

typedef struct block_device sysapi_bdev;
extern sysapi_bdev *sysapi_bdev_open(int8_t * nm_bdev);
extern int64_t sysapi_bdev_getsize(sysapi_bdev * bdev);
extern void sysapi_bdev_close(sysapi_bdev * bdev);

extern sector64 sysapi_get_timer_us(void);
extern sector64 sysapi_get_timer_ns(void);

extern int32_t sysapi_bdev_name2id(int8_t * nm);
extern int8_t *sysapi_bdev_id2name(int32_t id_disk, int8_t * nm);

#endif
