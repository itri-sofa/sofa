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
#ifndef _BMT_PPDM_H_
#define _BMT_PPDM_H_

#define FACTOR_PAGEBMT2PPQ 1

//#define ppdm_printk(...)  printk("[%s] ", __FUNCTION__);printk(__VA_ARGS__)
#define ppdm_printk(...)

#define PBNO_SPECIAL_NOT_IN_CACHE 9
#define PBNO_SPECIAL_BAD_PAGE 8
#define PBNO_SPECIAL_NEVERWRITE 0

#if DYNAMIC_PPQ_LOCK_KEY == 1
#define PPDM_STATIC_KEY_NUM (8192*8*LFSM_DISK_NUM*2)    // AN:use external size ??
#else
#define PPDM_STATIC_KEY_NUM (8192*8*24*2)   // DN: dynamic key for ppq lock may be wrong. If get problem, use me
#endif

#define DBMTE_PENDING 2
#define DBMTE_NONPENDING 1
#define DBMTE_EMPTY 0

#define PAGEBMT_NON_ONDISK (~0)
//#define PPDM_PAGEBMT_printk(fmt, args...) printk(fmt, ##args)
#define PPDM_PAGEBMT_printk(fmt, args...)

// direct_bmt: the struct will be modified as only using pbno record with one(or twe) bit pending mark
/*========= Pubic Function =================*/
extern int32_t PPDM_BMT_dbmta_return(lfsm_dev_t * td, MD_L2P_table_t * bmt,
                                     int32_t cnt);

//extern bool PPDM_BMT_cache_drop(lfsm_dev_t *td, MD_L2P_table_t *bmt);

extern bool PPDM_BMT_cache_init(MD_L2P_table_t * bmt);
extern bool PPDM_BMT_cache_lookup(MD_L2P_table_t * bmt, sector64 lbno,
                                  bool acc_incr, struct D_BMT_E *pret_dbmta);
extern void PPDM_BMT_cache_insert_nonpending(lfsm_dev_t * td,
                                             MD_L2P_table_t * bmt,
                                             struct D_BMT_E *bmt_in,
                                             int32_t cnt, sector64 start_lbno);
extern int32_t PPDM_BMT_cache_insert_one_pending(lfsm_dev_t * td, sector64 lbno,
                                                 sector64 pbno,
                                                 int32_t rec_size,
                                                 bool to_remove_pin,
                                                 enum EDBMTA_OP_DIRTYBIT
                                                 op_dirty, bool is_fake_pbn);
extern void PPDM_BMT_dbmta_dump(MD_L2P_item_t * ppq);
extern int32_t PPDM_BMT_cache_remove_pin(MD_L2P_table_t * pbmt, sector64 lpn,
                                         int32_t cn_pins);

extern int32_t PPDM_BMT_dbmta_return_threshold(lfsm_dev_t * td,
                                               MD_L2P_table_t * bmt);
extern int32_t PPDM_PAGEBMT_Flush(lfsm_dev_t * td, MD_L2P_table_t * bmt,
                                  int32_t PPDM_idx, devio_fn_t ** pCb,
                                  int cmt_idx);
extern bool PPDM_PAGEBMT_rebuild(lfsm_dev_t * td, MD_L2P_table_t * bmt);
extern bool PPDM_PAGEBMT_CheckAndRunGC(lfsm_dev_t * td, MD_L2P_table_t * bmt);
extern bool PPDM_PAGEBMT_CheckAndRunGC_Reuse(lfsm_dev_t * td,
                                             MD_L2P_table_t * bmt);
extern bool PPDM_PAGEBMT_isFULL(MD_L2P_table_t * bmt);

extern bool PPDM_BMT_ppq_flush_return(lfsm_dev_t * td, MD_L2P_item_t * c_ppq);

extern bool PPDM_PAGEBMT_GCEU_disk_adjust(MD_L2P_table_t * bmt);
extern int32_t PPDM_BMT_load(lfsm_dev_t * td, int32_t idx_ppq,
                             int8_t * bmt_rec);

extern int32_t PPDM_PAGEBMT_reload_defect(lfsm_dev_t * td,
                                          MD_L2P_table_t * bmt);

#define READ_PER_BATCH 64

#endif //_BMT_PPDM_H_
