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
