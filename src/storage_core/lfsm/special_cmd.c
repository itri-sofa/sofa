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
#include <linux/types.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/wait.h>
#include "config/lfsm_setting.h"
#include "config/lfsm_feature_setting.h"
#include "config/lfsm_cfg.h"
#include "common/common.h"
#include "common/mem_manager.h"
#include "common/rmutex.h"
#include "io_manager/mqbiobox.h"
#include "io_manager/biocbh.h"
#include "nextprepare.h"
#include "diskgrp.h"
#include "system_metadata/Dedicated_map.h"
#include "lfsm_core.h"
#include "special_cmd.h"
#include "io_manager/io_common.h"
#include "EU_access.h"
#include "GC.h"
#include "batchread.h"

#define END_BBM 1
#define CONT_BBM 0

int32_t mark_as_bad(sector64 eu_position, int32_t run_length_in_eus)
{
    lfsm_dev_t *td = &lfsmdev;
    return build_mark_bad_command(td, eu_position, run_length_in_eus);
}

/* end_bio_erase_io HWINT for special write
 * @[in]bio: bio 
 * @[in]err: 
 * @[return] void
 */
static void end_bio_special_wr_io_pipeline(struct bio *bio)
{
    struct bio_container *entry = bio->bi_private;
    int32_t err = blk_status_to_errno(bio->bi_status);

    if (err) {
        LOGERR("ret %d \n", err);
        atomic_set(&entry->active_flag, err);
    } else {
        atomic_set(&entry->active_flag, ACF_PAGE_READ_DONE);
    }

    if (err) {
        entry->io_queue_fin = err;
    } else {
        entry->io_queue_fin = 1;
    }

    biocbh_io_done(&lfsmdev.biocbh, entry, true);
    return;
}

static void end_bio_special_wr_io(struct bio *bio)
{
    struct bio_container *entry = bio->bi_private;
    int32_t err;
//     lfsm_dev_t *td = entry->lfsm_dev;

//    printk("returned from special write err=%d\n",err);
    //entry->status= err;
    //atomic_set(&entry->active_flag, ACF_PAGE_READ_DONE);
    err = blk_status_to_errno(bio->bi_status);
    if (err) {
        LOGERR("ret %d\n", err);
        atomic_set(&entry->active_flag, err);
    } else {
        atomic_set(&entry->active_flag, ACF_PAGE_READ_DONE);
    }

    wake_up_interruptible(&entry->io_queue);
    return;

}

static void end_bio_special_rd_io(struct bio *bio)
{
    struct bio_container *entry;
    int32_t err;

    entry = bio->bi_private;
    err = blk_status_to_errno(bio->bi_status);
    dprintk("Inside %s at %d\n", __FUNCTION__, __LINE__);
    if (err) {
        LOGERR("ret %d \n", err);
        atomic_set(&entry->active_flag, err);
    } else {
        atomic_set(&entry->active_flag, ACF_PAGE_READ_DONE);
    }

    wake_up_interruptible(&entry->io_queue);
    return;
}

int32_t compose_special_command_bio(struct diskgrp *pgrp, int32_t cmd_type,
                                    void *cmd, struct bio *page_bio)
{
    struct special_cmd spl;
    struct page *page_buf;
    struct bio_vec *bv;
    sector64 tmp;
    uint8_t *source, *dest;

    while (NULL == (page_buf = track_alloc_page(GFP_ATOMIC | __GFP_ZERO))) {
        LOGERR("Failure allocating page\n");
        schedule();
    }

    dprintk("Inside %s at %d: cmd_type=%d bi_sector=%lld\n",
            __FUNCTION__, __LINE__, cmd_type, page_bio->bi_iter.bi_sector);
    spl.signature[0] = 'L';
    spl.signature[1] = 'F';
    spl.signature[2] = 'S';
    spl.signature[3] = 'M';

    spl.cmd_type = cmd_type;

    switch (cmd_type) {
    case LFSM_ERASE:
        memcpy(&spl.erase_cmd, (struct erase *)cmd, sizeof(struct erase));
        page_bio->bi_iter.bi_sector = spl.erase_cmd.start_position;
        tmp = spl.erase_cmd.start_position;
        spl.erase_cmd.start_position = do_div(tmp, pgrp->cn_sectors_per_dev);
        break;

    case LFSM_COPY:
        memcpy(&spl.copy_cmd, (struct copy *)cmd, sizeof(struct copy));
        page_bio->bi_iter.bi_sector = spl.copy_cmd.source;
        tmp = spl.copy_cmd.source;
        spl.copy_cmd.source = do_div(tmp, pgrp->cn_sectors_per_dev);
        tmp = spl.copy_cmd.dest;
        spl.copy_cmd.dest = do_div(tmp, pgrp->cn_sectors_per_dev);
        break;

    case LFSM_MARK_BAD:
        memcpy(&spl.mark_bad_cmd, (struct mark_as_bad *)cmd,
               sizeof(struct mark_as_bad));
        page_bio->bi_iter.bi_sector = spl.mark_bad_cmd.eu_position;
        tmp = spl.mark_bad_cmd.eu_position;
        spl.mark_bad_cmd.eu_position = do_div(tmp, pgrp->cn_sectors_per_dev);
        break;
    case LFSM_READ_BATCH_PRELOAD:
        memcpy(&spl.brm_cmd, (struct sbrm_cmd *)cmd, sizeof(struct sbrm_cmd));
        page_bio->bi_iter.bi_sector = (sector64) spl.brm_cmd.padding;
        page_bio->bi_iter.bi_sector |= LFSM_BREAD_BIT;
        break;

    default:
        LFSM_ASSERT2(0, "InternalERROR@%s cmd_type= %d\n", __FUNCTION__,
                     cmd_type);
        return -1;
        break;
    }

    dest = (uint8_t *) page_address(page_buf);
    source = (uint8_t *) & spl;

    memset(dest, 0, PAGE_SIZE);

    memcpy(dest, source, sizeof(spl));

    page_bio->bi_iter.bi_idx = 0;

    page_bio->bi_vcnt = 1;      // 1 page
    page_bio->bi_iter.bi_size = SECTOR_SIZE;    // PAGE_SIZE; // 1 pages // weafon
    page_bio->bi_opf = REQ_OP_WRITE;
    page_bio->bi_status = BLK_STS_OK;
    page_bio->bi_flags &= ~(1 << BIO_SEG_VALID);
    page_bio->bi_phys_segments = 0;

    page_bio->bi_seg_front_size = 0;
    page_bio->bi_seg_back_size = 0;
    page_bio->bi_next = NULL;

    // New changes
    bv = &(page_bio->bi_io_vec[0]);
    bv->bv_page = page_buf;
    bv->bv_offset = 0;
    bv->bv_len = SECTOR_SIZE;
    // End of new changes

    page_bio->bi_iter.bi_sector |= LFSM_SPECIAL_CMD;

    return 0;

}

int32_t build_erase_command(sector64 lsn, int32_t size_in_eus)
{
    return devio_erase(lsn, size_in_eus, NULL);
}

static void _cmd_io_wait_timeout(struct bio_container *bio_c, int32_t cmd_type)
{
    int32_t cnt, wait_ret;

    cnt = 0;
    while (true) {
        cnt++;
        wait_ret = wait_event_interruptible_timeout(bio_c->io_queue,
                                                    atomic_read(&bio_c->
                                                                active_flag) !=
                                                    ACF_ONGOING,
                                                    LFSM_WAIT_EVEN_TIMEOUT);
        if (wait_ret <= 0) {
            LOGWARN("command %d IO no response after seconds %d\n",
                    cmd_type, LFSM_WAIT_EVEN_TIMEOUT_SEC * cnt);
        } else {
            break;
        }
    }
}

int32_t glfsm_state_copy = 0;
int32_t build_copy_command(lfsm_dev_t * td, sector64 source, sector64 dest,
                           int32_t run_length_in_pages, bool isPipeline)
{
    struct copy cmd;
    struct bio_container *bio_c;
    struct bio *page_bio, *org_bioct_bio;
    int32_t ret;

    ret = 0;
    dprintk("Inside %s at %d\n", __FUNCTION__, __LINE__);
    while (NULL == (page_bio = track_bio_alloc(GFP_ATOMIC | __GFP_ZERO, 1))) {
        LOGERR("allocate header_bio fails \n");
        schedule();
    };

    bio_c = get_bio_container(td, NULL, 0, 0, 0, DONTCARE_TYPE, WRITE);
    LFSM_ASSERT(bio_c);
    /*remember prev bio. */
    org_bioct_bio = bio_c->bio;

    bio_c->org_bio = bio_c->bio;
    bio_c->bio = page_bio;

    cmd.source = source;
    cmd.dest = dest;
    cmd.run_length_in_pages = run_length_in_pages;

    ret = compose_special_command_bio(&grp_ssds, LFSM_COPY, &cmd, page_bio);

    if (ret < 0) {
        LFSM_ASSERT(0);
        return ret;
    }
    bio_c->io_queue_fin = 0;
    page_bio->bi_end_io = end_bio_special_wr_io;
    page_bio->bi_private = bio_c;

    atomic_set(&bio_c->active_flag, ACF_ONGOING);

    if (isPipeline) {
        bio_c->bio->bi_end_io = end_bio_special_wr_io_pipeline;
        ret = 0;
    } else {
        glfsm_state_copy = 1;
    }

    ret = my_make_request(page_bio);

    if (!isPipeline) {
        glfsm_state_copy = 2;
        if (ret == 0) {
            //lfsm_wait_event_interruptible(bio_c->io_queue,
            //        atomic_read(&bio_c->active_flag) != ACF_ONGOING);
            _cmd_io_wait_timeout(bio_c, LFSM_COPY);
        }
    /**Error handling for copy*/
        glfsm_state_copy = 3;
        if (atomic_read(&bio_c->active_flag) == ACF_PAGE_READ_DONE) {
            ret = 0;
        } else {
            ret = -1;
        }
        dprintk("Copy command returned %d source %llu dest %llu\n", ret, source,
                dest);

        atomic_set(&bio_c->active_flag, ACF_ONGOING);

        track_free_page(page_bio->bi_io_vec[0].bv_page);
        page_bio->bi_io_vec[0].bv_page = NULL;

        track_bio_put(page_bio);
        /* restore prev bio */
        bio_c->bio = org_bioct_bio;
        put_bio_container(td, bio_c);
    }

    return ret;
}

int32_t build_mark_bad_command(lfsm_dev_t * td, sector64 eu_position,
                               int32_t run_length_in_eus)
{
    struct mark_as_bad cmd;
    struct bio_container *bio_c;
    struct bio *page_bio, *org_bioct_bio;
    int32_t ret;

    ret = 0;
    while (NULL == (page_bio = track_bio_alloc(GFP_ATOMIC | __GFP_ZERO, 1))) {
        LOGERR("allocate header_bio fails in %s\n", __FUNCTION__);
        schedule();
    };

    bio_c = get_bio_container(td, NULL, 0, 0, 0, DONTCARE_TYPE, WRITE);
    LFSM_ASSERT(bio_c);
    /*remember prev bio. */
    org_bioct_bio = bio_c->bio;

    cmd.eu_position = (eu_position >> SECTORS_PER_SEU_ORDER) << SUPER_EU_ORDER;
    cmd.runl_length_in_eus = run_length_in_eus;
    LOGINFO("mark eu_position %llu physicl_block_id %lld len_block %d",
            eu_position, cmd.eu_position, cmd.runl_length_in_eus);

    ret = compose_special_command_bio(&grp_ssds, LFSM_MARK_BAD, &cmd, page_bio);

    if (ret < 0) {
        LFSM_ASSERT(0);
        return ret;
    }

    bio_c->io_queue_fin = 0;
    page_bio->bi_end_io = end_bio_special_wr_io;
    page_bio->bi_private = bio_c;

    atomic_set(&bio_c->active_flag, ACF_ONGOING);

    if (my_make_request(page_bio) == 0) {
        //lfsm_wait_event_interruptible(bio_c->io_queue,
        //        atomic_read(&bio_c->active_flag) != ACF_ONGOING);
        _cmd_io_wait_timeout(bio_c, LFSM_MARK_BAD);
    }
/**Error handling for erase*/

    if (atomic_read(&bio_c->active_flag) == ACF_PAGE_READ_DONE) {
        ret = 0;
    } else {
        ret = -1;
    }
    LOGINFO("Mark as bad returned %d\n", ret);

    atomic_set(&bio_c->active_flag, ACF_ONGOING);

    track_free_page(page_bio->bi_io_vec[0].bv_page);
    page_bio->bi_io_vec[0].bv_page = NULL;
    track_bio_put(page_bio);

    /* restore prev bio */
    bio_c->bio = org_bioct_bio;

    put_bio_container(td, bio_c);

    return ret;
}

static int32_t BBM_parse(int8_t * pBuf, int32_t id_disk,
                         int32_t * pRetBadBlockNum)
{
    uint32_t id_eu, offseteu, id_eu_eu_map, i, off;

    id_eu = 0;
    offseteu = 0;
    id_eu_eu_map = 0;
    *pRetBadBlockNum = 0;

    // Based on the disk number, calculate the global disk number
    for (i = 0; i < id_disk; i++) {
        offseteu += (gc[i]->total_eus);
    }

    for (off = 0; off < FLASH_PAGE_SIZE; off += sizeof(id_eu)) {
        id_eu = *(uint32_t *) (pBuf + off);
        if (id_eu == 0xFFFFFFFF) {
            return END_BBM;
        }

        if (id_eu == 0) {
            continue;
        }

        if ((id_eu >> SUPER_EU_ORDER) > gc[id_disk]->total_eus) {
            continue;
        }
//        printk("BootBBM: phyblk=%u EU %u + %u\n", id_eu,id_eu>>SUPER_EU_ORDER, offseteu);
        id_eu_eu_map = (id_eu >> SUPER_EU_ORDER) + offseteu;

        if (drop_EU(id_eu_eu_map)) {
            if (atomic_read(&pbno_eu_map[id_eu_eu_map]->state_list) != -1) {    // AN: 2012/10/4 not in freelist
                continue;
            }

            if (gc[id_disk]->free_list_number <= 0) {
                return -EMFILE;
            }

            LFSM_RMUTEX_LOCK(&gc[id_disk]->whole_lock);
            list_del_init(&pbno_eu_map[id_eu_eu_map]->list);
            gc[id_disk]->free_list_number--;
            LFSM_RMUTEX_UNLOCK(&gc[id_disk]->whole_lock);
            (*pRetBadBlockNum)++;
        }
    }

    return CONT_BBM;
}

/**Boot time query for bad block management (BBM)**/
// return <0 means failure.
// otherwise, return the number of bad block of all ssd devices
int32_t BBM_load_all(struct diskgrp *pgrp)
{
    int32_t i, ret, cn_total;

    cn_total = 0;
    for (i = 0; i < pgrp->cn_max_drives; i++) {
        if ((ret = BBM_load(pgrp, i)) < 0) {
            LOGERR("fail to load bbm for disk %d (ret=%d)\n", i, ret);
            return ret;
        }
        cn_total += ret;
    }

    return cn_total;
}

int32_t BBM_load(struct diskgrp *pgrp, int32_t id_disk)
{
    sector64 pbno;
    int8_t *pbuf;
    int32_t i, cn_bad, cn_total, ret;

    cn_bad = 0;
    cn_total = 0;
    ret = 0;
    pbno = id_disk * pgrp->cn_sectors_per_dev;

    if (NULL == (pbuf = (int8_t *)
                 track_kmalloc(FLASH_PAGE_SIZE, __GFP_ZERO, M_UNKNOWN))) {
        LOGERR("alloc buffer fail\n");
        return -ENOMEM;
    }

    for (i = 0; i < 10; i++) {  // TO FIX: bbm table must be load complete with in 4 times (1024*4 = 4096), //(one fpage is 1024 bad id_eu)
        if (!devio_read_pages((pbno +
                               (i << SECTORS_PER_FLASH_PAGE_ORDER)) |
                              (LFSM_BOOT_QUERY | LFSM_SPECIAL_CMD), pbuf, 1,
                              NULL)) {
            LOGERR("BBM special read fail\n");
            ret = -EIO;
            goto l_end;
        }
//        hexdump(pbuf,8192);
        if ((ret = BBM_parse(pbuf, id_disk, &cn_bad)) < 0) {
            goto l_end;
        }

        cn_total += cn_bad;
        if (ret == END_BBM) {
            break;
        }
        LOGINFO("%dth BBM query for disk %d\n", i, id_disk);
    }

    //printk("badblock count %d\n",cn_total);
    if (ret != END_BBM) {
        ret = -ENFILE;
        goto l_end;
    }
    ret = cn_total;
  l_end:
    track_kfree(pbuf, FLASH_PAGE_SIZE, M_UNKNOWN);
    return ret;

}

void hexdump(int8_t * data, int32_t len)
{
    int32_t i, j;

    printk("-------------------------\n");
    printk("HEX DUMP OF mem %p \n", data);
    printk("-------------------------\n");
    for (i = 0; i < (len / 16); i++) {
        printk("%8p : ", data + i * 16);
        for (j = 0; j < 16; j++) {
            printk("%02x ", (uint8_t) data[i * 16 + j]);
        }

        for (j = 0; j < 16; j++) {
            if (data[i * 16 + j] > 32) {
                printk("%c", data[i * 16 + j]);
            } else {
                printk(".");
            }
        }
        printk("\n");
    }
}

/** special read command for enquiry about what went wrong **/
// the function will return the number of bad block, which is equal to bad_blk_info.num_bad_items
// below func return -EIO, -ENODEV, 0, or the number of fail items
// query_num : if BlkUnit=0 then query_range is in page else in SEU
int32_t special_read_enquiry(sector64 bi_sector,
                             int32_t query_num, int32_t org_cmd_type,
                             int32_t BlkUnit,
                             struct bad_block_info *bad_blk_info)
{
    int32_t i;

    if (BlkUnit != 0 || query_num > 64) {
        return -EIO;            // cannot handle Unit= SEU || query_num >64
    }
    // fill in
    bad_blk_info->num_bad_items = query_num;

    for (i = 0; i < query_num; i++) {
        bad_blk_info->errors[i].failure_code = LFSM_ERR_WR_ERROR;
        bad_blk_info->errors[i].logical_num =
            bi_sector + i * SECTORS_PER_FLASH_PAGE;
    }

    return query_num;
}

//TODO: 20160905-lego no more maintain, remove it
/* Query the sectors to read the information in spare areas */
/* Set the 3 MSBs to 1 for the special lpn query. */
int32_t build_lpn_query(lfsm_dev_t * td, sector64 target_position,
                        struct page *ret_page)
{
    struct bio_container *bio_ct;
    struct bio *org_bioct_bio;
    sector64 *lpns;
    int32_t err, i;
    uint8_t *source, *dest;

    err = 0;
    i = 0;

    LFSM_ASSERT(ret_page != NULL);

    bio_ct = get_bio_container(td, NULL, 0, 0, 0, DONTCARE_TYPE, READ);
    LFSM_ASSERT(bio_ct);
    org_bioct_bio = bio_ct->bio;

    err =
        compose_bio(&bio_ct->bio, NULL, end_bio_special_rd_io, bio_ct,
                    PAGE_SIZE, 1);
    if (err != 0) {
        goto end_bio_ct;
    }

    bio_ct->bio->bi_opf = REQ_OP_READ;
    bio_ct->bio->bi_next = NULL;

    bio_ct->bio->bi_iter.bi_sector = target_position;

    bio_ct->bio->bi_iter.bi_sector |= LFSM_LPN_QUERY;

    atomic_set(&bio_ct->active_flag, ACF_ONGOING);

    LOGINFO("LPN query org sector %llu bv_page(%p)\n", target_position,
            bio_ct->bio->bi_io_vec[0].bv_page);
    dprintk("Original sector %llx transformed sector %llx\n",
            target_position, bio_ct->bio->bi_iter.bi_sector);
    if (my_make_request(bio_ct->bio) == 0) {
        //lfsm_wait_event_interruptible(bio_ct->io_queue,
        //        atomic_read(&bio_ct->active_flag) != ACF_ONGOING);
        _cmd_io_wait_timeout(bio_ct, 0);
    }

    if (atomic_read(&bio_ct->active_flag) == ACF_PAGE_READ_DONE) {
        source = kmap(bio_ct->bio->bi_io_vec[0].bv_page);
        dest = kmap(ret_page);

        memcpy(dest, source, PAGE_SIZE);
        lpns = (sector64 *) (dest + LPN_QUERY_HEADER);  // Header size is 32.

        for (i = 0; i < ((PAGE_SIZE - LPN_QUERY_HEADER) / sizeof(sector64));
             i++) {
            tf_printk("\nLBA %llu(%llx) PBA %llu", lpns[i], lpns[i],
                      target_position + (i * sizeof(sector64)));
        }

        kunmap(bio_ct->bio->bi_io_vec[0].bv_page);
        kunmap(ret_page);
    } else {
        LOGERR("[LFSM] Error in LPN query %d\n",
               atomic_read(&bio_ct->active_flag));
        err = 1;
    }

    atomic_set(&bio_ct->active_flag, ACF_ONGOING);

    free_composed_bio(bio_ct->bio, bio_ct->bio->bi_vcnt);

  end_bio_ct:
    bio_ct->bio = org_bioct_bio;

    put_bio_container(td, bio_ct);
    return err;
}

bool read_updatelog_page(sector64 pos, struct bio_container *ul_read_bio)
{
    bool ret = true;

    ul_read_bio->bio->bi_iter.bi_size = PAGE_SIZE * MAX_MEM_PAGE_NUM;   // ul_read_bio->bio is crate during bio_container is allocated
    ul_read_bio->bio->bi_next = NULL;
    ul_read_bio->bio->bi_iter.bi_idx = 0;

    ul_read_bio->bio->bi_disk = NULL;
    atomic_set(&ul_read_bio->active_flag, ACF_ONGOING);
    ul_read_bio->bio->bi_opf = REQ_OP_READ;
    ul_read_bio->bio->bi_iter.bi_sector = pos;
    if (my_make_request(ul_read_bio->bio) == 0) {
        //lfsm_wait_event_interruptible(ul_read_bio->io_queue,
        //        atomic_read(&ul_read_bio->active_flag) != ACF_ONGOING);
        _cmd_io_wait_timeout(ul_read_bio, 0);
    }

    if (atomic_read(&ul_read_bio->active_flag) != ACF_PAGE_READ_DONE) {
        ret = false;
    }

    return ret;
}
