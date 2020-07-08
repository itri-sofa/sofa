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
#ifndef BIOBOXPOOL_INC
#define BIOBOXPOOL_INC

//#define bbp_printk(...) printk(__VA_ARGS__)

#define bbp_printk(...)
#define BIOBOX_TIMEOUT 1000

#define LFSM_BIOBOX_STAT 0
#define LFSM_NUM_BIOBOX 20480
#define BIOBOX_BIGPRIMARY 179426549U

#define my_kthread_run(threadfn, data, namefmt,cpu, ...)                          \
({                                                                         \
    struct task_struct *__k                                            \
         = kthread_create(threadfn, data, namefmt, ## __VA_ARGS__); \
    kthread_bind(__k, cpu);\
    if (!IS_ERR(__k))                                                  \
         wake_up_process(__k);                                      \
        __k;                                                               \
})

typedef struct biobox {
    struct list_head list_2_bioboxp;
    struct bio *usr_bio;
    int64_t tag_time;
} biobox_t;

typedef struct _usr_bio_req_queue {
    struct list_head free_biobox_pool;  //link list to link all biobox_t with usr_bio = NULL
    struct list_head used_biobox_pool;  //link list to link all biobox_t with usr_bio != NULL
    wait_queue_head_t biobox_pool_waitq;    //for io threads waiting
    spinlock_t biobox_pool_lock;    //for protect list_free_biobox and list_used_biobox
    atomic_t cnt_used_biobox;   //for calculate how many user bio waiting for process and
    //wake up io thread
    uint64_t cnt_totalwrite;    //How many data wrote from startup to now
    uint64_t cnt_total_partial_write;
    uint64_t cnt_totalread;     //How many data read from startup to now
    uint64_t cnt_total_partial_read;
} usrbio_queue_t;

typedef struct _usr_bio_req_handler {
    struct task_struct *pIOthread;
    int32_t handler_index;      //We will give each thread an index (count from 0), so that
    //some feature design (for example: calcuate how many request be handled
    //by handler) no need spinlock or atomic operations
    void *usr_private_data;
} usrbio_handler_t;

#define SZ_BIOBOX_HASH (LFSM_NUM_BIOBOX<<1)

#define LFSM_MAX_IOTHREAD 64
#define LFSM_MAX_IOQUEUE    LFSM_MAX_IOTHREAD
typedef struct bioboxpool {
    usrbio_handler_t io_worker[LFSM_MAX_IOTHREAD];
    usrbio_queue_t bioReq_queue[LFSM_MAX_IOQUEUE];

    int32_t *ioT_vcore_map;     //Reference to lfsmCfg, no need alloc or free
    int32_t cnt_io_workers;
    int32_t cnt_io_workqueues;

    atomic_t idx_sel_free;
    atomic_t idx_sel_used;

    atomic_t cn_total_copied;

#if LFSM_BIOBOX_STAT
    atomic_t cn_empty;
    atomic_t cn_runout;
    atomic64_t cn_totalcmd;
    atomic_t stat_queuetime;
    atomic_t stat_rounds;
#endif

    spinlock_t bio_hash_lock;
    struct bio *bio_hash[SZ_BIOBOX_HASH];
} bioboxpool_t;

extern int32_t bioboxpool_init(bioboxpool_t * pBBP, int32_t cnt_workers,
                               int32_t cnt_queues, int (*threadfunc)(void *arg),
                               void *arg, int32_t * vcore_map);
extern void bioboxpool_destroy(bioboxpool_t * pBBP);
extern int32_t bioboxpool_free2used(bioboxpool_t * pBBP, struct bio *pBio,
                                    int32_t id_sel);
extern struct bio *bioboxpool_used2free(bioboxpool_t * pBBP);
extern int32_t bioboxpool_dump(bioboxpool_t * pBBP, int8_t * buffer);
//bool bioboxpool_append_last(bioboxpool_t *pBBP, struct bio *pBio);
extern uint64_t get_total_write_io(bioboxpool_t * pBBP);
extern uint32_t get_total_waiting_io(bioboxpool_t * pBBP);
#endif // BIOBOXPOOL_INC
