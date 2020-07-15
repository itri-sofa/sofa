/*
 * SPDX-License-Identifier: GPL-2.0-or-later OR Apache-2.0
 * Copyright (C) 2015-2020 Industrial Technology Research Institute
 *
 *
 *
 *  
 */

#include <linux/types.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/bio.h>
#include "sofa_main.h"
#include "sofa_main_private.h"
#include "../common/drv_userD_common.h"
#include "../common/sofa_config.h"
#include "drv_chrdev.h"
#include "../common/sofa_common.h"
#include "../sofa_feature_generic/io_common/storage_api_wrapper.h"

int32_t init_sofa_component(void)
{
    int32_t ret;

    ret = 0;

    sofa_printk(LOG_INFO, "main INFO initial all sofa components lfsm\n");

    ret = storage_init();
    sofa_printk(LOG_INFO, "main INFO initial all sofa components done\n");

    return ret;
}

int32_t rel_sofa_component(void)
{
    int32_t ret;

    ret = 0;

    rel_chrdev(&sofaDev);

    rel_sofa_comm();

    return ret;
}

static void _init_sofa_mainDev(sofa_dev_t * myDev)
{
    int32_t len;

    memset(myDev, 0, sizeof(sofa_dev_t));

    myDev->sofaStat = SOFA_DEV_S_INIT;
    myDev->blkdev_major_num = 0;
    myDev->chrdev_major_num = SOFA_CHRDEV_MAJORNUM;
    memset(myDev->chrdev_name, '\0', sizeof(int8_t) * MAX_LEN_DEV_NAME);
    memset(myDev->blkdev_name_prefix, '\0', sizeof(int8_t) * MAX_LEN_DEV_NAME);

    len = strlen(SOFA_CHRDEV_Name);
    strcpy(myDev->chrdev_name, SOFA_CHRDEV_Name);
}

static void show_sofa_startup(void)
{
    printk("[SOFA]: -----------------------------------------------------\n");
    printk("[SOFA]: |            SOFA kernel module start               |\n");
    printk("[SOFA]: -----------------------------------------------------\n");
}

int32_t init_sofa_main(void)
{
    int32_t ret;

    ret = 0;

    show_sofa_startup();

    _init_sofa_mainDev(&sofaDev);
    init_sofa_comm();
    init_chrdev(&sofaDev);

    return ret;
}

static void show_sofa_shutdown(void)
{
    printk("[SOFA]: -----------------------------------------------------\n");
    printk("[SOFA]: |            SOFA kernel module stopped             |\n");
    printk("[SOFA]: -----------------------------------------------------\n");
}

void exit_sofa_main(void)
{
    sofa_printk(LOG_INFO, "main INFO shutdown sofa driver start\n");

    sofaDev.sofaStat = SOFA_DEV_S_STOPPING;
    rel_sofa_component();
    sofaDev.sofaStat = SOFA_DEV_S_STOPPED;
    show_sofa_shutdown();
}
