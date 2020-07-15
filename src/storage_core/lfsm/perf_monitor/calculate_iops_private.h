/*
 * SPDX-License-Identifier: GPL-2.0-or-later OR Apache-2.0
 * Copyright (C) 2015-2020 Industrial Technology Research Institute
 *
 *
 *
 *  
 */

#ifndef CALCULATE_IOPS_PRIVATE_H_
#define CALCULATE_IOPS_PRIVATE_H_

atomic_t cal_request_iops = ATOMIC_INIT(0);
atomic_t cal_respond_iops = ATOMIC_INIT(0);

#endif /* CALCULATE_IOPS_PRIVATE_H_ */
