/*
 * SPDX-License-Identifier: GPL-2.0-or-later OR Apache-2.0
 * Copyright (C) 2015-2020 Industrial Technology Research Institute
 *
 *
 *
 *  
 */

#ifndef LFSM_ERASE_COUNT_PRIVATE_H_
#define LFSM_ERASE_COUNT_PRIVATE_H_

int8_t *EU_usage_type[EU_END_TAG] = {
    "EU_FREE",
    "EU_DATA",
    "EU_UPDATELOG",
    "EU_DEDICATE",
    "EU_ERASETAB",
    "EU_HPEU",
    "EU_PMT",
    "EU_SUPERMAP",
    "EU_UNKNOWN",
    "EU_GC"
};

#endif /* LFSM_ERASE_COUNT_PRIVATE_H_ */
