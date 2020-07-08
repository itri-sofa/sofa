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
#ifndef _MATALOG_H_
#define _MATALOG_H_

#define MLOG_BATCH_NUM    255
#define MLOG_BH_THREAD_NUM  2
#define MLOG_EU_NUM 2

#define MLOG_TAG_EU_ERASE ((sector64)0xFFFFFFFFC3C3C3C3ULL)

#define ARI(X,i,j,cn_col) ((X)[(i)*cn_col+(j)])

enum smetalog_type { ml_user = 0, ml_event = 1, ml_idle = 2 };

struct lfsm_dev_struct;

typedef struct smetalog_data {
    sector64 pbn;
    sector64 lpn;
} smetalog_data_t;

typedef struct smetalog_info    // PAGE_SIZE should be divisible by this struct size
{
    union {
        int32_t seq;
        smetalog_data_t x;
    };

} smetalog_info_t;

typedef struct smetalog_packet {
    struct list_head hook;
    struct bio_container *ar_pbioc[MLOG_BATCH_NUM];
    smetalog_info_t ar_info[MLOG_BATCH_NUM];
    struct smetalogger *pmlogger;
    atomic_t cn_pending;
    int32_t id_disk;
    int32_t tp;                 // smetalog_type, 0: packet of normal user, 1: packet of active eu when change metalog
    int32_t err;
} smetalog_pkt_t;

typedef struct smetalog_manager //smetalogger_thread
{
    struct task_struct *pthread;
    wait_queue_head_t wq;
    atomic_t cn_act;
    struct list_head list_act;
    spinlock_t slock;
} smetalog_mgr_t;

typedef struct smetalog_manager_fg  //smetalogger_thread
{
    spinlock_t slock;
    struct task_struct *pthread;
    wait_queue_head_t wq_enqueue;
    atomic_t cn_full;
    struct list_head list_full;
    wait_queue_head_t wq_dequeue;
    smetalog_pkt_t *ppacket_act;
    atomic_t state;             //1: on , 0: off -> 0 means one incomplete packet is written to log, should wait until its done.
    int32_t sz_submit;
    int32_t cn_submit;
    atomic_t cn_pkt_bf_swap;
    // Every active EU will be async-logged to metalog, so we use cn_pkt_bf_swap to determine whether the logging is finished.
    // When cn_pkt_bf_swap decrease to zero, metalog will be swapped, and old metalog EU will be discard. Tifa, 2013.10.07
} smetalog_mgr_fg_t;

typedef struct smetalog_eulog {
    struct EUProperty *eu[MLOG_EU_NUM];
    int32_t idx;
    int32_t seq;
} smetalog_eulog_t;

typedef struct smetalogger {
    //smetalog_eulog_t ar_eulog[LFSM_DISK_NUM];
    smetalog_eulog_t *par_eulog;
    struct kmem_cache *pmlpacket_pool;
    //struct kmem_cache *pmlhost_pool;

    //smetalog_mgr_fg_t mlmgr[LFSM_DISK_NUM];
    smetalog_mgr_fg_t *par_mlmgr;
    smetalog_mgr_t mlmgr_bh[MLOG_BH_THREAD_NUM];
    atomic_t cn_bh;
    atomic_t cn_host;
    struct lfsm_dev_struct *td;
    wait_queue_head_t wq;
} smetalogger_t;

////public
extern struct EUProperty *pbno_return_eu_idx(sector64 pbn, int32_t * ret_idx);
extern int32_t metalog_init(smetalogger_t * mlogger, struct HListGC **gc,
                            lfsm_dev_t * td);
extern int32_t metalog_eulog_reinit(smetalogger_t * pmlogger,
                                    int32_t cn_ssd_orig);

extern void metalog_destroy(smetalogger_t * mlogger);
extern void metalog_eulog_initA(smetalog_eulog_t * peulog, int32_t id_disk,
                                bool fg_insert_old);

extern int32_t metalog_packet_register(smetalogger_t * mlogger,
                                       struct bio_container *pbioc,
                                       sector64 pbn, sector64 lpn);
extern void metalog_eulog_get_all(smetalog_eulog_t * ar_eulog,
                                  sector64 * ar_eu_mlogger,
                                  int32_t lfsm_cn_ssds);
extern int32_t metalog_eulog_set_all(smetalog_eulog_t * ar_eulog,
                                     sector64 * ar_eu_mlogger,
                                     int32_t lfsm_cn_ssds, lfsm_dev_t * td);
extern int32_t metalog_create_thread_append(smetalogger_t * pmlogger,
                                            int32_t cn_ssd_orig);
extern int32_t metalog_create_thread_all(smetalogger_t * pmlogger);
extern int32_t metalog_eulog_reset(smetalogger_t * pmlogger,
                                   struct HListGC **gc, bool fg_insert_old);

extern int32_t metalog_load_all(smetalogger_t * mlogger, lfsm_dev_t * td);
extern int32_t metalog_grp_load_all(smetalogger_t * mlogger, lfsm_dev_t * td,
                                    struct rddcy_fail_disk *fail_disk);
extern void metalog_grp_eulog_rescue(smetalogger_t * mlogger, int32_t id_grp,
                                     int32_t fail_disk);
extern bool metalog_is_exist(struct EUProperty *eu);
extern int32_t metalog_packet_cur_eus_all(smetalogger_t * pmlogger,
                                          struct HListGC **gc);
extern int32_t metalog_packet_cur_eus(smetalogger_t * pmlogger, int32_t id_disk,
                                      struct HListGC **gc);

extern bool metalog_packet_flushed(smetalogger_t * pmlogger,
                                   struct HListGC **gc);

#endif
