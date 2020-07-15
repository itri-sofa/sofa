/*
 * SPDX-License-Identifier: GPL-2.0-or-later OR Apache-2.0
 * Copyright (C) 2015-2020 Industrial Technology Research Institute
 *
 *
 *
 *  
 */

#ifndef CALCULATE_IOPS_H_
#define CALCULATE_IOPS_H_

#define ENABLE_CAL_IOPS

extern atomic_t cal_request_iops;
extern atomic_t cal_respond_iops;

#ifdef ENABLE_CAL_IOPS
#define INC_RESP_IOPS() increase_resp_iops()
#define INC_REQ_IOPS() increase_req_iops()
#else
#define INC_RESP_IOPS() {}
#define INC_REQ_IOPS() {}
#endif

extern void increase_resp_iops(void);
extern void increase_req_iops(void);

#endif /* CALCULATE_IOPS_H_ */
