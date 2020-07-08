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

#ifndef HPEU_INC_12093801298309123
#define HPEU_INC_12093801298309123
#include "persistlog.h"

// calc XOR mechanism flag   

//LFSM_XOR_METHOD 0 -> no xor
//LFSM_XOR_METHOD 1 -> inside
//LFSM_XOR_METHOD 2 -> outside
//LFSM_XOR_METHOD 3 -> both and compare
#define LFSM_XOR_METHOD  2

// KERNEL_XOR : USE kernel xor API or use lfsm's xor api
#define KERNEL_XOR  0

struct lfsm_dev_struct;
struct SPssLog;

typedef struct HPEUTab {
    int32_t cn_total;           /* total # of hpeu in table number of EU in a disk * number of protection group */
    int32_t cn_used;
    void *ar_item;              /* pointer array which each entry is HPEUTabItem_t */
    int32_t curri;
    spinlock_t lock;
    struct lfsm_dev_struct *td; /* lfsm device */
    struct SPssLog pssl;        /* persist log for recording which EU in hpeu table */
    atomic_t cn_damege;
    int32_t cn_disk;            /* # of disks in HEPU table */
} HPEUTab_t;

enum ehpeu_state { ERROR = -1, damege, good };

typedef struct HPEUTabItem {
    int32_t cn_added;           /* # of disks in a hpeu table */
    enum ehpeu_state state;
    sector64 birthday;
    atomic_t cn_unfinish_fpage;
    struct EUProperty *ar_peu[0];   /* SEU array of a table item (array size = # of disks in hpeu table) */
} HPEUTabItem_t;

#define MAX_FAIL_DISK 1
struct rddcy_fail_disk {
    int32_t pg_id;              // fail disk pgroup id
    int32_t idx[MAX_FAIL_DISK]; // fail disk index of pgroup
    int32_t count;              // fail disks count
};

// Public Function:
#define HPEU_EU2HPEU_OFF(X,Y) ((Y)*lfsmdev.hpeutab.cn_disk+(X))
#define HPEU_HPEUOFF2EUIDX(X) ((X)%lfsmdev.hpeutab.cn_disk)
#define HPEU_HPEUOFF2EUOFF(X) ((X)/lfsmdev.hpeutab.cn_disk)
#define HPEU_ID_FIRST_DISK(X) LFSM_FLOOR(X,lfsmdev.hpeutab.cn_disk)

#define HPEU_EU2HPEU_FRONTIER(IDX_EU,EU_FRONTIER) ((EU_FRONTIER-SECTORS_PER_FLASH_PAGE)*lfsmdev.hpeutab.cn_disk+((IDX_EU+1)<<SECTORS_PER_FLASH_PAGE_ORDER))

extern int32_t HPEU_re_init(HPEUTab_t * hpeutab, int32_t cn_total_hpeu,
                            struct lfsm_dev_struct *td,
                            int32_t cn_ssds_per_hpeu);

extern bool HPEU_init(HPEUTab_t * hpeutab, int32_t cn_total_hpeu,
                      struct lfsm_dev_struct *td, int32_t cn_ssds_per_hpeu);
extern bool HPEU_destroy(HPEUTab_t * hpeutab);

extern sector64 HPEU_get_frontier(sector64 pbno);
extern sector64 HPEU_get_birthday(HPEUTab_t * hpeutab, struct EUProperty *peu);

extern bool HPEU_is_older_pbno(sector64 prev, sector64 curr);

extern bool HPEU_free(HPEUTab_t * hpeutab, int32_t id_hpeu);    // delete a hpeu by id,
extern int32_t HPEU_get_eus(HPEUTab_t * hpeutab, int32_t id_hpeu, struct EUProperty **ret_ar_peu);  //return start_position of EUs included in the HPEU
extern void HPEU_get_eupos(HPEUTab_t * hpeutab, sector64 * pmain,
                           sector64 * pbackup, int32_t * pidx_next);

//void HPEU_set_eupos(HPEUTab_t *hpeutab,sector64 main, sector64 backup);
extern bool HPEU_force_update(HPEUTab_t * hpeutab, int32_t id_hpeu,
                              sector64 pos_eu, sector64 birthday);
extern void HPEU_reset_birthday(HPEUTab_t * hpeutab);

extern int32_t HPEU_is_younger(HPEUTab_t * hpeutab, sector64 prev,
                               sector64 curr, sector64 lpn);

extern int32_t HPEU_iterate(HPEUTab_t * hpeutab, int32_t id_faildisk,
                            int32_t(*func) (struct EUProperty * eu,
                                            int32_t usage));

extern bool HPEU_save(HPEUTab_t * hpeutab);
extern bool HPEU_load(HPEUTab_t * hpeutab, sector64 pos_main,
                      sector64 pos_backup, int32_t idx_next);
extern bool HPEU_alloc_log_EU(HPEUTab_t * hpeutab, int32_t id_dev0,
                              int32_t id_dev1);
extern void HPEU_sync_all_eus(HPEUTab_t * hpeutab);
extern int32_t HPEU_get_stripe_pbnos(HPEUTab_t * hpeutab, int32_t id_hpeu,
                                     sector64 pbno, sector64 * ar_stripe_pbno,
                                     int32_t * id_fail_trunk);
extern enum ehpeu_state HPEU_get_state(HPEUTab_t * hpeutab, sector64 pbno);
extern int32_t HPEU_set_allstate(HPEUTab_t * hpeutab, int32_t id_faildisk,
                                 enum ehpeu_state state);
extern int32_t HPEU_set_state(HPEUTab_t * hpeutab, int32_t id_hpeu,
                              enum ehpeu_state state);
extern bool HPEU_turn_fixed(HPEUTab_t * phpeutab, int32_t id_hpeu,
                            int32_t id_faildisk, struct EUProperty *eu_new);
extern bool HPEU_smartadd_eu(HPEUTab_t * hpeutab, struct EUProperty *eu_new,
                             int32_t * ref_id_hpeu);
extern int32_t hpeu_find_ecc_lpn(sector64 ** p_metadata, int32_t idx_curr,
                                 int32_t director, int32_t * ret_len,
                                 int32_t cn_ssds_per_hpeu);
extern int32_t hpeu_get_eu_frontier(sector64 ** p_metadata, int32_t id_faildisk,
                                    int32_t cn_ssds_per_hpeu);
extern int32_t HPEU_read_metadata(lfsm_dev_t * td, struct EUProperty **ar_peu,
                                  sector64 ** p_metadata,
                                  struct rddcy_fail_disk *fail_disk);
extern int32_t HPEU_get_pgroup_id(HPEUTab_t * hpeutab, int32_t id_hpeu);
extern int32_t HPEU_cn_damage_get(int8_t * buffer, int32_t size);
extern void HPEU_change_next(HPEUTab_t * hpeutab);
extern int32_t HPEU_eunext_init(HPEUTab_t * hpeutab);

extern int32_t HPEU_metadata_smartread(lfsm_dev_t * td, int32_t id_hpeu,
                                       struct EUProperty **ar_seu,
                                       sector64 ** ar_lpn);
extern int32_t HPEU_metadata_commit(lfsm_dev_t * td, struct EUProperty **ar_seu,
                                    sector64 ** ar_lpn);
extern HPEUTabItem_t *HPEU_GetTabItem(HPEUTab_t * hpeutab, int32_t idx);
extern int32_t hpeu_get_maxcn_data_disk(int32_t cn_ssds_per_hpeu);
extern int32_t HPEU_dec_unfinish(HPEUTab_t * hpeutab,
                                 sector64 pbno_finished_write);
extern int32_t HPEU_is_unfinish(HPEUTab_t * phpeutab, int32_t id_hpeu);

extern bool HPEU_rescue(HPEUTab_t * hpeutab);

int HPEU_dump(char *buffer, char **buffer_location, off_t offset,
              int32_t buffer_length, int32_t * eof, void *data);
#endif // HPEU_INC_12093801298309123
