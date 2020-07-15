/*
 * SPDX-License-Identifier: GPL-2.0-or-later OR Apache-2.0
 * Copyright (C) 2015-2020 Industrial Technology Research Institute
 *
 *
 *
 *  
 */
#ifndef __IOCTL_H
#define __IOCTL_H

int lfsm_ioctl(struct block_device *device, fmode_t mode, unsigned int cmd,
               unsigned long arg);
int32_t ioctl_check_ddmap_updatelog_bmt(lfsm_dev_t * td,
                                        sftl_err_msg_t * perr_msg,
                                        sftl_ioctl_sys_dump_t * arg);

extern int32_t fg_limit_iops;
extern int32_t gmax_kiops;
extern int32_t cn_udelay;
#endif
