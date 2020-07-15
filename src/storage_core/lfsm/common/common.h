/*
 * SPDX-License-Identifier: GPL-2.0-or-later OR Apache-2.0
 * Copyright (C) 2015-2020 Industrial Technology Research Institute
 *
 *
 *
 *  
 */

#ifndef STORAGE_CORE_LFSM_COMMON_COMMON_H_
#define STORAGE_CORE_LFSM_COMMON_COMMON_H_

#define LOGINERR(fmt, ...) printk("[LFSM] INTERERR: "fmt, ##__VA_ARGS__)
#define LOGERR(fmt, ...) printk("[LFSM] ERR: "fmt, ##__VA_ARGS__)
#define LOGINFO(fmt, ...) printk("[LFSM] INFO: "fmt, ##__VA_ARGS__)
#define LOGWARN(fmt, ...) printk("[LFSM] WARN: "fmt, ##__VA_ARGS__)

#define LFSM_MUTEX_LOCK(X) mutex_lock(X)
#define LFSM_MUTEX_UNLOCK(X)  mutex_unlock(X)

/****** define lfsm types *****/
typedef unsigned long long sector64;
typedef sector64 page_t;
typedef unsigned int tp_ondisk_unit;
typedef unsigned int t_erase_count; /**type of erase count.( change here if necessary)*/

/****** common define utility *****/
#define LFSM_CEIL_DIV(X,Y) DIV_ROUND_UP(X,Y)    //(((X)+((Y)-1))/(Y))
#define LFSM_CEIL(X,Y) (DIV_ROUND_UP(X,Y)*(Y))
#define LFSM_FLOOR(X,Y) (((X)/(Y))*(Y))
#define LFSM_FLOOR_DIV(X,Y) ((X)/(Y))

#define lfsm_set_bit(nr, addr)  set_bit(nr%8, (void*)((char*)addr+nr/8))
#define lfsm_clear_bit(nr, addr)  clear_bit(nr%8, (void*)((char*)addr+nr/8))
#define lfsm_test_bit(nr, addr)  test_bit(nr%8, (void*)((char*)addr+nr/8))
#define LFSM_MAX(a,b)  ((a)>(b)?(a):(b))

#define  lfsm_atomic_inc_return(x)    ((uint32_t)atomic_inc_return(x))
#define lfsm_wait_event_interruptible(wq,condition) do {wait_event_interruptible(wq,condition); }while(0)

#endif /* STORAGE_CORE_LFSM_COMMON_COMMON_H_ */
