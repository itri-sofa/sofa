/*
 * SPDX-License-Identifier: GPL-2.0-or-later OR Apache-2.0
 * Copyright (C) 2015-2020 Industrial Technology Research Institute
 *
 *
 *
 *  
 */

#ifndef SOFA_MAIN_H_
#define SOFA_MAIN_H_

typedef enum {
    SOFA_DEV_S_INIT,
    SOFA_DEV_S_CONFIGING,
    SOFA_DEV_S_RUNNING,
    SOFA_DEV_S_STOPPING,
    SOFA_DEV_S_STOPPED,
} sofa_stat_t;

#define MAX_LEN_DEV_NAME    64
typedef struct vm_dev {
    int8_t chrdev_name[MAX_LEN_DEV_NAME];
    int8_t blkdev_name_prefix[MAX_LEN_DEV_NAME];
    int32_t chrdev_major_num;
    int32_t blkdev_major_num;
    sofa_stat_t sofaStat;
} sofa_dev_t;

extern sofa_dev_t sofaDev;

extern int32_t init_sofa_component(void);
extern int32_t rel_sofa_component(void);
extern int32_t init_sofa_main(void);
extern void exit_sofa_main(void);

#endif /* SOFA_MAIN_H_ */
