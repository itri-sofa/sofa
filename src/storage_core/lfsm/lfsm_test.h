/*
 * SPDX-License-Identifier: GPL-2.0-or-later OR Apache-2.0
 * Copyright (C) 2015-2020 Industrial Technology Research Institute
 *
 *
 *
 *  
 */
#ifndef __LFSM_TEST_H
#define __LFSM_TEST_H

#include "ioctl_A.h"
typedef int (*fp)(lfsm_dev_t * td, struct EUProperty *,
                  sftl_err_msg_t * perr_msg);

extern int32_t check_allEU_valid_lpn(lfsm_dev_t * td);
extern int32_t check_allEU_valid_lpn_new(lfsm_dev_t * td);

extern int32_t check_allEU_EUmeta_spare(lfsm_dev_t * td);

extern int32_t get_metadata_with_check(lfsm_dev_t * td,
                                       struct EUProperty *src_eu,
                                       sftl_err_msg_t * perr_msg);

extern void print_eu_erase_count(struct HListGC *h);
extern void check_mdlist_and_sparelpn(lfsm_dev_t * td, struct EUProperty *eu,
                                      struct bio *mbio);
extern int32_t check_all_lpn(lfsm_dev_t * td);
extern int32_t get_metadata_with_check_new(lfsm_dev_t * td,
                                           struct EUProperty *src_eu,
                                           sftl_err_msg_t * perr_msg);
extern int32_t check_one_lpn(lfsm_dev_t * td, sector64 lpn,
                             sftl_err_msg_t * perr_msg);
extern int32_t check_eu_idhpeu(lfsm_dev_t * td, struct EUProperty *eu,
                               sftl_err_msg_t * perr_msg);
extern int32_t check_hpeu_table(lfsm_dev_t * td, int32_t id_hpeu,
                                sftl_err_msg_t * perr_msg);
extern int32_t check_eu_mdnode(lfsm_dev_t * td, struct EUProperty *eu,
                               sftl_err_msg_t * perr_msg);
extern int32_t check_eu_bitmap_validcount(lfsm_dev_t * td,
                                          struct EUProperty *eu,
                                          sftl_err_msg_t * perr_msg);
extern int32_t check_disk_eu_state(lfsm_dev_t * td, int32_t id_disk,
                                   sftl_err_msg_t * perr_msg);
extern int32_t check_active_eu_frontier(lfsm_dev_t * td, int32_t useless,
                                        sftl_err_msg_t * perr_msg);
extern int32_t check_sys_table(lfsm_dev_t * td, int32_t useless,
                               sftl_err_msg_t * perr_msg);
extern int32_t check_freelist_bioc(lfsm_dev_t * td, int32_t id_bioc,
                                   sftl_err_msg_t * perr_msg);
extern int32_t check_freelist_bioc_simple(lfsm_dev_t * td, int32_t useless,
                                          sftl_err_msg_t * perr_msg);
extern int32_t check_systable_hpeu(lfsm_dev_t * td, int32_t useless,
                                   sftl_err_msg_t * perr_msg);
#endif
