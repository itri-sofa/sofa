/*
 * SPDX-License-Identifier: GPL-2.0-or-later OR Apache-2.0
 * Copyright (C) 2015-2020 Industrial Technology Research Institute
 *
 *
 *
 *  
 */
#ifndef INC_SYSAPI_LINUXKERNEL_239429110980128
#define INC_SYSAPI_LINUXKERNEL_239429110980128

typedef struct block_device sysapi_bdev;
extern sysapi_bdev *sysapi_bdev_open(int8_t * nm_bdev);
extern int64_t sysapi_bdev_getsize(sysapi_bdev * bdev);
extern void sysapi_bdev_close(sysapi_bdev * bdev);

extern sector64 sysapi_get_timer_us(void);
extern sector64 sysapi_get_timer_ns(void);

extern int32_t sysapi_bdev_name2id(int8_t * nm);
extern int8_t *sysapi_bdev_id2name(int32_t id_disk, int8_t * nm);

#endif
