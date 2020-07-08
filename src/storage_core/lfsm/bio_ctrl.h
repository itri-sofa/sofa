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
#ifndef BIO_CTRL987565788787
#define BIO_CTRL_987565788787

typedef struct cb_data {
    atomic_t cn_send;
    wait_queue_head_t wq;       // to block user flush command
} cb_data_t;

typedef struct sflushbio {
    void *user_bi_private;
    bio_end_io_t *user_bi_end_io;
    struct bio *user_bio;
    atomic_t cn_pending_tag;    // the number of "mark" bio in queue that didn't encounter by the iod00x
    atomic_t cn_pending_flush;  // the number of flush bio to every device that didn't finished
    wait_queue_head_t wq;       // to block user flush command
    blk_status_t result_flushbio;
} sflushbio_t;

#define FLUSH_BIO_BIRW_FLAG  (REQ_PREFLUSH)
#define DISCARD_BIO_BIRW_FLAG  (REQ_OP_DISCARD)

#define IS_FLUSH_BIO(bio)  (bio->bi_opf & FLUSH_BIO_BIRW_FLAG)
#define IS_TRIM_BIO(bio)   (bio_op(bio) == DISCARD_BIO_BIRW_FLAG)

#if LFSM_FLUSH_SUPPORT == 1
extern sflushbio_t *flushbio_alloc(struct bio *pBio);
extern bool flushbio_handler(struct sflushbio *pflushbio, struct bio *pBio,
                             bioboxpool_t * pBBP);
#endif

extern int32_t ctrl_bio_filter(lfsm_dev_t * td, struct bio *bio);
extern int32_t trim_bmt_table(lfsm_dev_t * td, sector64 lpn, sector64 cn_pages);    // in flash page
extern bool trim_bio_shall_reset_UL(SUpdateLog_t * pUL);
#endif
