/*
 * SPDX-License-Identifier: GPL-2.0-or-later OR Apache-2.0
 * Copyright (C) 2015-2020 Industrial Technology Research Institute
 *
 *
 *
 *  
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
