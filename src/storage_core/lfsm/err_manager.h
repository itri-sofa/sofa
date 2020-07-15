/*
 * SPDX-License-Identifier: GPL-2.0-or-later OR Apache-2.0
 * Copyright (C) 2015-2020 Industrial Technology Research Institute
 *
 *
 *
 *  
 */
#ifndef _ERRMGR_H_
#define _ERRMGR_H_

extern void errmgr_wait_falselist_clean(void);
extern void errmgr_bad_process(sector64 vic_pbno_start, page_t vic_lpn_start,
                               page_t vic_lpn_end, lfsm_dev_t * td,
                               bool isMetadata, bool to_remove_pin);
extern void move_to_FalseList(struct HListGC *h, struct EUProperty *vic_eu);

#endif // _ERRMGR_H_
