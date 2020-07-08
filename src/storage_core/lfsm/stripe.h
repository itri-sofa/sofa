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
#ifndef SSTRIPE_INC_12980312893
#define SSTRIPE_INC_12980312893
#include "lfsm_core.h"
#include "hpeu.h"
#include "devio.h"

typedef struct lfsm_stripe_w_ctrl_Ext_buf {
    int8_t *pbuf_xor;
    atomic_t wio_cn;
} lfsm_stripe_buf_t;

struct lfsm_stripe_w_ctrl {
    atomic_t cn_completed;
    atomic_t error;
    atomic_t cn_allocated;
    atomic_t cn_finish;
    struct lfsm_stripe *psp;
    lfsm_stripe_buf_t *pbuf;
    int32_t cn_ExtEcc;
    int32_t isEndofStripe;

#if LFSM_XOR_METHOD ==2  || LFSM_XOR_METHOD ==3
    struct page *ar_page[(1 << RDDCY_ECC_MARKER_BIT_NUM)][MEM_PER_FLASH_PAGE];
#endif
};

typedef struct lfsm_stripe {
    lfsm_stripe_buf_t *pbuf;
    struct mutex buf_lock;
    sector64 xor_lbns;
    sector64 start_pbno;
    struct lfsm_stripe_w_ctrl *wctrl;
    struct mutex lock;
    int32_t id_hpeu;
    int32_t id_pgroup;
    atomic_t wio_cn;
    atomic_t idx;
    atomic_t cn_ExtEcc;
} lfsm_stripe_t;

typedef struct stask_stripe_rebuild {
    int len;
    sector64 lpn;
    devio_fn_t cb_write;
    devio_fn_t arcb_read[0];    //[DATA_DISK_PER_HPEU];
} stsk_stripe_rebuild_t;

struct status_ExtEcc_recov {
    sector64 start_pbno;
    sector64 new_start_pbno;
    int32_t idx;
    int32_t ecc_idx;
    int8_t s_ExtEcc;
};

extern int32_t stripe_async_read(lfsm_dev_t * td, struct EUProperty **ar_peu,
                                 sector64 ** ar_lbno, int32_t * map,
                                 int32_t start, int32_t len,
                                 int32_t id_faildisk,
                                 stsk_stripe_rebuild_t * ptsr,
                                 int32_t maxcn_data_disk,
                                 struct status_ExtEcc_recov *status_ExtEcc);
extern bool stripe_locate(lfsm_dev_t * td, sector64 start_pos,
                          sector64 ** p_metadata,
                          struct rddcy_fail_disk *fail_disk, int32_t start,
                          int32_t * ret_len, int32_t cn_ssds_per_hpeu,
                          struct status_ExtEcc_recov *status_ExtEcc);
extern int32_t stripe_async_rebuild(lfsm_dev_t * td, sector64 pbno,
                                    stsk_stripe_rebuild_t * ptsr,
                                    int8_t ** buf_fpage, int8_t * buf_xor,
                                    struct status_ExtEcc_recov *status_ExtEcc);
extern bool stripe_init(lfsm_stripe_t * psp, int32_t id_pgroup);
extern bool stripe_data_init(lfsm_stripe_t * psp, bool isExtEcc);
extern int32_t stripe_current_disk(lfsm_stripe_t * psp);
extern void stripe_commit_disk_select(lfsm_stripe_t * psp);
extern bool stripe_is_last_page(lfsm_stripe_t * psp, int32_t maxcn_data_disk);
extern void stripe_destroy(lfsm_stripe_t * psp);

#endif
