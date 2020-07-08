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
///////////////////////////////////////////////////////////////////////////////
//    Overview:
//        rmutex is a extend version of Linux mutex to provide 2 additional features.
//
//        The first feature is about the variable "rm->state", but the feature has
//        been suspressed by SOFA.
//         You should find all callers in SOFA set the rm->state to zero, i.e. they call
//        LFSM_RMUTEX_LOCK / LFSM_RMUTEX_UNLOCK to use rmutex, instead of
//        LFSM_RMUTEX_LOCK1 / LFSM_RMUTEX_UNLOCK1
//
//        The second feature is to help SOFA to call a user-defined callback function
//        right after a mutex is unlocked.
//        To use the function, before the mutex is unlocked, you should call
//        rmutex_unlock_hook() to hook a callback function. Then, once the mutex is unlocked,
//        the function you specified will be called.
//
//        The actual usage example is the "whole_lock" in UpdateLog_logging().
//        SOFA sets the callback to UpdateLog_hook().
//////////////////////////////////////////////////////////////////////////////
#include <linux/types.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/wait.h>
#include "../config/lfsm_setting.h"
#include "../config/lfsm_feature_setting.h"
#include "../config/lfsm_cfg.h"
#include "common.h"
#include "rmutex.h"
#include "../io_manager/mqbiobox.h"
#include "../io_manager/biocbh.h"
#include "../nextprepare.h"
#include "../diskgrp.h"
#include "../system_metadata/Dedicated_map.h"
#include "../lfsm_core.h"

void rmutex_init(rmutex_t * rm)
{
    mutex_init(&rm->lock);
    rm->pid = -1;
    rm->state = 0;
    rm->unlock_hook_func = NULL;
}

void rmutex_destroy(rmutex_t * rm)
{
    if (rm->pid != -1) {
        LOGWARN("mutex %p is locking by pid %d\n", &rm->lock, rm->pid);
    }
    mutex_destroy(&rm->lock);
}

// the hooked function should consider the possiblity of reentry, or said should be reentry safe.
void rmutex_unlock_hook(rmutex_t * rm, bool (*pfunc)(void *), void *parg)
{
    LFSM_ASSERT(mutex_is_locked(&rm->lock));
    rm->unlock_hook_func = pfunc;
    rm->unlock_hook_arg = parg;
    return;
}

void rmutex_lock(rmutex_t * rm, int32_t state)
{
    if (rm->pid != current->pid) {
        RMUTEX_LOCK_A(rm, state);
    } else {
        LFSM_ASSERT(state > rm->state);
        rm->state = state;
    }

    return;
}

void rmutex_unlock(rmutex_t * rm, int32_t state)
{
    bool (*pfunc)(void *);
    void *parg;

    LFSM_ASSERT2(state == rm->state, "rm_state= %d state= %d\n", rm->state,
                 state);
    LFSM_ASSERT2(rm->pid == current->pid, "rm_pid= %d cur_pid= %d\n", rm->pid,
                 current->pid);

    if (rm->state == 0) {
        rm->state--;
        rm->pid = -1;
        pfunc = rm->unlock_hook_func;
        rm->unlock_hook_func = NULL;
        parg = rm->unlock_hook_arg;
        rm->unlock_hook_arg = NULL;
        mutex_unlock(&rm->lock);
        if (pfunc) {
            pfunc(parg);
        }
        return;
    }
    rm->state--;
}
