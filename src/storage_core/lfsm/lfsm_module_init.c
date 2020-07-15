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
