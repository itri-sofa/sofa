/*
 * SPDX-License-Identifier: GPL-2.0-or-later OR Apache-2.0
 * Copyright (C) 2015-2020 Industrial Technology Research Institute
 *
 *
 *
 *  
 */

#ifndef DRV_CHRDEV_H_
#define DRV_CHRDEV_H_

int32_t init_chrdev(sofa_dev_t * myDev);
void rel_chrdev(sofa_dev_t * myDev);

#endif /* DRV_CHRDEV_H_ */
