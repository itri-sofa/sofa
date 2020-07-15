/*
 * SPDX-License-Identifier: GPL-2.0-or-later OR Apache-2.0
 * Copyright (C) 2015-2020 Industrial Technology Research Institute
 *
 *
 *
 *  
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
