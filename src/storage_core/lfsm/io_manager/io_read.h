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
#ifndef IOREAD_INC_1239712983712412
#define IOREAD_INC_1239712983712412

#define __bio_for_each_flash_page(bvl, bio, i, iter)           \
    for (bvl = &bio_iter_iovec((bio), (iter)), i = (iter).bi_idx;  \
         i < (bio)->bi_vcnt;                    \
         bvl+=MEM_PER_FLASH_PAGE, i+=MEM_PER_FLASH_PAGE)

#define bio_for_each_flash_page(bvl, bio, i)                \
    __bio_for_each_flash_page(bvl, bio, i, (bio)->bi_iter)

extern int32_t iread_bio(lfsm_dev_t * td, struct bio *bio, int32_t thdindex);
extern int32_t iread_bio_bh(lfsm_dev_t * td, struct bio *bio,
                            struct bio_container *bio_c, bool pipe_line,
                            int32_t iothd_index);
extern int32_t bioc_update_user_bio(struct bio_container *bio_c,
                                    struct bio *bio_user);
extern int32_t iread_mustread(lfsm_dev_t * td, sector64 pbno,
                              struct bio_container *bioc, int32_t idx_fpage,
                              bool isPipeline,
                              bio_end_io_t * ar_bi_end_io[MAX_ID_ENDBIO]);
extern int32_t iread_syncread(lfsm_dev_t * td, sector64 lpn,
                              struct bio_container *bioc, int32_t idx_fpage);

#endif // IOREAD_INC_1239712983712412
