/*
 * SPDX-License-Identifier: GPL-2.0-or-later OR Apache-2.0
 * Copyright (C) 2015-2020 Industrial Technology Research Institute
 *
 *
 *
 *  
 */

#include <linux/jiffies.h>
#include <linux/types.h>

#include "calculate_iops_private.h"
#include "calculate_iops.h"

void increase_resp_iops(void)
{
    atomic_inc(&cal_respond_iops);
}

void increase_req_iops(void)
{
    atomic_inc(&cal_request_iops);
}
