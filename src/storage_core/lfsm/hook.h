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
#ifndef    HOOK_INC_18293091823091823
#define    HOOK_INC_18293091823091823

#define HOOK_REPLACE_WITH_RET(original_code_segment,ret,fn_helper,...)  \
            { if (lfsmdev.hook.fn_helper) {ret=(*(lfsmdev.hook.fn_helper))(__VA_ARGS__);}  else {original_code_segment;} ;}

// original_code_segment must be one line
#define HOOK_REPLACE(original_code_segment,fn_helper,...)  \
            { if (lfsmdev.hook.fn_helper) {(*(lfsmdev.hook.fn_helper))(__VA_ARGS__);}  else {original_code_segment;} ;}

#define HOOK_INSERT(fn_helper,...)\
            { if (lfsmdev.hook.fn_helper) {(*(lfsmdev.hook.fn_helper))(__VA_ARGS__);}}

#define HOOK_ARG(member) (lfsmdev.hook.member)

typedef struct shook {
    void (*fn_change_active_eu)(struct HListGC * h, int32_t temper);
    void (*fn_biocbh_process)(int32_t usec);
    int32_t arg_biocbh_process_usec;
} shook_t;

//#define LOGERR(fmt, ...) printk("Error@%s(%d):"fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__) 

extern int32_t hook_init(shook_t * hook);

#endif
