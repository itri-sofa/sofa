/*
 * Copyright (c) 2015-2025 Industrial Technology Research Institute.
 *
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */
/*
** This is the header file for EU_create.h file
** Only the function prototypes declared below are exported to be used by other files
*/

#ifndef	EU_CREATE_H
#define	EU_CREATE_H

struct lfsm_dev_struct;
struct HListGC;
struct diskgrp;

extern void dummy_write(struct EUProperty *);
extern bool EU_init(struct lfsm_dev_struct *td, struct HListGC *, int32_t,
                    sector64, int32_t, int32_t, sector64, uint64_t, int32_t);
extern bool categorize_EUs_based_on_bitmap(struct lfsm_dev_struct *td);
extern int32_t EU_destroy(struct lfsm_dev_struct *);
extern int32_t flush_metadata_of_active_eu(struct lfsm_dev_struct *,
                                           struct HListGC *,
                                           struct EUProperty *);
extern int32_t EU_param_init(struct EUProperty *eu, int32_t eu_usage,
                             int32_t log_frontier);

extern int32_t EU_erase(struct EUProperty *victim_EU);
extern int32_t EU_erase_if_undone(struct HListGC *h, struct EUProperty *eu_ret);

extern void EU_cleanup(struct lfsm_dev_struct *);
extern bool active_eu_init(struct lfsm_dev_struct *td, struct HListGC *h);
extern int32_t system_eu_rescue(struct lfsm_dev_struct *td,
                                int32_t id_fail_disk);

extern int32_t EU_close_active_eu(struct lfsm_dev_struct *td,
                                  struct HListGC *h,
                                  int32_t(*move_func) (struct HListGC * h,
                                                       struct EUProperty * eu));

extern int32_t EU_spare_read_unit(struct lfsm_dev_struct *td, int8_t * buffer,
                                  sector64 addr_main, sector64 addr_bkup,
                                  int32_t size);
extern bool EU_spare_read_A(struct lfsm_dev_struct *td, int8_t * buffer,
                            sector64 addr_main, sector64 addr_bkup,
                            int32_t num_pages, int8_t * ar_isbkup);
extern bool EU_spare_read(struct lfsm_dev_struct *td, int8_t * buffer,
                          sector64 addr, int32_t num_pages);

#endif
