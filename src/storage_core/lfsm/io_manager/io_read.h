/*
 * SPDX-License-Identifier: GPL-2.0-or-later OR Apache-2.0
 * Copyright (C) 2015-2020 Industrial Technology Research Institute
 *
 *
 *
 *  
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
