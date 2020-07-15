/*
 * Copyright (c) 2015-2020 Industrial Technology Research Institute.
 *
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 *
 *
 *
 *
 *
 *
 *  
 */
#ifndef SOFA_COMMON_H_
#define SOFA_COMMON_H_

#include <linux/slab.h>

typedef enum {
    LOG_DEBUG,
    LOG_INFO,
    LOG_WARN,
    LOG_ERR,
    LOG_EMERG
} sofa_loglvl_t;

extern uint32_t sofa_loglevel;
extern atomic_t sofa_rtime_err;

#define sofa_printk(LOG_LEVEL, fmt, args...) if(LOG_LEVEL >= sofa_loglevel) printk("[SOFA] "fmt, ##args)

/*
 * NOTE: under some deployment, compile fail due to atomic.h not found,
 *       include slab.h (or other header contains atomic.h) for passing compiler
 */
#define SOFA_WARN_ON(cond)   do {\
    if (cond) { atomic_inc(&sofa_rtime_err); }\
    WARN_ON(cond); \
    } while (0)

#define list_add_before(new_node, node) \
        list_add_tail(new_node, node)
#define list_add_behind(new_node, node) \
        list_add(new_node, node)

#define SOFA_FLOOR_DIV(X,Y) ((X)/(Y))
#define SOFA_CEIL_DIV(X,Y) (((X)+((Y)-1))/(Y))

extern int32_t init_sofa_comm(void);
extern int32_t rel_sofa_comm(void);

#endif /* SOFA_COMMON_H_ */
