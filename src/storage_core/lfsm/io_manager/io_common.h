/*
 * SPDX-License-Identifier: GPL-2.0-or-later OR Apache-2.0
 * Copyright (C) 2015-2020 Industrial Technology Research Institute
 *
 *
 *
 *  
 */
#ifndef    __IOCOMMON_H
#define    __IOCOMMON_H

//#define CHK_BIO_BVPAG_KMAP

struct flash_page {
    struct page *page_array[MEM_PER_FLASH_PAGE];
};

extern struct bio_container **gar_pbioc;
extern int32_t gidx_arbioc;

#define ID_ENDBIO_NORMAL 0
#define ID_ENDBIO_BRM 1
#define MAX_ID_ENDBIO 2

#define CN_EXTRA_BIOC 10240

#define EPPLRB_CONFLICT  0      // represent the page has been conflicted
#define EPPLRB_READY2GO 1       // represent the data of the required page is ready on memory
#define EPPLRB_READ_DISK  2     // represent you must read the page from disk no matter the page is conflicted

/*Common functions*/
extern int32_t bioc_alloc_add2list(lfsm_dev_t * td, int32_t idx);
extern int32_t bioc_extra_free(lfsm_dev_t * td);

extern sector64 bioc_get_psn(struct bio_container *pbioc, int32_t idx_fpage);
extern sector64 bioc_get_sourcepsn(struct bio_container *pbioc,
                                   int32_t idx_fpage);

int32_t bioc_cn_fpage(struct bio_container *bioc);
extern void bioc_resp_user_bio(struct bio *bio, int32_t err);
struct page *bio_get_mpage(struct bio *pbio, int32_t off_mpage);

extern void end_bio_page_read(struct bio *bio);
extern int32_t post_rw_check_waitlist(lfsm_dev_t * td,
                                      struct bio_container *bio_c, bool failed);
extern struct bio_container *get_bio_container(lfsm_dev_t * td,
                                               struct bio *org_bio,
                                               sector64 org_bi_sector,
                                               sector64 lpn_start,
                                               sector64 lpn_end,
                                               int32_t write_type, int32_t rw);
extern void put_bio_container(lfsm_dev_t * td, struct bio_container *bio_c);
extern void put_bio_container_nolock(lfsm_dev_t * td,
                                     struct bio_container *bio_c,
                                     sector64 prelocked_lbno);

extern void init_bio_container(struct bio_container *bio_c, struct bio *bio,
                               struct block_device *bdev,
                               bio_end_io_t * bi_end_io, int32_t rw,
                               sector64 bi_sector, int32_t vcnt);
extern void bio_lfsm_init(struct bio *bio, struct bio_container *bio_c,
                          struct block_device *bdev, bio_end_io_t * bi_end_io,
                          int32_t rw, sector64 bi_sector, int32_t vcnt);

extern int32_t ibvpage_RW(uint64_t ulldsk_boffset, void *in_buf,
                          uint32_t uiblen, uint32_t ori_vcnt,
                          struct bio_container *bio_c, int32_t RW);
extern uint64_t get_elapsed_time2(struct timeval *, struct timeval *);
extern int32_t request_enqueue_A(struct request_queue *q, struct bio *bio);
extern blk_qc_t request_enqueue(struct request_queue *q, struct bio *bio);
extern int32_t request_dequeue(void *arg);
extern void free_composed_bio(struct bio *, int32_t);
extern void free_bio_container(struct bio_container *, int32_t);
//int32_t move_from_active_to_free(lfsm_dev_t *);
extern int32_t compose_bio(struct bio **, struct block_device *, bio_end_io_t *,
                           void *, int32_t, int32_t);
extern void end_bio_page_read_pipeline(struct bio *bio);

extern int32_t compose_bio_no_page(struct bio **biop, struct block_device *bdev,
                                   bio_end_io_t * bi_end_io, void *bi_private,
                                   int32_t bi_size, int32_t bi_vec_size);

extern struct bio_container *bioc_alloc(lfsm_dev_t * td, int32_t page_num,
                                        int32_t alloc_pages);
int32_t dump_bio_active_list(lfsm_dev_t * td, int8_t * buffer);

extern int32_t copy_pages(lfsm_dev_t * td, sector64 source, sector64 dest,
                          sector64 lpn, int32_t run_length, bool isPipeline);

extern int32_t free_all_per_page_list_items(lfsm_dev_t * td,
                                            struct bio_container *bio_c);

extern int32_t read_flash_page_vec(lfsm_dev_t * td, sector64 pos,
                                   struct bio_vec *page_vec,
                                   int32_t idx_mpage_in_bio,
                                   struct bio_container *entry,
                                   bool is_pipe_line,
                                   bio_end_io_t * ar_bi_end_io[MAX_ID_ENDBIO]);
extern int32_t read_flash_page(lfsm_dev_t * td, sector64 pos,
                               struct flash_page *flashpage,
                               int32_t idx_mpage_in_bio,
                               struct bio_container *entry, bool is_pipe_line,
                               bio_end_io_t * bi_end_io[MAX_ID_ENDBIO]);
extern bool set_flash_page_vec_zero(struct bio_vec *page_vec);
extern void bioc_get_init(struct bio_container *pbioc, sector64 lpn_start,
                          int32_t cn_fpage, int32_t write_type);

extern struct bio_container *bioc_get(lfsm_dev_t * td, struct bio *org_bio,
                                      sector64 org_bi_sector,
                                      sector64 lpn_start, sector64 lpn_end,
                                      int32_t write_type, int32_t rw,
                                      int32_t tp_get);
extern int32_t bioc_insert_activelist(struct bio_container *bioc,
                                      lfsm_dev_t * td, bool isConflictCheck,
                                      int32_t write_type, int32_t rw);
extern struct bio_container *bioc_lock_lpns_wait(lfsm_dev_t * td, sector64 lpn,
                                                 int32_t cn_fpages);
extern void bioc_unlock_lpns(lfsm_dev_t * td, struct bio_container *pbioc);
extern struct bio_container *bioc_get_and_wait(lfsm_dev_t * td,
                                               struct bio *bio_user,
                                               int32_t rw);
extern struct bio_container *bioc_gc_get_and_wait(lfsm_dev_t * td,
                                                  struct bio *bio_user,
                                                  sector64 lpn);

extern void bioc_wait_conflict(struct bio_container *bio_c, lfsm_dev_t * td,
                               int32_t HasConflict);
extern bool bioc_bio_free(lfsm_dev_t * td, struct bio_container *pBioc);
extern bool batch_read_flash_page(struct diskgrp *pgrp, sector64 pos,
                                  int32_t idx_page_in_bio, struct bio *pbio);

extern bool compare_whole_eu(lfsm_dev_t * td, struct EUProperty *eu_source,
                             struct EUProperty *eu_dest,
                             int32_t cn_hpage_perlog, bool compare_spare,
                             bool comp_whole_eu, sftl_err_msg_t * perr_msg,
                             bool (*parse_compare_fn)(lfsm_dev_t * td,
                                                      int8_t * buf_main,
                                                      int8_t * buf_backup));

extern bool copy_whole_eu_inter(lfsm_dev_t * td, sector64 source, sector64 dest,
                                int32_t cn_page);
extern int32_t bioc_dump_missing(lfsm_dev_t * td);
extern atomic_t gcn_gc_conflict_giveup;
extern bool bioc_list_search(lfsm_dev_t * td, struct list_head *plist,
                             struct bio_container *pbioc_target);
extern bool copy_whole_eu_cross(lfsm_dev_t * td, sector64 source, sector64 dest,
                                sector64 * ar_lpn, int32_t cn_page,
                                char *ar_valid_byte);

#endif
