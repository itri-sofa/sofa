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
#ifndef _IO_WRITE_H_
#define _IO_WRITE_H_

#define REGET_DESTPBNO       98
#define RET_BIOCBH          94

#define HEADER 1
#if HEADER

#define HEAD_PAGE_UNALIGN_MASK 0x2
#define TAIL_PAGE_UNALIGN_MASK 0x1
#define HEAD_PAGE_UNALIGN(X) (((X)&HEAD_PAGE_UNALIGN_MASK)>0)
#define TAIL_PAGE_UNALIGN(X) (((X)&TAIL_PAGE_UNALIGN_MASK)>0)

#endif

typedef int32_t(*func_sync_t) (lfsm_dev_t * td, struct bio_container * bioc,
                               int32_t lpn_start, int32_t idx_fpage,
                               bool isForceUpdate);
extern int32_t bioc_data_cache_update(lfsm_dev_t * td,
                                      struct bio_container *bioc);

extern struct EUProperty *change_active_eu_A(lfsm_dev_t * td, struct HListGC *h,
                                             int32_t temper, bool withOld);
extern int32_t iwrite_bio(lfsm_dev_t * td, struct bio *bio);
extern int32_t get_disk_num(sector64 pbno);
extern int32_t bioc_submit_write(lfsm_dev_t * td, struct bio_container *bio_c,
                                 bool isRetry);

extern int32_t bioext_locate(bio_extend_t * pbioext, struct bio *pbio,
                             int32_t len);
extern void bioext_free(bio_extend_t * pbioext, int32_t len);

extern int32_t iwrite_bio_bh(lfsm_dev_t * td, struct bio_container *bio_c,
                             bool isRetry);
extern int32_t update_old_eu(struct HListGC *h, int32_t size,
                             sector64 * old_pbno);
extern void end_bio_write(struct bio *bio);
extern void end_bio_partial_write_read(struct bio *bio);
extern int32_t retry_special_read_enquiry(lfsm_dev_t * td, sector64 bi_sector,
                                          int32_t page_num,
                                          int32_t org_cmd_type, int32_t BlkUnit,
                                          struct bad_block_info *bad_blk_info);
extern int32_t process_copy_write_fail(struct bad_block_info *bad_blk_info);

extern int32_t update_bio_spare_area(struct bio_container *bio_ct,
                                     sector64 * ar_lbn, int32_t num_lbns);
extern int32_t get_dest_pbno(lfsm_dev_t * td, struct lfsm_stripe *pstripe,
                             bool isValidData, struct swrite_param *pwp);

extern int32_t rddcy_write_submit(lfsm_dev_t * td, struct swrite_param *pwp);

int32_t iwrite_bioc_padding_write(lfsm_dev_t * td, int32_t id_temp,
                                  int32_t id_pgroup);
extern bool bioc_chk_is_latest(struct bio_container *pbioc);
extern void bioc_dump(struct bio_container *pbioc);
extern int32_t rddcy_wparam_setup_helper(lfsm_dev_t * td,
                                         struct lfsm_stripe *cur_stripe,
                                         struct swrite_param *pm_act,
                                         struct swrite_param *pm_ecc,
                                         int32_t idx_fpage);

extern struct swrite_param *rddcy_wparam_setup(lfsm_dev_t * td,
                                               struct lfsm_stripe *cur_stripe,
                                               struct swrite_param *pm_data,
                                               struct swrite_param *pm_prepad,
                                               bool isRetry,
                                               struct bio_container *bio_c,
                                               int32_t temper,
                                               int32_t idx_fpage,
                                               bool isDataPage);

#endif
