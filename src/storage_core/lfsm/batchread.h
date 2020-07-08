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
#ifndef BATCHREAD_INC_1283910283091823
#define BATCHREAD_INC_1283910283091823

#define BRM_MAX_DEPTH 1024
#define BRM_MAX_DEPTH_MOD   1023
#define BRM_MAX_QU 64
#define BRM_MAX_QU_MOD  63

// The design of FW is supported to provide 64-cmd batch read, but it support 8 for now.
// On the other hand, lfsm support 8 for now. So....we have below two defines...
#define FTLB_CN_SLOTS 64
#define BRM_MAX_BATCH 8

#define sbrm_printk printk

typedef struct sbrm_cmd {
    int32_t cn_used_slot;
    uint32_t padding;
    uint32_t addr[FTLB_CN_SLOTS];   // should be BRM_MAX_BATCH, but fw now support FTLB_CN_SLOTS
} sbrm_cmd_t;

extern void sbrm_end_bio_generic_read(struct bio *bio);
#endif
