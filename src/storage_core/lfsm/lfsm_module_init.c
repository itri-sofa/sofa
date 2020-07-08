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

#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/moduleparam.h>

#include "lfsm_module_init_private.h"
#include "lfsm_drv_main.h"

static int32_t __init init_lfsm_module(void)
{
    return load_lfsm_drv();
}

static void
__exit exit_lfsm_module(void)
{
    unload_lfsm_drv();
}

module_init(init_lfsm_module);
module_exit(exit_lfsm_module);

MODULE_AUTHOR(DRIVER_AUTHOR);
//MODULE_LICENSE("Proprietary");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_VERSION(VERSION_STR);
MODULE_INFO(Copyright, COPYRIGHT);
