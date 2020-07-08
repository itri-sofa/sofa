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
#ifndef METABIOC_12009485199990000
#define METABIOC_12009485199990000
#define metabioc_printk(...)

extern struct bio_container *metabioc_build(lfsm_dev_t * td, struct HListGC *h,
                                            sector64 pbno);

extern void metabioc_build_page(struct EUProperty *eu, onDisk_md_t * buf_lpn,
                                bool isTail, int32_t idx);

extern int32_t metabioc_save_and_free(lfsm_dev_t * td,
                                      struct bio_container *metadata_bio_c,
                                      bool isActive);
extern bool metabioc_commit(lfsm_dev_t * td, onDisk_md_t * buf_metadata,
                            struct EUProperty *eu_new);

extern void metabioc_asyncload(lfsm_dev_t * td, struct EUProperty *ar_peu,
                               devio_fn_t * pcb);
extern bool metabioc_asyncload_bh(lfsm_dev_t * td, struct EUProperty *ar_peu,
                                  sector64 * ar_lbno, devio_fn_t * pcb);
extern bool metabioc_isempty(sector64 * ar_lbno);
extern sector64 metabioc_CalcChkSumB(sector64 * ar_lbno, sector64 checksum,
                                     int32_t num);
#endif
