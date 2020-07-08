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
