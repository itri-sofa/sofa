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

#include <linux/types.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/module.h>
#include <linux/uaccess.h>
#include "sofa_main.h"
#include "drv_chrdev.h"
#include "../common/drv_userD_common.h"
#include "../common/sofa_common.h"
#include "../common/sofa_mm.h"
#include "../common/sofa_config.h"
#include "drv_lfsmcmd_handler.h"

static int32_t _do_ioctl_config(uint64_t ioctl_param)
{
    cfg_para_t cfgPara;
    int32_t ret;

    ret =
        copy_from_user(&cfgPara, (void __user *)ioctl_param,
                       sizeof(cfg_para_t));

    if (ret) {
        sofa_printk(LOG_WARN,
                    "CHRDEV WARN fail copy config from User daemon\n");
        SOFA_WARN_ON(true);
        return ret;
    }

    if (cfgPara.valSet_len > 0) {
        set_config_item(cfgPara.name, cfgPara.value, cfgPara.valSet);
    } else {
        set_config_item(cfgPara.name, cfgPara.value, NULL);
    }

    return ret;
}

static int32_t _do_ioctl_init(uint64_t ioctl_param)
{
    int32_t ret;

    ret = init_sofa_component();

    return ret;
}

static int32_t _dispatch_usercmd(usercmd_para_t * ucmdPara)
{
    int32_t ret;

    ret = 0;

    switch (ucmdPara->opcode) {
    case CMD_OP_CONFIG:
        ucmdPara->cmd_result = -1;
        break;
    case CMD_OP_LFSM:
        ret = handle_lfsm(ucmdPara);
        break;
    default:
        ucmdPara->cmd_result = -1;
        sofa_printk(LOG_WARN, "CHRDEV WARN unknown user cmd opcode %d\n",
                    ucmdPara->opcode);
        break;
    }

    return ret;
}

static int32_t _do_ioctl_usercmd(uint64_t ioctl_param)
{
    usercmd_para_t *ucmdPara;
    int32_t ret;

    //TODO: check whether drv ready

    //sofa_printk(LOG_INFO, "main INFO handle user command\n");

    ucmdPara = sofa_kmalloc(sizeof(usercmd_para_t), GFP_KERNEL);
    if (IS_ERR_OR_NULL(ucmdPara)) {
        return -ENOMEM;
    }

    ret =
        copy_from_user(ucmdPara, (void __user *)ioctl_param,
                       sizeof(usercmd_para_t));

    if (ret) {
        sofa_printk(LOG_WARN, "CHRDEV WARN fail copy cmd from User daemon\n");
        SOFA_WARN_ON(true);
        return ret;
    }

    ret = _dispatch_usercmd(ucmdPara);

    if (copy_to_user
        ((void __user *)ioctl_param, ucmdPara, sizeof(usercmd_para_t))) {
        ret = -EFAULT;
        sofa_printk(LOG_WARN,
                    "CHRDEV WARN fail copy cmd result to User daemon\n");
        SOFA_WARN_ON(true);
        //TODO: Should we undo the cmd?
    }

    sofa_kfree(ucmdPara);
    //sofa_printk(LOG_INFO, "main INFO handle user command ret %d\n", ret);
    return ret;
}

static int32_t chrdev_ioctl_gut(uint32_t ioctl_num, uint64_t ioctl_param)
{
    int32_t ret;

    ret = 0;

    switch (ioctl_num) {
    case IOCTL_CONFIG_DRV:
        ret = _do_ioctl_config(ioctl_param);
        break;

    case IOCTL_INIT_DRV:
        ret = _do_ioctl_init(ioctl_param);
        break;
    case IOCTL_USER_CMD:
        ret = _do_ioctl_usercmd(ioctl_param);
        break;
//    case IOCTL_GET_DRV_STATE:
//        ret = _ioctl_get_drv_stat(ioctl_param);
//        break;

//    case IOCTL_REGISTER_DRV:
//        ret = _do_ioctl_reg(ioctl_param);
//        break;

//    case IOCTL_REMOVE_ALL_COMP:
//        ret = _do_ioctl_rel_all_comps(ioctl_param);
//        break;
    default:
        //ret = _do_normal_ioctl(ioctl_num, ioctl_param);
        sofa_printk(LOG_INFO, "main INFO unknown ioctl command %d\n",
                    ioctl_num);
        break;
    }

    return ret;
}

long sofa_chrdev_ioctl(struct file *file,   /* ditto */
                       unsigned int ioctl_num,  /* number and param for ioctl */
                       unsigned long ioctl_param)
{
    return (long)chrdev_ioctl_gut(ioctl_num, ioctl_param);
}

struct file_operations chrdev_fops = {
  owner:
    THIS_MODULE,
  unlocked_ioctl:
    sofa_chrdev_ioctl
};

int32_t init_chrdev(sofa_dev_t * myDev)
{
    int32_t ret;

    ret = 0;

    if (register_chrdev
        (myDev->chrdev_major_num, myDev->chrdev_name, &chrdev_fops)) {
        sofa_printk(LOG_ERR, "main ERROR unable register char device %s %d\n",
                    myDev->chrdev_name, myDev->chrdev_major_num);
        ret = -ENOMEM;
    } else {
        sofa_printk(LOG_INFO, "main INFO register char device success %s %d\n",
                    myDev->chrdev_name, myDev->chrdev_major_num);
    }

    return ret;
}

void rel_chrdev(sofa_dev_t * myDev)
{
    unregister_chrdev(myDev->chrdev_major_num, myDev->chrdev_name);
    sofa_printk(LOG_INFO,
                "main INFO unregister character device success %s %d\n",
                myDev->chrdev_name, myDev->chrdev_major_num);
}
