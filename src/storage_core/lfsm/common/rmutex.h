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
#ifndef RMUTEX_LOCK_H
#define RMUTEX_LOCK_H
#include <linux/mutex.h>

#define LFSM_RMUTEX_LOCK(x)     rmutex_lock(x,0)
#define LFSM_RMUTEX_UNLOCK(x)    rmutex_unlock(x,0)
#define LFSM_RMUTEX_LOCK1(x)    rmutex_lock(x,1)
#define LFSM_RMUTEX_UNLOCK1(x)     rmutex_unlock(x,1)

#ifdef  HLIST_GC_WHOLE_LOCK_DEBUG
/*
 * NOTE: Lego: user can log message through marcro RMUTEX_LOG_LOCKSRC
 *       to help lock debug
 */
#define RMUTEX_LOG_LOCKSRC(x, fmt, ...)   \
    do { \
        sprintf(x->lock_fn_name, fmt, ##__VA_ARGS__); \
        x->access_cnt++; \
    } while (0)
#define RMUTEX_CR_LOCKSRC(x, y)    memset(x->lock_fn_name, '\0', y)
#else
#define RMUTEX_LOG_LOCKSRC(x, fmt, ...)   {}
#define RMUTEX_CR_LOCKSRC(x, y)
#endif

#define RMUTEX_LOCK_A(x,s) {\
    mutex_lock(&x->lock); \
    x->pid = current->pid; \
    x->state = s;    }\

typedef struct rmutex {
    int32_t pid;
    struct mutex lock;
    int32_t state;              //outside state get
    bool (*unlock_hook_func)(void *);
    void *unlock_hook_arg;
} rmutex_t;

extern void rmutex_init(rmutex_t * rm);
extern void rmutex_destroy(rmutex_t * rm);
extern void rmutex_lock(rmutex_t * rm, int32_t state);
extern void rmutex_unlock(rmutex_t * rm, int32_t state);
extern void rmutex_unlock_hook(rmutex_t * rm, bool (*pfunc)(void *),
                               void *parg);
#endif
