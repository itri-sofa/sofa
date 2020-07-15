/*
 * SPDX-License-Identifier: GPL-2.0-or-later OR Apache-2.0
 * Copyright (C) 2015-2020 Industrial Technology Research Institute
 *
 *
 *
 *  
 */

#include <linux/types.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/bio.h>
#include "../config/lfsm_setting.h"
#include "../config/lfsm_feature_setting.h"
#include "../config/lfsm_cfg.h"
#include "../common/common.h"
#include "../common/mem_manager.h"
#include "../common/rmutex.h"
#include "mqbiobox.h"
#include "biocbh.h"
#include "../nextprepare.h"
#include "../diskgrp.h"
#include "../system_metadata/Dedicated_map.h"
#include "../lfsm_core.h"
#include "../bmt.h"
#include "../GC.h"
#include "../bmt_ppq.h"
#include "../bmt_ppdm.h"
#include "../err_manager.h"
#include "../special_cmd.h"
#include "io_common.h"
#include "io_read.h"
#include "io_write.h"
#include "../perf_monitor/ioreq_pendstage_mon.h"

/* iread_bio read handler
 * @[in]td: info 
 * @[in]bio: original bio request
 * @[return] 0:ok -1:error
 */
int32_t iread_bio(lfsm_dev_t * td, struct bio *bio, int32_t thdindex)
{
    struct bio_container *bio_c;
#if LFSM_PAGE_SWAP == 1
    int32_t i;
#endif

    if (bio->bi_vcnt > MAX_MEM_PAGE_NUM) {
        LOGERR("bio_c->bio->bi_vcnt(%d) > MAX_MEM_PAGE_NUM\n", bio->bi_vcnt);
        return -EIO;
    }

    if (NULL == (bio_c = bioc_get_and_wait(td, bio, REQ_OP_READ))) {
        LOGERR("fail to get free bioc when read\n");
        return -ENOMEM;
    }
#if LFSM_PAGE_SWAP == 1
    if ((bio->bi_vcnt == MEM_PER_FLASH_PAGE)
        && ((bio->bi_iter.bi_sector % SECTORS_PER_FLASH_PAGE) == 0)
        && (bio->bi_iter.bi_size % PAGE_SIZE == 0)
        && (bio->bi_io_vec[0].bv_offset == 0)
        && (bio->bi_io_vec[0].bv_len == PAGE_SIZE)) {
        for (i = 0; i < MEM_PER_FLASH_PAGE; i++) {
            bio_c->ar_ppage_swap[i] = bio_c->bio->bi_io_vec[i].bv_page;
            bio_c->bio->bi_io_vec[i].bv_page = bio->bi_io_vec[i].bv_page;
        }
    }
#endif

    iread_bio_bh(td, bio, bio_c, true, thdindex);
    return 0;
}

int32_t iread_mustread(lfsm_dev_t * td, sector64 pbno,
                       struct bio_container *bioc, int32_t idx_fpage,
                       bool isPipeline,
                       bio_end_io_t * ar_bi_end_io[MAX_ID_ENDBIO])
{
    struct bad_block_info bad_blk_info;
    int32_t ret, retry, idx_mpage;

    idx_mpage = idx_fpage * MEM_PER_FLASH_PAGE;
    ret = 0;
    retry = 0;
    while (true) {
        ret = read_flash_page_vec(td, pbno, &(bioc->bio->bi_io_vec[idx_mpage]),
                                  idx_mpage, bioc, isPipeline, ar_bi_end_io);
        if ((ret == 0) || (ret == -ENODEV)) {
            return ret;
        }

        if (special_read_enquiry(pbno, 1, 0, 0, &bad_blk_info) != 0) {
            goto l_fail;
        }

        LOGWARN(" read fail retry=%d\n", retry);
        if (++retry > LFSM_MIN_RETRY) {
            goto l_fail;
        }
    }
  l_fail:
    errmgr_bad_process(pbno, bioc->start_lbno, bioc->start_lbno, td, false,
                       false);
    return -EIO;

}

static int32_t iread_read_ifneed(lfsm_dev_t * td, sector64 pbno,
                                 struct bio_container *bioc, int32_t idx_fpage,
                                 bool isPipeline,
                                 bio_end_io_t * ar_bi_end_io[MAX_ID_ENDBIO])
{
    int32_t ret;

    if (NULL == bioc->per_page_list[idx_fpage]) {
        LOGINERR("fatal error bioc->per_page_list[%d] is null\n", idx_fpage);
        dump_stack();
        return -EPERM;
    }

    switch (bioc->per_page_list[idx_fpage]->ready_bit) {
    case EPPLRB_READY2GO:
        return 0;
    case EPPLRB_CONFLICT:
        return -EPERM;
    case EPPLRB_READ_DISK:
        if ((ret =
             iread_mustread(td, pbno, bioc, idx_fpage, isPipeline,
                            ar_bi_end_io)) < 0) {
            if ((ret = rddcy_fpage_read(td, pbno, bioc, idx_fpage)) < 0) {
                return ret;
            }
        }

        if (!isPipeline) {
            //TODO: no need lock here since no lock for this variable at other place
            spin_lock(&td->datalog_list_spinlock[bioc->idx_group]);
            bioc->per_page_list[idx_fpage]->ready_bit = EPPLRB_READY2GO;
            spin_unlock(&td->datalog_list_spinlock[bioc->idx_group]);
        }
        return 0;
    default:
        return -EPERM;
    }
}

// lpn in fpage
int32_t iread_syncread(lfsm_dev_t * td, sector64 lpn,
                       struct bio_container *bioc, int32_t idx_fpage)
{
    bio_end_io_t *ar_bi_end_io[MAX_ID_ENDBIO];
    struct D_BMT_E dbmta;
    int32_t ret;

    ar_bi_end_io[ID_ENDBIO_BRM] = end_bio_partial_write_read;
    ar_bi_end_io[ID_ENDBIO_NORMAL] = end_bio_page_read;

    if ((ret = bmt_lookup(td, td->ltop_bmt, lpn, false, 0, &dbmta)) < 0) {
        if (ret != -EACCES) {
            return false;
        }
    }

    if (dbmta.pbno == PBNO_SPECIAL_NEVERWRITE) {
        set_flash_page_vec_zero(&
                                (bioc->bio->
                                 bi_io_vec[idx_fpage * MEM_PER_FLASH_PAGE]));
        return true;
    }

    if (iread_read_ifneed(td, dbmta.pbno, bioc, idx_fpage, false, ar_bi_end_io)
        < 0) {
        return false;
    }
    return true;
}

//TODO: we can use a global function array to save the assignment of ar_bi_end_io
/*
 * Description: iothd_index used for io request counting.
 *              if caller is io thread, set corresponding iothd_index
 *              if caller is other thread (for example: fail bio handler), please assign -1
 */
int32_t iread_bio_bh(lfsm_dev_t * td, struct bio *bio,
                     struct bio_container *bio_c, bool is_pipe_line,
                     int32_t iothd_index)
{
    bio_end_io_t *ar_bi_end_io[MAX_ID_ENDBIO];
    struct bad_block_info bad_blk_info;
    struct bio_vec *bvec;
    struct D_BMT_E dbmta;
    sector64 lpn;
    int32_t i, j, idx_fpage, ret;
    bool bh_handled, data_readed;

    ret = 0;
    bh_handled = false;
    data_readed = false;

    ar_bi_end_io[ID_ENDBIO_BRM] = sbrm_end_bio_generic_read;
    if (is_pipe_line) {
        ar_bi_end_io[ID_ENDBIO_NORMAL] = end_bio_page_read_pipeline;
    } else {
        ar_bi_end_io[ID_ENDBIO_NORMAL] = end_bio_page_read;
    }

    if (bio_c->bio->bi_iter.bi_size > PAGE_SIZE * MAX_MEM_PAGE_NUM) {
        LOGERR("bi_size(%u) > PAGE_SIZE*MAX_MEM_PAGE_NUM\n",
               bio_c->bio->bi_iter.bi_size);
        ret = -EIO;
        goto END_ERR;
    }
    lpn = bio_c->bio->bi_iter.bi_sector >> SECTORS_PER_FLASH_PAGE_ORDER;

    // weafon: calling iread_bio_bh once may trigger end_bio_page_read(pipeline) multiple times.
    // Thus, set io_queue_fin to 0 "once only" per iread_bio_bh called
    // then end_bio_page_read(wi/wo pipeline) will set it to err once a err of page is detected.
    // Or, set it to 1 when the last time of end_bio_page_read(pipeline) is called.
    bio_c->io_queue_fin = 0;

    bio_for_each_flash_page(bvec, bio_c->bio, i) {
        idx_fpage = i / MEM_PER_FLASH_PAGE;

        if ((ret = bmt_lookup(td, td->ltop_bmt, lpn, true, 0, &dbmta)) < 0) {
            if (ret == -EACCES) {
                if ((ret =
                     rddcy_fpage_read(td, dbmta.pbno, bio_c, idx_fpage)) < 0) {
                    goto END_ERR;
                }
                bio_c->per_page_list[idx_fpage]->ready_bit = EPPLRB_READY2GO;
            } else {
                goto END_ERR;
            }
        }
        //TODO: if we can know a pbno never be wrote, we can return 0
        if (dbmta.pbno == PBNO_SPECIAL_NEVERWRITE) {
            //This lbno is not mapped, no data -- trying to read raw content.
            atomic_inc(&lfsmdev.biocbh.cnt_readNOPPNIO_done);   //iostat
            for (j = 0; j < MEM_PER_FLASH_PAGE; j++) {
                bio_c->bio->bi_io_vec[i + j].bv_len = 0;
                bio_c->bio->bi_io_vec[i + j].bv_offset = 0;
            }

            if (atomic_dec_and_test(&bio_c->cn_wait)) {
                bh_handled = false;
            }
        } else {
            if (NULL == bio_c->per_page_list[idx_fpage]) {
                LOGERR("Error1: bio_c->per_page_list[%d]=NULL\n", idx_fpage);
                ret = -EIO;
                goto END_ERR;
            }

            if (bio_c->per_page_list[idx_fpage]->ready_bit == EPPLRB_READ_DISK) {
                if (is_pipe_line) {
                    if ((ret =
                         iread_mustread(td, dbmta.pbno, bio_c, idx_fpage,
                                        true, ar_bi_end_io)) < 0) {
                        if ((ret =
                             rddcy_fpage_read(td, dbmta.pbno, bio_c,
                                              idx_fpage)) < 0) {
                            goto END_ERR;
                        }
                        bio_c->per_page_list[idx_fpage]->ready_bit =
                            EPPLRB_READY2GO;
                    } else {
                        bh_handled = true;
                    }
                } else {
                    data_readed = false;
                    if (special_read_enquiry(dbmta.pbno, 1, 0, 0, &bad_blk_info)
                        == 0) {
                        // special query no fail, continue non-pipeling read from disk

                        if ((ret = iread_mustread(td, dbmta.pbno, bio_c,
                                                  idx_fpage, false,
                                                  ar_bi_end_io)) == 0) {
                            data_readed = true;
                        }
                    }

                    if (!data_readed) {
                        errmgr_bad_process(dbmta.pbno, bio_c->start_lbno, bio_c->start_lbno, td, false, false); // log one error

                        if ((ret =
                             rddcy_fpage_read(td, dbmta.pbno, bio_c,
                                              idx_fpage)) < 0) {
                            LOGERR("rddcy_fpage_read fail %d\n", ret);
                            goto END_ERR;
                        }
                    }
                    bio_c->per_page_list[idx_fpage]->ready_bit =
                        EPPLRB_READY2GO;
                }
            } else {
                if (atomic_dec_and_test(&bio_c->cn_wait)) {
                    bh_handled = false;
                }
                //printk("readybit=1 bio_c%p deced cn_wait %d\n",bio_c,atomic_read(&bio_c->read4k_bio_unfinish_count));
            }

            for (j = 0; j < MEM_PER_FLASH_PAGE; j++) {
                bio_c->bio->bi_io_vec[i + j].bv_len = PAGE_SIZE;
                bio_c->bio->bi_io_vec[i + j].bv_offset = 0;
            }
        }

        lpn++;
    }

    if (bh_handled) {           //  org_bio is handled by bh, so we only return
        return 0;
    }
//    if(is_pipe_line)// if is_pipeline and run to this line, we must remove the hanged submit list
//    {
//        spin_lock_irqsave(&td->biocbh.lock_biocbh,flag);
//        list_del(&bio_c->submitted_list);
//        spin_unlock_irqrestore(&td->biocbh.lock_biocbh,flag);
//    }
    atomic_set(&bio_c->active_flag, ACF_PAGE_READ_DONE);

    if (bio_c->is_user_bioc) {
        if (iothd_index >= 0) {
            INC_IOR_STAGE_CNT(IOR_STAGE_NO_METADATA_RESP, iothd_index);
            INC_IOR_STAGE_CNT(IOR_STAGE_LFSMIOT_PROCESS_DONE, iothd_index);
        }
    }

    if (bioc_update_user_bio(bio_c, bio)) {
        ret = -EIO;
    }

  END_ERR:
    /*tell post_rw that this bio_c has failed, so do not satisfy any dependency from memory */
    lfsm_resp_user_bio_free_bioc(td, bio_c, ret, -1);
    return 0;
}

int32_t bioc_update_user_bio(struct bio_container *bio_c, struct bio *bio_user)
{
    uint64_t ulldsk_boffset;
    sector64 ulremaindatalen;
    void *iovec_mem;
    struct bio_vec bvec;
    struct bvec_iter iter;
    int32_t i;

#if LFSM_PAGE_SWAP == 1
    if (NULL != bio_c->ar_ppage_swap[0]) {
        if (bio_c->bio->bi_io_vec[0].bv_len != 0) {
            return 0;
        }

        for (i = 0; i < MEM_PER_FLASH_PAGE; i++) {
            iovec_mem = kmap(bio_c->bio->bi_io_vec[i].bv_page);
            memset(iovec_mem, 0, PAGE_SIZE);
            kunmap(bio_c->bio->bi_io_vec[i].bv_page);
        }

        return 0;
    }
#endif

    ulldsk_boffset = bio_user->bi_iter.bi_sector << SECTOR_ORDER;   //in bytes
    ulremaindatalen = bio_sectors(bio_user) << SECTOR_ORDER;    //in bytes
    if (ulremaindatalen > PAGE_SIZE * MAX_MEM_PAGE_NUM) {
        LOGERR("err ulremaind\n");
        return -EIO;
    }

    bio_for_each_segment(bvec, bio_user, iter) {
        if (NULL == (iovec_mem = kmap(bvec.bv_page))) {
            LOGERR("kmap iovec_mem fail\n");
            return -ENOMEM;
        }

        iovec_mem += bvec.bv_offset;
        if (ibvpage_RW
            (ulldsk_boffset, iovec_mem, bvec.bv_len, iter.bi_idx, bio_c,
             READ) != 0) {
            LOGERR(" IS_ERR_VALUE(ibvpage_RW()) for READ\n");
            kunmap(bvec.bv_page);
            return -EIO;
        }

        kunmap(bvec.bv_page);
        ulremaindatalen -= bvec.bv_len;
        ulldsk_boffset += bvec.bv_len;
    }

    if (ulremaindatalen != 0) {
        LOGERR("Error2: ulremaindatalen(%llu)!=0\n", ulremaindatalen);
        return -EIO;
    }

    return 0;
}
