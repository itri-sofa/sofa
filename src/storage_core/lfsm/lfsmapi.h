/*
 * SPDX-License-Identifier: GPL-2.0-or-later OR Apache-2.0
 * Copyright (C) 2015-2020 Industrial Technology Research Institute
 *
 *
 *
 *  
 */

#ifndef LFSMAPI_H_
#define LFSMAPI_H_

extern int32_t lfsm_init(lfsm_cfg_t * myCfg, int32_t ul_off, int32_t ronly,
                         int64_t lfsm_auth_key);
extern void lfsm_cleanup(lfsm_cfg_t * myCfg, bool isNormalExit);

#endif /* LFSMAPI_H_ */
