/*
 * SPDX-License-Identifier: GPL-2.0-or-later OR Apache-2.0
 * Copyright (C) 2015-2020 Industrial Technology Research Institute
 *
 *
 *
 *  
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
