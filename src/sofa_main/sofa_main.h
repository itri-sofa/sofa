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

#ifndef SOFA_MAIN_H_
#define SOFA_MAIN_H_

typedef enum {
    SOFA_DEV_S_INIT,
    SOFA_DEV_S_CONFIGING,
    SOFA_DEV_S_RUNNING,
    SOFA_DEV_S_STOPPING,
    SOFA_DEV_S_STOPPED,
} sofa_stat_t;

#define MAX_LEN_DEV_NAME    64
typedef struct vm_dev {
    int8_t chrdev_name[MAX_LEN_DEV_NAME];
    int8_t blkdev_name_prefix[MAX_LEN_DEV_NAME];
    int32_t chrdev_major_num;
    int32_t blkdev_major_num;
    sofa_stat_t sofaStat;
} sofa_dev_t;

extern sofa_dev_t sofaDev;

extern int32_t init_sofa_component(void);
extern int32_t rel_sofa_component(void);
extern int32_t init_sofa_main(void);
extern void exit_sofa_main(void);

#endif /* SOFA_MAIN_H_ */
