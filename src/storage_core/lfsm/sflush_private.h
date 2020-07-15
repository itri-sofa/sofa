/*
 * SPDX-License-Identifier: GPL-2.0-or-later OR Apache-2.0
 * Copyright (C) 2015-2020 Industrial Technology Research Institute
 *
 *
 *
 *  
 */

#ifndef SFLUSH_PRIVATE_H_
#define SFLUSH_PRIVATE_H_

static int32_t sflush_exec(lfsm_dev_t * td, struct sflushops *ops,
                           sector64 s_lpno, int32_t len);
static int32_t sflush_estimate(MD_L2P_item_t * ar_ppq, sector64 lpno_s,
                               int32_t page_num);
struct sflushops ssflushops = {
    .name = "SFLUSH",
    .variables = NULL,
    .init = sflush_init,
    .estimate = sflush_estimate,
    .exec = sflush_exec,
    .waitdone = sflush_waitdone,
    .destroy = sflush_destroy
};

#endif /* SFLUSH_PRIVATE_H_ */
