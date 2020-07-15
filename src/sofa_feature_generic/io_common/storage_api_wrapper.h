/*
 * SPDX-License-Identifier: GPL-2.0-or-later OR Apache-2.0
 * Copyright (C) 2015-2020 Industrial Technology Research Institute
 *
 *
 *
 *  
 */

#ifndef IO_COMMON_STORAGE_API_WRAPPER_H_
#define IO_COMMON_STORAGE_API_WRAPPER_H_

extern int32_t storage_set_config(uint32_t type, uint32_t val, int8_t * valStr,
                                  int8_t * setting, uint32_t set_len);
extern int32_t storage_init(void);
extern int32_t submit_IO_request(uint64_t sect_s, uint32_t sect_len,
                                 struct bio *mybio);
extern int32_t submit_MD_request(uint64_t lb_src, uint32_t lb_len,
                                 uint64_t lb_dest);
extern uint64_t get_storage_capacity(void);
extern uint32_t get_gc_seu_resv(void);

extern uint64_t get_physicalCap(void);
extern uint32_t get_groups(void);
extern int8_t get_groupdisks(uint32_t groupId, int8_t * gdisk_msg);

extern int8_t get_sparedisks(int8_t * spare_msg);

extern uint64_t cal_resp_iops_lfsm(void);
extern uint64_t cal_req_iops_lfsm(void);

#endif /* IO_COMMON_STORAGE_API_WRAPPER_H_ */
