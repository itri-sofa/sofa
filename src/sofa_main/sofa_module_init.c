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
#include "sofa_main.h"

#define DRIVER_AUTHOR        "ICL SOFA team"
#define VERSION_STR          "1.0.00"
#define COPYRIGHT            "Copyright 2015 SOFA/F/ICL/ITRI"
#define DRIVER_DESC          "software storage manager with complex features"

#define PARAM_PERMISSION 0644

static int
__init init_sofa_module(void)
{
    return init_sofa_main();
}

static void
__exit exit_sofa_module(void)
{
    exit_sofa_main();
}

module_init(init_sofa_module);
module_exit(exit_sofa_module);

MODULE_LICENSE("GPL");
MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_VERSION(VERSION_STR);
MODULE_INFO(Copyright, COPYRIGHT);
