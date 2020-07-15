/*
 * SPDX-License-Identifier: GPL-2.0-or-later OR Apache-2.0
 * Copyright (C) 2015-2020 Industrial Technology Research Institute
 *
 *
 *
 *  
 */

#ifndef PGROUP_PRIVATE_H_
#define PGROUP_PRIVATE_H_

atomic_t lfsm_arpgroup_wcn[TEMP_LAYER_NUM];
spinlock_t lfsm_pgroup_birthday_lock;

atomic_t lfsm_pgroup_cn_padding_page;
struct kmem_cache *ppool_wctrl = NULL;

#endif /* PGROUP_PRIVATE_H_ */
