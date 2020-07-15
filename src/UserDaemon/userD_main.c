/*
 * Copyright (c) 2015-2020 Industrial Technology Research Institute.
 *
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 *
 *
 *
 *
 *
 *
 *  
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <syslog.h>
#include <sys/types.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <stdint.h>
#include <pthread.h>
#include <sys/ioctl.h>
#include "../common/sofa_mm.h"
#include "../common/sofa_config.h"
#include "../common/drv_userD_common.h"
#include "userD_main_private.h"
#include "userD_main.h"
#include "userD_req_mgr.h"
#include "userD_sock_srv.h"
#include "calculate_perf.h"

static int32_t init_sofa_drv(int32_t chrdev_fd)
{
    int32_t ret;

    ret = ioctl(chrdev_fd, IOCTL_INIT_DRV, NULL);

    return ret;
}

static int32_t _check_driver(int8_t * drvfn)
{
    int8_t buf[256], cmd[DRV_FN_MAX_LEN * 2];
    FILE *fp;
    int32_t ret;

    ret = -1;

    sprintf(cmd, "lsmod|grep %s", drvfn);

    fp = popen(cmd, "r");

    memset(buf, 0, sizeof(int8_t) * 256);
    if (fgets(buf, 256, fp) != NULL) {
        if (buf[0] == 0) {
            ret = -1;
        } else {
            ret = 0;
        }
    }
    pclose(fp);

    return ret;
}

static int32_t _set_lfsm_install_cmd(int8_t * cmd)
{
    int8_t *name_dir, *name_drv, *val_dir, *val_drv;

#if 0
    int8_t *name_para1, *name_para2, *name_para3, *name_para4;
    int64_t val_para1, val_para2, val_para3, val_para4;

    get_config_name_val(config_cn_ssds, &name_para1, &val_para1, NULL, NULL);
    get_config_name_val(config_ssds_hpeu, &name_para2, &val_para2, NULL, NULL);
    get_config_name_val(config_cn_pgroup, &name_para3, &val_para3, NULL, NULL);
    get_config_name_val(config_reinstall, &name_para4, &val_para4, NULL, NULL);

    if (name_para1 == NULL || name_para2 == NULL || name_para3 == NULL
        || name_para4 == NULL) {
        syslog(LOG_ERR,
               "[SOFA] USERD ERROR fail get sofa_vm loading parameters\n");
        return -1;
    }

    if (val_para1 == NULL_CONFIG_VAL || val_para2 == NULL_CONFIG_VAL ||
        val_para3 == NULL_CONFIG_VAL || val_para4 == NULL_CONFIG_VAL) {
        syslog(LOG_ERR,
               "[SOFA] USERD ERROR fail get sofa_vm loading parameters\n");
        return -1;
    }

    get_config_name_val(config_sofa_dir, &name_dir, NULL, &val_dir, NULL);
    get_config_name_val(config_lfsmdrvM_fn, &name_drv, NULL, &val_drv, NULL);
    sprintf(cmd, "insmod %s/%s%s %s=%u %s=%llu %s=%u %s=%u",
            val_dir, val_drv, DRV_FN_EXTEN,
            name_para1, val_para1, name_para2, val_para2,
            name_para3, val_para3, name_para4, val_para4);
#else
    get_config_name_val(config_sofa_dir, &name_dir, NULL, &val_dir, NULL);
    get_config_name_val(config_lfsmdrvM_fn, &name_drv, NULL, &val_drv, NULL);
    sprintf(cmd, "insmod %s/%s%s", val_dir, val_drv, DRV_FN_EXTEN);
#endif

    return 0;
}

static int32_t _set_sofa_install_cmd(int8_t * cmd)
{
    int8_t *name_dir, *name_drv, *val_dir, *val_drv;

    get_config_name_val(config_sofa_dir, &name_dir, NULL, &val_dir, NULL);
    get_config_name_val(config_sofadrvM_fn, &name_drv, NULL, &val_drv, NULL);
    sprintf(cmd, "insmod %s/%s%s", val_dir, val_drv, DRV_FN_EXTEN);
    return 0;
}

static int32_t _chk_chr_device(int8_t * chrDev_name, int32_t chr_Mnum)
{
    int8_t chrDev_full[128], cmd[128];
    FILE *fp;

    sprintf(chrDev_full, "/dev/%s", chrDev_name);
    fp = fopen(chrDev_full, "r");

    if (!fp) {
        sprintf(cmd, "mknod %s c %d 0", chrDev_full, chr_Mnum);
        system(cmd);
        syslog(LOG_INFO, "[SOFA] USERD INFO mknod %s c %d 0\n", chrDev_full,
               chr_Mnum);
    } else {
        fclose(fp);
        syslog(LOG_INFO, "[SOFA] USERD INFO char device interface exist\n");
    }

    fd_chrDev = open(chrDev_full, O_RDWR);

    if (fd_chrDev == -1) {
        syslog(LOG_ERR, "[SOFA] USERD ERROR char interface open fail %s\n",
               chrDev_full);
        return -1;
    }

    return 0;
}

static int32_t _install_driver(void)
{
    int8_t cmd[128];
    FILE *fp;
    int8_t *lfsm_drv, *sofa_drv, *val_lfsm, *val_sofa;
    int32_t ret;

    get_config_name_val(config_lfsmdrvM_fn, &lfsm_drv, NULL, &val_lfsm, NULL);
    get_config_name_val(config_sofadrvM_fn, &sofa_drv, NULL, &val_sofa, NULL);

    ret = 0;
    syslog(LOG_INFO, "[SOFA] USERD INFO Install SOFA Driver\n");

    if (_check_driver(val_lfsm)) {
        if (_set_lfsm_install_cmd(cmd)) {
            return -1;
        }
        syslog(LOG_INFO, "[SOFA] USERD INFO try to load lfsm driver %s\n", cmd);
        system(cmd);
    }

    if (_check_driver(val_sofa)) {
        if (_set_sofa_install_cmd(cmd)) {
            //TODO: remove lfsm or sofa_dummy
            return -1;
        }
        syslog(LOG_INFO, "[SOFA] USERD INFO try to load sofa driver %s\n", cmd);
        system(cmd);
    }

    if (_chk_chr_device(SOFA_CHRDEV_Name, SOFA_CHRDEV_MAJORNUM)) {
        //TODO: remove all driver
        syslog(LOG_ERR, "[SOFA] USERD ERROR fail open char device\n");
        return -1;
    }

    return ret;
}

static void conn_broken()
{
    syslog(LOG_ERR, "SOFA INFO conn broken\n");
}

static void _rm_drv_module(int8_t * drvfn)
{
    int8_t cmd[64];

    sprintf(cmd, "rmmod %s", drvfn);
    system(cmd);
}

static void _rm_all_sofa_drv(void)
{
    int8_t cmd[64];
    int8_t *lfsm_drv, *sofa_drv, *val_lfsm, *val_sofa;

    get_config_name_val(config_lfsmdrvM_fn, &lfsm_drv, NULL, &val_lfsm, NULL);
    get_config_name_val(config_sofadrvM_fn, &sofa_drv, NULL, &val_sofa, NULL);

    close(fd_chrDev);

    _rm_drv_module(val_sofa);
    _rm_drv_module(val_lfsm);

    sprintf(cmd, "rm -rf /dev/%s", SOFA_CHRDEV_Name);
    system(cmd);
}

void sighandler(int32_t sig)
{
    syslog(LOG_INFO, "[SOFA] USERD INFO shutdown sofa\n");

#ifdef CALCULATE_PERF_ENABLE
    stop_perf_thread();
#endif

    _rm_all_sofa_drv();

    stop_socket_server();
}

static void show_start_screen(void)
{
    syslog(LOG_INFO, "-----------------------------------------------------\n");
    syslog(LOG_INFO, "|            SOFA User Daemon start                 |\n");
    syslog(LOG_INFO, "-----------------------------------------------------\n");
}

static void show_stop_screen(void)
{
    syslog(LOG_INFO, "-----------------------------------------------------\n");
    syslog(LOG_INFO, "|            SOFA User Daemon stopped               |\n");
    syslog(LOG_INFO, "-----------------------------------------------------\n");
}

static void _roll_back_ssd_setting(void)
{
    int8_t *sched_drv, *val_sched;

    config_sofa_UsrD_exit();    //roll back each ssd's setting

}

static void _setup_user_start_para(int32_t argc, int8_t ** argv,
                                   int8_t ** cfgfile, int32_t * user_reset)
{
    if (argc == 2) {
        if (strcmp(argv[1], "-r") == 0) {
            *user_reset = 1;
        }
        return;
    }

    if (argc == 3) {
        if (strcmp(argv[1], "-c") == 0) {
            *cfgfile = argv[2];
        }
        return;
    }

    if (argc == 4) {
        if (strcmp(argv[1], "-r") == 0 && strcmp(argv[2], "-c") == 0) {
            *user_reset = 1;
            *cfgfile = argv[3];
        }
        if (strcmp(argv[1], "-c") == 0 && strcmp(argv[3], "-r") == 0) {
            *user_reset = 1;
            *cfgfile = argv[2];
        }

        return;
    }
}

int32_t main(int32_t argc, int8_t ** argv)
{
    int8_t *cfile, *name_port, *name_reset;
    int64_t srvPort, val_reset;
    int32_t ret, user_reset;
    pthread_t pid;

    show_start_screen();

    signal(SIGINT, &sighandler);
    signal(SIGTERM, &sighandler);
    signal(SIGPIPE, &conn_broken);

    ret = 0;

    cfile = NULL;
    user_reset = 0;
    _setup_user_start_para(argc, argv, &cfile, &user_reset);

    syslog(LOG_INFO, "[SOFA] USERD INFO User level daemon start\n");

    do {
        init_sofa_mm();
        init_sofa_config();

        if ((ret = load_sofa_config(cfile))) {
            syslog(LOG_ERR, "[SOFA] USERD ERROR fail load config\n");
            break;
        }

        if (user_reset) {
            set_config_item_index(config_reinstall, NULL, 1);
        }
        show_sofa_config();

        if ((ret = _install_driver())) {
            syslog(LOG_ERR, "[SOFA] USERD ERROR fail load driver\n");
            break;
        }

        config_sofa_drv(fd_chrDev);

        get_config_name_val(config_reinstall, &name_reset, &val_reset, NULL,
                            NULL);
        if (val_reset) {
            set_config_item_index(config_reinstall, NULL, 0);
            save_sofa_config();
        }
        //TODO: establish netlink

        config_sofa_UsrD_pre();
        if (init_sofa_drv(fd_chrDev)) {
            syslog(LOG_ERR, "[SOFA] USERD fail init sofa driver\n");
            _rm_all_sofa_drv();
            break;
        }

        config_sofa_UsrD_post();

        init_reqs_mgr();

        get_config_name_val(config_srv_port, &name_port, &srvPort, NULL, NULL);

#ifdef CALCULATE_PERF_ENABLE
        pthread_create(&pid, NULL, (void *)create_perf_thread, NULL);
#endif

        create_socket_server((int32_t) srvPort);
    } while (0);

    _roll_back_ssd_setting();

    rel_sofa_config();
    rel_sofa_mm();
    show_stop_screen();

    return ret;
}
