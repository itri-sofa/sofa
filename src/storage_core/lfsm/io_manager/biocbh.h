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
#ifndef BIOCBH_INC
#define BIOCBH_INC

//#define biocbh_printk(...) printk(__VA_ARGS__)
#define biocbh_printk(...)
#define MAX_BIOCBH_THREAD 64

#define HANDLE_FAIL_PROCESS_NUM 2

struct lfsm_dev_struct;
struct bio_container;

typedef struct bioc_bh {
    struct list_head ar_io_done_list[MAX_BIOCBH_THREAD];    /* list for IO done */
    struct list_head handle_fail_bioc_list; /* list for fail IO */
    spinlock_t ar_lock_biocbh[MAX_BIOCBH_THREAD];   /* protection of ar_io_done_list */
    spinlock_t lock_fail_bh;    /* protection of fail IO thread */
    wait_queue_head_t ar_wq[MAX_BIOCBH_THREAD]; /* wait queue to control lfsm_bh */
    wait_queue_head_t ehq;      /* wait queue to control fail IO */
    struct task_struct *pthread_bh[MAX_BIOCBH_THREAD];  /* kthread of lfsm_bh */
    struct task_struct *pthread_handle_fail_process[HANDLE_FAIL_PROCESS_NUM];   /* kthread of lfsm_fail */
    atomic_t ar_len_io_done[MAX_BIOCBH_THREAD]; /* counting of io done */
    atomic_t len_fail_bioc;     /* counting of io done */
    bool ready;
    atomic_t cn_thread_biocbh;
    atomic_t idx_curr_thread;
    /*
     *  LEGO: for reduce atomic operation (perf consideration),
     *        I use cnt_readIO_done for each bh thread and cnt_readNOPPNIO_done (No PPN IO) for
     *        iod thread (no metadata)
     */
    int32_t cnt_readIO_done[MAX_BIOCBH_THREAD];
    atomic_t cnt_readNOPPNIO_done;
    int32_t cnt_writeIO_done[MAX_BIOCBH_THREAD];
} biocbh_t;

extern bool biocbh_init(biocbh_t * pbiocbh, struct lfsm_dev_struct *td);
extern void biocbh_destory(biocbh_t * pbiocbh);

//extern static int biocbh_process(void* vtd);
extern void biocbh_io_done(biocbh_t * pbiocbh, struct bio_container *pBioc,
                           bool in_issued_list);
extern int32_t err_write_bioc_normal_path_handle(struct lfsm_dev_struct *td,
                                                 struct bio_container *pBioc,
                                                 bool isMetaData);

struct bad_block_info;
extern void put_err_bioc_to_err_list(biocbh_t * pbiocbh,
                                     struct bio_container *pBioc);
#endif
