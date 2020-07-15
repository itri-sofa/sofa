/*
 * SPDX-License-Identifier: GPL-2.0-or-later OR Apache-2.0
 * Copyright (C) 2015-2020 Industrial Technology Research Institute
 *
 *
 *
 *  
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
