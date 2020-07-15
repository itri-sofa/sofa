/*
 * SPDX-License-Identifier: GPL-2.0-or-later OR Apache-2.0
 * Copyright (C) 2015-2020 Industrial Technology Research Institute
 *
 *
 *
 *  
 */
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <error.h>
#include <linux/types.h>
#include "common/common.h"
#include "ioctl_A.h"

int ioctl_submit(char *name, unsigned long cmd, void *data)
{

    int ret, fd;
    fd = open(name, O_RDWR | O_SYNC);

    if (fd == -1) {
        perror("open error");
        return -1;
    }

    ret = ioctl(fd, cmd, data);
    close(fd);
    return ret;
}

int main(int argc, char **argv)
{
    int i;
    sftl_cp2kern_t sftl_copy;
    char user[10];

    for (i = 0; i < 10; i++)
        user[i] = 0;

    sftl_copy.pkernel = (char *)0xffff8802b24c5000;
    sftl_copy.puser = user;
    sftl_copy.cn = 10;

    ioctl_submit("/dev/lfsm", SFTL_IOCTL_COPY_TO_KERNEL, &sftl_copy);

    return 0;

}
