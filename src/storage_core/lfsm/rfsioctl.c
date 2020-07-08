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
#include <string.h>
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

static int32_t is_non_number(char *data)
{
    int32_t i = 0;
    int32_t size = strlen(data);
    int32_t ret = 1;
    for (i = 0; i < size; i++) {
        if (data[i] == ' ') {
            continue;
        }
        if (data[i] >= '0' && data[i] <= 9) {
            continue;
        }
        ret = 0;
        break;
    }
    return ret;
}

int main(int argc, char **argv)
{
    int ret, fd, i;

    if (argc < 3) {
        printf("rfsioctl filename print_debug\n");
        printf("rfsioctl filename finish_init type num\n");
        printf("rfsioctl filename submit_write lbno ReqSize async Rw\n");
        printf
            ("rfsioctl filename simu_write wNum ReqSize milliDelay simuBatch workloadType(0:seq, 1:random) direction(0:read, 1,write) start_sector\n");

        return -1;
    };

    //fd = open (argv[1], O_RDWR|O_SYNC);

    if (!strcmp(argv[2], "print_debug")) {
        ret = ioctl_submit(argv[1], SFTL_IOCTL_PRINT_DEBUG, NULL);
        return 0;
    }

    if (!strcmp(argv[2], "finish_init")) {
        sftl_ioctl_simu_w_t arg;
        memset(&arg, 0, sizeof(sftl_ioctl_simu_w_t));
        if (argc != 4 && argc != 5 && argc != 6 && argc != 7) {
            printf
                ("rfsioctl filename finish_init type (1: population finishes; 2: DO read disk at exit; 3: DO NOT read at exit)\n");
        } else {
            arg.m_nType = atoi(argv[3]);
            if (argc > 4) {
                if (!is_non_number(argv[4])) {
                    printf
                        ("rfsioctl with non_number: argv[4]=%s### strlen(argv[4])=%lu\n",
                         argv[4], strlen(argv[4]));
                    strncpy(arg.data, argv[4], strlen(argv[4]));
                } else {
                    arg.m_nSum = atoi(argv[4]);
                }
            }
            if (argc > 5) {
                arg.m_nSize = atoi(argv[5]);
            }
            if (argc > 6) {
                arg.m_nBatchSize = atoi(argv[6]);
            }
            ret = ioctl_submit(argv[1], SFTL_IOCTL_INIT_FIN, &arg);
        }
        perror("rfsioctl:");
        printf("rfsioctl type=%d arg1=%d arg2=%d ret= %d\n", arg.m_nType,
               arg.m_nSum, arg.m_nSize, ret);
        return ret;
    }

    if (!strcmp(argv[2], "simu_write")) {
        sftl_ioctl_simu_w_t arg;
        if (argc != 10) {
            printf
                ("rfsioctl filename simu_write wNum ReqSize milliDelay simuBatch workloadType(0:seq, 1,random) direction(0:read, 1,write) start_sector\n");
        } else {

            arg.m_nSum = atoi(argv[3]);

            arg.m_nSize = atoi(argv[4]);

            arg.m_nMd = atoi(argv[5]);

            arg.m_nBatchSize = atoi(argv[6]);

            arg.m_nType = atoi(argv[7]);

            arg.m_nRw = atoi(argv[8]);

            arg.m_start_sector = atol(argv[9]);

            ret = ioctl_submit(argv[1], SFTL_IOCTL_SIMU_IO, &arg);

            if (ret) {
                perror("ioctl");
            }
        }
        return 0;
    }

    if (!strcmp(argv[2], "submit_write")) {
        sftl_ioctl_simu_w_t arg;
        if (argc != 7) {
            printf("rfsioctl filename submit_write lbno ReqSize Async Rw\n");
        } else {

            arg.m_nSum = atoi(argv[3]);

            arg.m_nSize = atoi(argv[4]);

            arg.m_nType = atoi(argv[5]);

            arg.m_nRw = atoi(argv[6]);

            printf("lbn: %u, size: %d, sync: %d, rw: %d\n", arg.m_nSum,
                   arg.m_nSize, arg.m_nType, arg.m_nRw);

            ret = ioctl_submit(argv[1], SFTL_IOCTL_SUBMIT_IO, &arg);

            if (ret) {
                perror("ioctl");
            }

        }
        return 0;
    }

    if (strcmp(argv[2], "scandev") == 0) {
        sftl_ioctl_scandev_t arg_scandev;
        char *nm_dev, *action;

        if (strcmp(getenv("SUBSYSTEM"), "block") != 0) {
            return -1;
        }

        if (((nm_dev = getenv("DEVNAME")) == NULL)
            || ((action = getenv("ACTION")) == NULL)) {
            printf("No device information\n");
            return -1;
        }
        if (strcmp(action, "add") != 0 && strcmp(action, "remove") != 0)
            return -1;
        strcpy((char *)arg_scandev.nm_dev, nm_dev);
        strcpy((char *)arg_scandev.action, action);
        ret = ioctl_submit(argv[1], SFTL_IOCTL_SCANDEV, &arg_scandev);
        return ret;

    }
    if (strcmp(argv[2], "trimfpg") == 0) {
        sftl_ioctl_trim_t arg;
        arg.lpn = atol(argv[3]);
        arg.cn_pages = atol(argv[4]);
        ret = ioctl_submit(argv[1], SFTL_IOCTL_TRIM, &arg);
        return ret;
    }
    if (strcmp(argv[2], "dumpcall") == 0) {
        ret =
            ioctl_submit(argv[1], SFTL_IOCTL_DUMPCALL,
                         (void *)(unsigned long long)((argc == 4) ?
                                                      atoi(argv[3]) : 0));

        return ret;
    }

    if (strcmp(argv[2], "dumpsyseu") == 0) {
        sftl_ioctl_sys_dump_t arg_dumpsyseu;
        if (argc != 4) {
            printf(" argc !=4, \n");
            return -1;
        }

        arg_dumpsyseu.num_item = atoi(argv[3]);
        ret = ioctl_submit(argv[1], SFTL_IOCTL_SYS_DUMPALL, &arg_dumpsyseu);
        return ret;
    }

    if (strcmp(argv[2], "changedev") == 0) {
        sftl_change_dev_name_t arg_change_name;
        if (argc != 5) {
            printf(" argc !=4, \n");
            return -1;
        }
        arg_change_name.disk_num = atoi(argv[3]);
        arg_change_name.nm_dev = argv[4];
        ret =
            ioctl_submit(argv[1], SFTL_IOCTL_CHANGE_DEVNAME_DUMPALL,
                         &arg_change_name);
        return ret;
    }

    printf("invalid command\n");

    return -1;
}
