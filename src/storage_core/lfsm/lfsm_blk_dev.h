/*
 * SPDX-License-Identifier: GPL-2.0-or-later OR Apache-2.0
 * Copyright (C) 2015-2020 Industrial Technology Research Institute
 *
 *
 *
 *  
 */

#ifndef STORAGE_CORE_LFSM_BLK_DEV_H_
#define STORAGE_CORE_LFSM_BLK_DEV_H_

extern void remove_disk(lfsm_dev_t * mydev);
extern int32_t create_disk(lfsm_dev_t * mydev, sector64 totalPages);
extern int32_t init_blk_device(int8_t * blk_dev_n);
extern void rm_blk_device(int32_t blk_majorN);
#endif /* STORAGE_CORE_LFSM_BLK_DEV_H_ */
