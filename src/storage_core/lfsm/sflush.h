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
#ifndef __SFLUSH_H
#define __SFLUSH_H

struct lfsm_dev_struct;
#define ID_USER_AUTOFLUSH 0
#define ID_USER_CROSSCOPY 1
#define ID_USER_GCWRITE 2

typedef struct ssflush {
    int32_t id_user;
    struct list_head finish_r_list;
    struct list_head finish_w_list;

    spinlock_t finish_r_list_lock;
    spinlock_t finish_w_list_lock;

    atomic_t len_finish_read;
    atomic_t len_finish_write;
    atomic_t non_ret_bio_cnt;

    wait_queue_head_t wq_proc_finish_r;
    wait_queue_head_t wq_proc_finish_w;
    wait_queue_head_t wq_submitor;

    struct task_struct *pthread_proc_finish_r;
    struct task_struct *pthread_proc_finish_w;
    struct lfsm_dev_struct *ptd;
} ssflush_t;

extern int32_t sflush_gcwrite_handler(ssflush_t * ssf,
                                      struct bio_container *pBioc,
                                      bool isRetry);

extern int32_t sflush_copy_submit(lfsm_dev_t * td, sector64 source,
                                  sector64 dest, sector64 lpn, int32_t cn_page);
extern void sflush_open(sflushops_t * ops);

extern void sflush_end_bio_gcwrite_read(struct bio *bio);

extern void sflush_readphase_done(ssflush_t * ssf, struct bio_container *pBioc);

extern bool sflush_init(struct lfsm_dev_struct *td, struct sflushops *ops,
                        int32_t id_user);

extern int32_t dump_sflush_info(struct lfsm_dev_struct *td, int8_t * buffer);

extern void sflush_end_bio_gcwrite_read_brm(struct bio *bio);
extern bool IS_GCWrite_THREAD(sflushops_t * ops, int32_t pid);

extern bool sflush_waitdone(sflushops_t * ops);

extern bool sflush_destroy(sflushops_t * ops);

#define MAX_BIO_NUM 9000        // for testing
#define FLUSH_INTERVAL 2

//#define sf_printk(...)  printk("SFLUSH:");printk(__VA_ARGS__)
#define sf_printk(...)

#endif
