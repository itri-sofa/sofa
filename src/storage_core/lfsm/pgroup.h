/*
 * SPDX-License-Identifier: GPL-2.0-or-later OR Apache-2.0
 * Copyright (C) 2015-2020 Industrial Technology Research Institute
 *
 *
 *
 *  
 */
#ifndef PGROUP_INC_19082039182039
#define PGROUP_INC_19082039182039

// protect group of disks

//#define LFSM_PGROUP_NUM (LFSM_DISK_NUM/DISK_PER_HPEU)
#define PGROUP_ID(ID_DISK) ((ID_DISK)/lfsmdev.hpeutab.cn_disk)
extern atomic_t lfsm_arpgroup_wcn[TEMP_LAYER_NUM];
extern atomic_t lfsm_pgroup_cn_padding_page;
enum epgroup_state { epg_ERROR = -1, epg_damege, epg_good, epg_limit };

typedef struct spgroup {
    enum epgroup_state state;   /* status of protection group */
    lfsm_stripe_t stripe[TEMP_LAYER_NUM];
    wait_queue_head_t wq;
    int32_t min_id_disk;
    atomic64_t ar_birthday[TEMP_LAYER_NUM];
    atomic64_t cn_valid_page;
    sector64 ideal_valid_page;
    sector64 max_valid_page;
    atomic_t cn_gcing;
} spgroup_t;

extern sector64 lfsm_pgroup_birthday_next(lfsm_dev_t * td, int32_t id_pgroup,
                                          int32_t temperature);

extern int32_t lfsm_pgroup_init(lfsm_dev_t * td);
extern int32_t lfsm_pgroup_selector(spgroup_t * ar_grp, int32_t temperature,
                                    sector64 pbno_orig,
                                    struct bio_container *bio_c, bool isRetry);

extern struct lfsm_stripe *lfsm_pgroup_stripe_lock(lfsm_dev_t * td,
                                                   int32_t id_pgroup,
                                                   int32_t temperature);
extern int32_t *lfsm_pgroup_retref_hpeuid(lfsm_dev_t * td, int32_t id_disk,
                                          int32_t temperature);
extern int32_t lfsm_pgroup_set_hpeu(lfsm_dev_t * td, int32_t id_disk,
                                    int32_t temperature, int32_t id_hpeu);
extern int32_t lfsm_pgroup_get_temperature(lfsm_dev_t * td,
                                           struct lfsm_stripe *psp);
extern void lfsm_pgroup_destroy(lfsm_dev_t * td);

extern int32_t pgroup_disk_all_active(int32_t id_pgroup, struct diskgrp *pgrp,
                                      int32_t cn_ssds_per_hpeu);
extern int32_t pgroup_birthday_reassign(spgroup_t * ar_grp, int32_t id_pgroup);

extern void lfsm_pgroup_valid_page_init(spgroup_t * ar_grp,
                                        sector64 logi_capacity,
                                        sector64 plus_capacity);

extern struct kmem_cache *ppool_wctrl;

#endif
