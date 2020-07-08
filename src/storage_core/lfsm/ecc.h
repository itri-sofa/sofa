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

#ifndef _ECC_H_
#define _ECC_H_

#define CN_REBUILD_BATCH 5000
#define LBNO_PREPADDING_PAGE 0x00676e696461 // 6 bytes
#define LBNO_REDO_PAGE         0xFF7777777777
#define RET_STRIPE_WAIT_AGAIN 1
#define STRIPE_CHUNK_SIZE 128   // 2*4K = 8K bytes
struct swrite_param {
    sector64 pbno_new;
    sector64 pbno_orig;
    sector64 pbno_meta;
    sector64 lbno;
    int32_t id_disk;
    struct bio_container *bio_c;
};

struct external_ecc {
    struct EUProperty *eu_main; // ecc eu
    struct mutex ecc_lock;
    int8_t state;               // indecate the external ecc disk is good
};

struct external_ecc_head {
    sector64 external_ecc_head; /* start pos of external ecc eu (sector base) */
    int32_t external_ecc_frontier;  /* log frontier of external ecc eu (sector base) */
};

struct stripe_disk_map {
    int8_t len;
    int32_t disk_num[];
};

struct on_disk_metadata;
struct lfsm_stripe;
struct lfsm_stripe_w_ctrl;

#define WPARAM_ID_PREPAD 0
#define WPARAM_ID_DATA 1
#define WPARAM_ID_ECC 2
#define WPARAM_CN_IDS 3

extern int32_t rddcy_end_bio_write_helper(struct bio_container *entry,
                                          struct bio *bio);
//bool rddcy_insert_metadata_to_eu(lfsm_dev_t *td, struct on_disk_metadata *buf_metadata,
//        struct EUProperty *eu_new, int32_t count);

extern bool rddcy_xor_flash_buff_op(struct lfsm_stripe *cur_stripe,
                                    struct bio_vec *page_vec, sector64 lbno);
extern bool rddcy_end_stripe(struct lfsm_dev_struct *td,
                             struct bio_container *pBioc);
//bool rddcy_resend_ecc(lfsm_dev_t *td, struct bio_container *pBioc);
void rddcy_wait_complete(lfsm_dev_t * td, struct bio_container *pBioc);

extern int32_t rddcy_fpage_read(lfsm_dev_t * td, sector64 pbno,
                                struct bio_container *bioc, int32_t idx_fpage);
extern int32_t rddcy_rebuild_fail_disk(struct HPEUTab *hpeutab,
                                       diskgrp_t * pgrp,
                                       struct rddcy_fail_disk *fail_disk);
extern int32_t rddcy_rebuild_all(struct HPEUTab *hpeutb, struct diskgrp *pgrp);
extern int32_t rddcy_rebuild_hpeus_data(struct HPEUTab *hpeutb,
                                        struct diskgrp *pgrp);

extern int32_t rddcy_rebuild_hpeu(struct HPEUTab *hpeutab, int32_t id_hpeu,
                                  struct rddcy_fail_disk *fail_disk);
extern int32_t rddcy_read_metadata(lfsm_dev_t * td, struct EUProperty *peu,
                                   sector64 * ar_lbno);
extern bool rddcy_gen_bioc(lfsm_dev_t * td, struct lfsm_stripe *cur_stripe,
                           struct swrite_param *pret_wparam, bool renew_stripe);
extern int32_t rddcy_search_failslot(struct diskgrp *pgrp);
extern int32_t rddcy_search_failslot_by_pg_id(struct diskgrp *pgrp,
                                              int32_t select_pg_id);
extern int32_t rddcy_monitor_init(lfsm_dev_t * td);
extern int32_t rddcy_monitor_destroy(lfsm_dev_t * td);
extern int32_t rddcy_rebuild_hpeus_A(struct HPEUTab *hpeutab,
                                     struct diskgrp *pgrp, int32_t id_faildisk);
extern int32_t rddcy_rebuild_metadata(struct HPEUTab *phpeutab,
                                      sector64 start_pos,
                                      struct rddcy_fail_disk *fail_disk,
                                      sector64 ** p_metadata);
//bool rddcy_calc_xor_outside(struct bio_container *ecc_bioc);

static inline void xor_block(uint8_t * dest, uint8_t * src, size_t len)
{
    int32_t i;
    uint64_t *d = (uint64_t *) dest;
    uint64_t *s = (uint64_t *) src;
    len >>= 3;
    for (i = 0; i < len; i++) {
        d[i] ^= s[i];
    }
}

static inline int32_t *rddcy_get_stripe_map(struct stripe_disk_map *map,
                                            int32_t idx,
                                            int32_t cn_ssds_per_hpeu)
{
    return &map->disk_num[((idx / STRIPE_CHUNK_SIZE) % cn_ssds_per_hpeu) *
                          cn_ssds_per_hpeu];
}

extern int32_t rddcy_pgroup_num_get(int8_t * buffer, int32_t size);
extern int32_t rddcy_pgroup_size_get(int8_t * buffer, int32_t size);
extern int32_t rddcy_faildrive_get(int8_t * buffer, int32_t size);

extern int32_t rddcy_faildrive_count_get(int32_t id);
extern void rddcy_fail_ids_to_disk(int32_t cn_disk, int32_t id_failslot,
                                   struct rddcy_fail_disk *fail_disk);
extern int32_t ExtEcc_eu_init(lfsm_dev_t * td, int32_t idx, bool fg_insert_old);
extern int32_t ExtEcc_eu_set_mdnode(lfsm_dev_t * td);
extern int32_t ExtEcc_eu_set(lfsm_dev_t * td,
                             struct sddmap_ondisk_header *pheader);
extern void ExtEcc_destroy(lfsm_dev_t * td);
extern int32_t ExtEcc_get_dest_pbno(lfsm_dev_t * td,
                                    struct lfsm_stripe *pstripe,
                                    bool isValidData, struct swrite_param *pwp);
extern int32_t ExtEcc_get_stripe_size(struct lfsm_dev_struct *td,
                                      sector64 start_pos, int32_t row,
                                      sector64 * extecc_pbno,
                                      struct rddcy_fail_disk *fail_disk);

extern int32_t rddcy_check_hpeus_data_ecc(struct HPEUTab *hpeutab,
                                          diskgrp_t * pgrp);

extern int32_t init_stripe_disk_map(lfsm_dev_t * td, int32_t ssds_per_hpeu);
extern void free_stripe_disk_map(lfsm_dev_t * td, int32_t ssds_per_hpeu);
#define for_rddcy_faildrive_get(ids, disk, i) for(i = 0, disk = (ids >> (i << 3)) & 0xff; i < rddcy_faildrive_count_get(ids); disk = (ids >> (++i << 3)) & 0xff)

#define RDDCY_ECC_MARKER_BIT_NUM 4
#define RDDCY_ECC_BIT_OFFSET (ADDR_MODE + 1 - RDDCY_ECC_MARKER_BIT_NUM) // 3 bit lest
#define RDDCY_ECC_BIT_MASK ( (sector64)((1<<RDDCY_ECC_MARKER_BIT_NUM)-1)<< RDDCY_ECC_BIT_OFFSET)
#define RDDCY_IS_ECCLPN(X) ((X&RDDCY_ECC_BIT_MASK)>0)
#define RDDCY_CLEAR_ECCBIT(X)  ( X &(~RDDCY_ECC_BIT_MASK))
#define RDDCY_ECCBIT(L)  ( ((sector64)L) << RDDCY_ECC_BIT_OFFSET)
#define RDDCY_GET_STRIPE_SIZE(LPN)  ((LPN & RDDCY_ECC_BIT_MASK ) >> RDDCY_ECC_BIT_OFFSET)
#endif
