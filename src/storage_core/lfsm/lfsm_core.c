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
/* This is the core source file for the flash disk project
 */

#include <linux/version.h>
#include <linux/major.h>
#include <linux/time.h>
#include <linux/blkdev.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/sched/signal.h>
#include <linux/fs.h>
#include <linux/bio.h>
#include <linux/stat.h>
#include <linux/errno.h>
#include <linux/file.h>
#include <linux/ioctl.h>
#include <linux/ide.h>
#include <linux/genhd.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/timer.h>
#include <linux/time.h>
#include <linux/kdev_t.h>
#include <linux/slab.h>

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
#include "bmt.h"
#include "ioctl.h"
#include "bmt_commit.h"
#include "io_manager/mqbiobox.h"
#include "io_manager/io_common.h"
#include "EU_create.h"
#include "GC.h"
#include "system_metadata/Super_map.h"
#include "special_cmd.h"
#include "dbmtapool.h"
#include "err_manager.h"

#include "lfsm_test.h"
#include "lfsm_info.h"
#include "autoflush.h"
#include "system_metadata/Dedicated_map.h"
#include "spare_bmt_ul.h"
#include "erase_count.h"
#include "devio.h"
#include "batchread.h"
#include "devio.h"
#include "hpeu.h"
#include "ecc.h"
#include "hpeu_gc.h"
#include "mdnode.h"
#include "metalog.h"
#include "common/tools.h"

#include "bio_ctrl.h"

#include "sysapi_linux_kernel.h"
#include "lfsmprivate.h"
#ifdef THD_EXEC_STEP_MON
#include "lfsm_thread_monitor.h"
#endif
#include "lfsm_blk_dev.h"
#include "metalog.h"
#include "perf_monitor/thd_perf_mon.h"
#include "perf_monitor/ioreq_pendstage_mon.h"
#include "perf_monitor/ssd_perf_mon.h"

static int32_t lfsm_bg_thread_fn(void *foo)
{
    lfsm_dev_t *sd;
    int32_t *ar_gc_flag;        //[LFSM_PGROUP_NUM]={0};
    int64_t min_sleep_time;
    int32_t ret, residue, i, idx_disk;

    sd = foo;
    current->flags |= PF_MEMALLOC;
    idx_disk = 0;

    ar_gc_flag = (int32_t *)
        track_kmalloc(sizeof(int32_t) * lfsmdev.cn_pgroup, GFP_KERNEL, M_GC);

    for (i = 0; i < lfsmdev.cn_pgroup; i++) {
        ar_gc_flag[i] = 0;
    }

    LFSM_ASSERT(sd != NULL);
    LOGINFO("gc[0]=%p \n", gc[0]);
    while (true) {

//        DECLARE_WAITQUEUE(wait, current);
//        add_wait_queue(&sd->worker_queue, &wait);
        min_sleep_time = lfsmCfg.gc_chk_intvl;

        set_current_state(TASK_INTERRUPTIBLE);
        lfsm_state_bg_thread = 1;
        residue = wait_event_interruptible_timeout(sd->worker_queue,
                                                   kthread_should_stop() ||
                                                   atomic_read(&sd->dbmtapool.
                                                               dbmtE_bk_used) >=
                                                   (sd->dbmtapool.max_bk_size -
                                                    DBMTAPOOL_ALLOC_LOWNUM)
                                                   || fg_gc_continue == true,
                                                   min_sleep_time);
        lfsm_state_bg_thread = 2;

        if (kthread_should_stop()) {
            break;
        }

        /* make swsusp happy with our thread */
        if (current->flags & PF_FROZEN) {
            printk("PF_FREEZE\n");
            //refrigerator();
        }

        if (signal_pending(current)) {
            flush_signals(current);
            printk("Got signal\n");
        }

        if ((ret = HPEUgc_process(sd, ar_gc_flag)) <= 0) {
            if (ret < 0) {
                LOGERR("Non normal exist HPEUgc process id_disk %d\n",
                       idx_disk);
            }
            fg_gc_continue = false;
        } else {
            fg_gc_continue = true;
        }

        lfsm_state_bg_thread = 3;
        for (idx_disk = 0; idx_disk < lfsmdev.cn_ssds; idx_disk++) {
#if LFSM_REVERSIBLE_ASSERT == 1
            LFSM_SHOULD_PAUSE();
#else
            LFSM_ASSERT(assert_flag == 0);
#endif
            lfsm_state_bg_thread = (idx_disk + 1) * 10;

            lfsm_state_bg_thread++;
            BMT_commit_manager(sd, sd->ltop_bmt);
            lfsm_state_bg_thread++;
            BMT_cache_commit(sd, sd->ltop_bmt);

        }
        lfsm_state_bg_thread = 1000;

//        remove_wait_queue(&sd->worker_queue, &wait);

        if (trim_bio_shall_reset_UL(&lfsmdev.UpdateLog)) {
            LOGINFO("Force updatelog reset due to trim bio\n");
            UpdateLog_reset_withlock(&lfsmdev);
        }
    }
    track_kfree(ar_gc_flag, sizeof(int32_t) * lfsmdev.cn_pgroup, M_GC);
    return 0;
}

bool pbno_lfsm2bdev_A(sector64 pbno, sector64 * pbno_af_adj, int32_t * dev_id,
                      diskgrp_t * pgrp)
{
    sector64 total_size;
    int32_t i;

    total_size = 0;
    for (i = 0; i < pgrp->cn_max_drives; i++) {
        total_size += pgrp->ar_drives[i].disk_size;
        if (pbno < total_size) {
            break;
        }
    }

    if (i >= pgrp->cn_max_drives) {
        LOGINERR("over max drives disk_num %d sector %llx \n", i, pbno);
        dump_stack();
        return false;
    }

    if (dev_id) {
        (*dev_id) = i;
    }

    if (pbno_af_adj) {
        (*pbno_af_adj) = pgrp->ar_drives[i].disk_size - (total_size - pbno);
    }

    return true;
}

inline bool pbno_lfsm2bdev(sector64 pbno, sector64 * pbno_af_adj,
                           int32_t * dev_id)
{
    return (pbno_lfsm2bdev_A(pbno, pbno_af_adj, dev_id, &grp_ssds));
}

static int32_t BiSectorAdjust_lfsm2bdev(struct bio *bio, diskgrp_t * pgrp)
{
    sector64 saved_bits;
    int32_t dev_id;

    dev_id = 0;
    saved_bits = 0;
    if (pgrp->phymedia == PHYMEDIA_HDD) {
        bio->bi_iter.bi_sector &= ~MAPPING_TO_HDD_BIT;  //remove MAP_TO_HDD_BIT
    } else {                    // imply phymedia == PHYMEDIA_CCMASSD
        saved_bits = bio->bi_iter.bi_sector & LFSM_SPECIAL_MASK;
        bio->bi_iter.bi_sector &= ~LFSM_SPECIAL_MASK;
    }

    if (!pbno_lfsm2bdev_A
        (bio->bi_iter.bi_sector, (sector64 *) (&(bio->bi_iter.bi_sector)),
         &dev_id, pgrp)) {
        bio->bi_iter.bi_sector |= saved_bits;
        return -EINVAL;
    }

    dprintk("%s: disk %d bio->bi_disk %p bi_sector %llu\n", __FUNCTION__,
            dev_id, bio->bi_disk, bio->bi_iter.bi_sector);

    if (bio->bi_iter.bi_sector + (bio->bi_iter.bi_size >> SECTOR_ORDER) >
        pgrp->ar_drives[dev_id].disk_size) {
        LOGERR
            ("needed a bio split!! sector %llu (0x%llu) size %u disk_size %llu "
             "in sectors\n", (sector64) bio->bi_iter.bi_sector,
             (sector64) bio->bi_iter.bi_sector,
             bio->bi_iter.bi_size / SECTOR_SIZE,
             pgrp->ar_drives[dev_id].disk_size);
        bio->bi_iter.bi_sector |= saved_bits;
        return -EINVAL;
    }

    bio->bi_iter.bi_sector |= saved_bits;
    return dev_id;
}

// return -1 means fail, other return disk_id
static int32_t BiSectorAdjust_bdev2lfsm_A(struct bio *bio, diskgrp_t * pgrp)
{
    int32_t i, ret;

    ret = -1;
    for (i = 0; i < pgrp->cn_max_drives; i++) {
        if (pgrp->ar_drives[i].bdev->bd_disk == bio->bi_disk) {
            ret = i;
            break;
        }

        bio->bi_iter.bi_sector += pgrp->ar_drives[i].disk_size;
    }

    bio->bi_disk = NULL;
    return ret;
}

int32_t BiSectorAdjust_bdev2lfsm(struct bio *bio)
{
    return (BiSectorAdjust_bdev2lfsm_A(bio, &grp_ssds));
}

// return 0 if success...
inline int32_t my_make_request(struct bio *bio)
{
    diskgrp_t *pgrp;
    int32_t ret;
    int32_t id_dev;
    struct block_device *bdev;

    pgrp = &grp_ssds;
    if ((id_dev = BiSectorAdjust_lfsm2bdev(bio, pgrp)) < 0) {
        return id_dev;
    }

    if (IS_ERR_OR_NULL(bdev = diskgrp_get_drive(pgrp, id_dev))) {   //pgrp->ar_drives[dev_id].bdev;
        return -ENODEV;
    }
    bio_set_dev(bio, bdev);
    ret = devio_make_request(bio, (pgrp->phymedia == PHYMEDIA_CCMASSD));
    diskgrp_put_drive(pgrp, id_dev);
    return ret;
}

//FIXME: wrong here, only check first queue
bool lfsm_idle(lfsm_dev_t * td)
{
    return (get_total_waiting_io(&(td->bioboxpool)) == 0);
}

static void lfsm_thread_destroy(lfsm_thread_t * pmonitor)
{
    if (IS_ERR_OR_NULL(pmonitor->pthread)) {
        return;
    }

    if (kthread_stop(pmonitor->pthread) < 0) {
        return;
    }

    pmonitor->pthread = NULL;
}

static int32_t lfsm_thread_init(lfsm_thread_t * pmonitor,
                                int32_t(*threadfunc) (void *arg), void *arg,
                                int8_t * name)
{
    init_waitqueue_head(&pmonitor->wq);
    pmonitor->pthread = kthread_run(threadfunc, arg, name);
    if (IS_ERR_OR_NULL(pmonitor->pthread)) {
        return -ENOMEM;
    }
#ifdef THD_EXEC_STEP_MON
    reg_thread_monitor(pmonitor->pthread->pid, name);
#endif

    return 0;
}

static int32_t lfsm_idle_monitor(void *pobj)
{
    lfsm_dev_t *td;

    td = (lfsm_dev_t *) pobj;
    current->flags |= PF_MEMALLOC;
    set_current_state(TASK_INTERRUPTIBLE);
    while (true) {
        // each 3s wake and chk cn_io in mqbiobox, if no io in mqbiobox, do flaush metalog
        wait_event_interruptible_timeout(td->monitor.wq, kthread_should_stop(),
                                         3000);

        if (kthread_should_stop()) {
            break;
        }
        //Ding: LFSM_STAT_REJECT for scale up (dirty solution)
        if (!lfsm_idle(td) || atomic_read(&td->dev_stat) == LFSM_STAT_REJECT) {
            continue;
        }
//        LOGINFO("wake up and idle 3s\n");

        metalog_packet_cur_eus_all(&td->mlogger, gc);
    }
    return 0;
}

static int32_t open_diskgrps(lfsm_dev_t * td, int32_t total_ssds)
{
    sector64 total_SEU;
    int32_t ret, i;

    //TODO: scheduler configurable and let user daemon proceed
    diskgrp_init(&grp_ssds, td->cn_ssds, PHYMEDIA_CCMASSD, td->cn_pgroup,
                 "noop");

    if (super_map_allocEU(&grp_ssds, 0) < 0) {
        LOGERR("fail alloc EU for super map when open disks");
        return -ENOMEM;
    }

    for (i = 0; i < total_ssds; i++) {
        if (diskgrp_open_dev(td, i, false) < 0) {
            LOGERR("fail open disk %d\n", i);
            return -ENOMEM;
        }
        reg_ssd_perf_monitor(i, lfsmCfg.ssd_path[i]);
    }

    if ((ret = diskgrp_check_and_complete_disknum(&grp_ssds)) < 0) {
        return ret;
    }
//AN :physical capacity is sector
//logical capacity is page based
    total_SEU = grp_ssds.capacity >> (SECTORS_PER_SEU_ORDER);
    //total_SEU -= total_SEU >> RESERVED_RATIO; // reserved ratio = 1/4 //Tifa, It's a rough count for reserved space (system EU and bad block)
    total_SEU = total_SEU - ((total_SEU * lfsmCfg.gc_seu_reserved) / 1000);

    td->logical_capacity = total_SEU * FPAGE_PER_SEU;
    //td->logical_capacity = FPAGE_PER_SEU*16;

    LOGINFO("SEU %llu Logical capacity: %llu pages OR %llu sectors\n",
            (sector64) total_SEU, (sector64) td->logical_capacity,
            (sector64) SECTORS_PER_FLASH_PAGE * td->logical_capacity);

    return 0;
}

static int32_t deconfigure_disks(void)
{
    LOGINFO("free diskgrp ssds\n");
    diskgrp_destory(&grp_ssds);
    return 0;
}

int32_t reinit_lfsm_subisks(lfsm_dev_t * td, MD_L2P_table_t * bmt_orig,
                            sector64 total_SEU_orig, int32_t cn_ssd_orig)
{
    int32_t i, cn_supermap_fail;
    struct HListGC **gc_new, **gc_orig;
    struct EUProperty **pbno_eu_map_new, **pbno_eu_map_orig;
    sector64 total_free_eus, disk_size, free_eus_in_disk, start_of_disk;

    disk_size = 0;
    start_of_disk = 0;
    free_eus_in_disk = 0;
    cn_supermap_fail = 0;

    if (!grp_ssds.capacity) {
        LOGERR("init subdisk fail grp_ssds.capacity = 0\n");
        return -EINVAL;
    }

    total_free_eus = grp_ssds.capacity >> (SECTORS_PER_SEU_ORDER);
//  LOGINFO("Total physical capacity %llu total EUs %llu\n",
//        grp_ssds.capacity, total_free_eus);

    if (IS_ERR_OR_NULL(pbno_eu_map_new = (struct EUProperty **)
                       track_kmalloc(total_free_eus *
                                     sizeof(struct EUProperty *), GFP_KERNEL,
                                     M_GC))) {
        LOGERR("Error allocating memory for pbno_eu_map\n");
        return -ENOMEM;
    }

    memcpy(pbno_eu_map_new, pbno_eu_map,
           total_SEU_orig * sizeof(struct EUProperty *));
    pbno_eu_map_orig = pbno_eu_map;
    pbno_eu_map = pbno_eu_map_new;
    track_kfree(pbno_eu_map_orig, total_SEU_orig * sizeof(struct EUProperty *),
                M_GC);

    total_free_eus = 0;
    if (IS_ERR_OR_NULL(gc_new = (struct HListGC **)
                       track_kmalloc(sizeof(struct HListGC *) * td->cn_ssds,
                                     GFP_KERNEL, M_GC))) {
        LOGERR("Error allocating memory for gc\n");
        return -ENOMEM;
    }

    memset(gc_new, 0, sizeof(struct HListGC *) * td->cn_ssds);
    memcpy(gc_new, gc, sizeof(struct HListGC *) * cn_ssd_orig);

    gc_orig = gc;
    gc = gc_new;
    track_kfree(gc_orig, sizeof(struct HListGC *) * cn_ssd_orig, M_GC);

    for (i = 0; i < cn_ssd_orig; i++) {
        init_waitqueue_head(&grp_ssds.ar_subdev[i].super_queue);
        //mutex_init(&grp_ssds.ar_subdev[i].supmap_lock);
        init_waitqueue_head(&gc[i]->io_queue);
    }

    for (i = cn_ssd_orig; i < td->cn_ssds; i++) {
        grp_ssds.ar_subdev[i].log_start = SUPERMAP_SIZE;

        LOGINFO("init subdisk %d ..............\n", i);
//      DN_PRINTK("alloc mem\n");
        if (IS_ERR_OR_NULL(gc[i] = (struct HListGC *)
                           track_kmalloc(sizeof(struct HListGC), GFP_KERNEL,
                                         M_GC))) {
            LOGERR("Error allocating memory for gc of disk number %d\n", i);
            return -ENOMEM;
        }
        gc[i]->disk_num = i;
        disk_size = grp_ssds.ar_drives[i].disk_size;

        free_eus_in_disk = disk_size >> (SECTORS_PER_SEU_ORDER);
        start_of_disk = disk_size * i;
        total_free_eus = free_eus_in_disk * i;
//      DN_PRINTK("EU_init, free_eus_in_disk=%llu, total_free_eus=%llu\n", free_eus_in_disk, total_free_eus);
        if (EU_init(td, gc[i], TEMP_LAYER_NUM, free_eus_in_disk,
                    EU_SIZE >> SECTOR_ORDER, 0, start_of_disk, total_free_eus,
                    i)) {
            LOGERR("err: EU_init fail\n");
            return -ENOMEM;
        }

        super_map_initEU(&grp_ssds, gc[i], total_free_eus, free_eus_in_disk, i);
        if (super_map_init(td, i, grp_ssds.ar_subdev)) {
            LOGWARN("super map init fail\n");
            return false;
        }

        if (super_map_get_last_entry(i, &grp_ssds) < 0) {
            LOGWARN("super map[%d] read fail\n", i);
            cn_supermap_fail++;
        }
        //start_of_disk += disk_size;
        //total_free_eus += free_eus_in_disk;
    }

    metalog_eulog_reinit(&td->mlogger, cn_ssd_orig);

    BMT_setup_for_scale_up(td, td->ltop_bmt, bmt_orig,
                           td->ltop_bmt->BMT_num_EUs, bmt_orig->BMT_num_EUs,
                           cn_ssd_orig);

    for (i = cn_ssd_orig; i < td->cn_ssds; i++) {
        metalog_eulog_initA(&td->mlogger.par_eulog[i], i, false);
    }

    for (i = cn_ssd_orig; i < grp_ssds.cn_max_drives; i++) {
        grp_ssds.ar_subdev[i].load_from_prev = all_ready;
    }

    for (i = cn_ssd_orig; i < td->cn_ssds; i++) {
        ExtEcc_eu_init(td, i, false);
        LOGINFO("External ECC start: %llu %d\n",
                td->ExtEcc[i].eu_main->start_pos,
                td->ExtEcc[i].eu_main->disk_num);
    }

    if (dedicated_map_logging(td) < 0) {
        LOGERR("ddmap logging fail\n");
        return -EIO;
    }

    super_map_logging_append(td, &grp_ssds, cn_ssd_orig);
    //super_map_logging_all(td, &grp_ssds);

    metalog_create_thread_append(&td->mlogger, cn_ssd_orig);

    for (i = cn_ssd_orig; i < td->cn_ssds; i++) {
        if (!active_eu_init(td, gc[i])) {
            return -ENOEXEC;
        }
    }

    return 0;

}

static int32_t init_lfsm_subdisks(lfsm_dev_t * td, MD_L2P_table_t ** bmt)
{
    sector64 disk_size, start_of_disk, free_eus_in_disk, total_free_eus;
    int32_t id_dev, i, cn_supermap_fail, disk;

    disk_size = 0;
    start_of_disk = 0;
    free_eus_in_disk = 0;
    cn_supermap_fail = 0;
    if (!grp_ssds.capacity) {
        LOGERR("init subdisk fail grp_ssds.capacity = 0\n");
        return -ENOMEM;
    }

    total_free_eus = grp_ssds.capacity >> (SECTORS_PER_SEU_ORDER);
    LOGINFO("Total physical capacity %llu total EUs %llu\n", grp_ssds.capacity,
            total_free_eus);

    /** Allocate memory for the global Pbno to EU map.**/
    if (IS_ERR_OR_NULL(pbno_eu_map = (struct EUProperty **)
                       track_kmalloc(total_free_eus *
                                     sizeof(struct EUProperty *), GFP_KERNEL,
                                     M_GC))) {
        LOGERR("Error allocating memory for pbno_eu_map\n");
        return -ENOMEM;
    }

    total_free_eus = 0;
    if (IS_ERR_OR_NULL(gc = (struct HListGC **)
                       track_kmalloc(sizeof(struct HListGC *) * td->cn_ssds,
                                     GFP_KERNEL, M_GC))) {
        LOGERR("Error allocating memory for gc\n");
        return -ENOMEM;
    }

    memset(gc, 0, sizeof(struct HListGC *) * td->cn_ssds);

    for (i = 0; i < td->cn_ssds; i++) {
        grp_ssds.ar_subdev[i].log_start = SUPERMAP_SIZE;

        /// Allocate a GC structure for each disk as each disk will have a seperate free list ///
        LOGINFO("init subdisk %d ..............\n", i);
        if (IS_ERR_OR_NULL(gc[i] = (struct HListGC *)
                           track_kmalloc(sizeof(struct HListGC), GFP_KERNEL,
                                         M_GC))) {
            LOGERR("Error allocating memory for gc of disk number %d\n", i);
            return -ENOMEM;
        }

        gc[i]->disk_num = i;
#ifdef  HLIST_GC_WHOLE_LOCK_DEBUG
        gc[i]->access_cnt = 0;
#endif
        disk_size = grp_ssds.ar_drives[i].disk_size;

        free_eus_in_disk = disk_size >> (SECTORS_PER_SEU_ORDER);

        // All EUs of a particular disk go to the free pool of that disk at this stage //
        // In categorize_EUs_based_on_bitmap they would be categorized properly under Heap, Active EU and Free List //
        if (EU_init(td, gc[i], TEMP_LAYER_NUM, free_eus_in_disk,
                    EU_SIZE >> SECTOR_ORDER, 0, start_of_disk, total_free_eus,
                    i)) {
            LOGERR("err: EU_init fail\n");
            return -ENOMEM;
        }

        super_map_initEU(&grp_ssds, gc[i], total_free_eus, free_eus_in_disk, i);
        if (super_map_init(td, i, grp_ssds.ar_subdev)) {
            LOGWARN("super map init fail\n");
            return false;
        }

        if (super_map_get_last_entry(i, &grp_ssds) < 0) {
            LOGWARN("super map[%d] read fail\n", i);
            cn_supermap_fail++;
        }

        start_of_disk += disk_size;
        total_free_eus += free_eus_in_disk;

        LOGINFO("Log SB%d: Start:%llu (sector) Size:%llu (sector)\n",
                i, grp_ssds.ar_subdev[i].log_start,
                grp_ssds.cn_sectors_per_dev);
    }

    td->start_sector = grp_ssds.ar_subdev[0].log_start;
    eu_log_start = 0;

    if (cn_supermap_fail == td->cn_ssds) {
        LOGERR("super map on all disk fails %d\n", cn_supermap_fail);
        return -EINVAL;
    }

    td->super_map_primary_disk = super_map_get_primary(td);
    if (grp_ssds.ar_subdev[td->super_map_primary_disk].load_from_prev ==
        non_ready) {
        td->prev_state = sys_fresh;
    }

    if (generate_BMT(td, td->ltop_bmt->BMT_num_EUs, &grp_ssds)) {
        LOGERR("fail gen BMT\n");
        return -ENOEXEC;
    }

    if (generate_bitmap_and_validnum(td, pbno_eu_map)) {
        LOGERR("fail gen bitmap and valid num\n");
        return -ENOEXEC;
    }

    if (categorize_EUs_based_on_bitmap(td)) {
        LOGERR("fail categorize EUs\n");
        return -ENOEXEC;
    }

    if (ExtEcc_eu_set_mdnode(td)) {
        LOGERR("fail set ExtEcc mdnode\n");
        return -ENOEXEC;
    }

    if ((id_dev = rddcy_search_failslot(&grp_ssds)) >= 0) {
        for_rddcy_faildrive_get(id_dev, disk, i) {
            LOGINFO("found a fail disk num = %d\n", disk);
            if (HPEU_set_allstate(&td->hpeutab, disk, damege) < 0) {
                LOGWARN
                    ("fail to change all corresponding hpeus of disk %d to damege\n",
                     id_dev);
            }
        }
        rddcy_rebuild_all(&td->hpeutab, &grp_ssds);
    }

    return 0;
}

/* Cleanup the subdev disks
 */
extern int32_t cleanup_lfsm_subdisks(lfsm_dev_t * td, diskgrp_t * pgrp)
{
    int32_t ret, ret_final, disk_num;

    disk_num = 0;
    ret_final = 0;

    //if(gc==0)
    //    return ret_final; // should be check inside the function

    super_map_freeEU(pgrp);

    ExtEcc_destroy(td);

    if ((ret = EU_destroy(td)) < 0) {
        ret_final = ret;
    }
    /* Free all EUs and the pbno_eu_map */
    EU_cleanup(td);

    if (gc != 0) {
        for (disk_num = 0; disk_num < td->cn_ssds; disk_num++) {
            if (IS_ERR_OR_NULL(gc[disk_num])) {
                continue;
            }
            track_kfree(gc[disk_num], sizeof(struct HListGC), M_GC);
        }
        track_kfree(gc, sizeof(struct HListGC *) * td->cn_ssds, M_GC);
        gc = 0;
    }

    for (disk_num = 0; disk_num < td->cn_ssds; disk_num++) {
        super_map_destroy(td, disk_num, pgrp->ar_subdev);
    }

    UpdateLog_destroy(&td->UpdateLog);
//        close_bdev_excl(td->subdev[i].bdev);
    deconfigure_disks();

    return ret_final;
}

/* Allocate free bios for the metadata records
 * @cn_data_bioc: number of data bios
 * return true:  grow fail
 *        false: grow ok
 */
static bool grow_bio_list(lfsm_dev_t * td, int32_t cn_data_bioc)
{
    int32_t i;
    //gar_pbioc = (struct bio_container **)kmalloc((td->freedata_bio_capacity+CN_EXTRA_BIOC)*sizeof(struct bio_container *),GFP_KERNEL|__GFP_ZERO);
    gar_pbioc = (struct bio_container **)vmalloc((td->freedata_bio_capacity +
                                                  CN_EXTRA_BIOC) *
                                                 sizeof(struct bio_container
                                                        *));
    atomic_set(&td->cn_extra_bioc, 0);

    init_waitqueue_head(&td->wq_datalog);
    atomic_set(&td->len_datalog_free_list, 0);
    for (i = 0; i < cn_data_bioc; i++) {
        bioc_alloc_add2list(td, i % DATALOG_GROUP_NUM);
        if (i % 100000 == 0) {
            LOGINFO("bioc allocing %d/%d\n", i, cn_data_bioc);
        }
    }

    return false;
}

//generate the per_page_active_list_item, and hook it on the list in td
static int32_t grow_per_page_list(lfsm_dev_t * td, int32_t nr_per_page_list)
{
    ppage_activelist_item_t *pItem;
    unsigned long perpagepool_list_flag;
    int32_t retry;

    td->per_page_pool_cnt = nr_per_page_list;
    retry = 0;
    while (nr_per_page_list) {
        pItem = (ppage_activelist_item_t *)
            track_kmalloc(sizeof(ppage_activelist_item_t), GFP_KERNEL, M_IO);
        if (IS_ERR_OR_NULL(pItem)) {
            LOGWARN("Error in alloc_per_page_active_list (retry=%d)\n", retry);
            schedule();
            if ((retry++) > LFSM_MAX_RETRY) {
                return false;
            }
            continue;
        }

        spin_lock_irqsave(&td->per_page_pool_lock, perpagepool_list_flag);
        list_add(&pItem->page_list, &td->per_page_pool_list);
        spin_unlock_irqrestore(&td->per_page_pool_lock, perpagepool_list_flag);
        nr_per_page_list--;
    }
    return true;
}

/* Free bios for the metadata records
 */
static void shrink_bio_list(lfsm_dev_t * td)
{
    struct bio_container *bio_ct, *next;
    int32_t i;

    LOGINFO("pbioc extra num: %d\n", atomic_read(&td->cn_extra_bioc));
    for (i = 0; i < DATALOG_GROUP_NUM; i++) {
        if (list_empty_check(&lfsmdev.datalog_free_list[i])) {
            return;
        }

        spin_lock(&td->datalog_list_spinlock[i]);
        list_for_each_entry_safe(bio_ct, next, &td->datalog_free_list[i], list) {
            free_bio_container(bio_ct, MAX_MEM_PAGE_NUM);
        }
        spin_unlock(&td->datalog_list_spinlock[i]);
    }
    vfree(gar_pbioc);
}

static void free_per_page_active_list(lfsm_dev_t * td)
{
    ppage_activelist_item_t *pItem, *pTmp;
    unsigned long perpagepool_list_flag;

    if (list_empty_check(&lfsmdev.per_page_pool_list)) {
        return;
    }

    spin_lock_irqsave(&td->per_page_pool_lock, perpagepool_list_flag);
    list_for_each_entry_safe(pItem, pTmp, &td->per_page_pool_list, page_list) {
        //Lego: todo Don't call free in spin_lock
        track_kfree(pItem, sizeof(ppage_activelist_item_t), M_IO);
    }
    spin_unlock_irqrestore(&td->per_page_pool_lock, perpagepool_list_flag);
}

int32_t lfsm_write_pause(void)
{
    lfsm_dev_t *td;
    td = &lfsmdev;
    atomic_set(&td->dev_stat, LFSM_STAT_READONLY);
    return 0;
}

/**
   TODO: Do we need a lock before incrementing. ?
**/
// this function only can be call by one thread or it need a lock
uint32_t get_next_disk(void)
{
    static uint32_t current_disk = 0;   //static as globle variable, only init once
    int32_t disk_num, i, tmp_disk;

    disk_num = lfsmdev.cn_ssds;
    tmp_disk = current_disk;
    for (i = 0; i < disk_num; i++) {
        if (lfsmdev.ar_pgroup[PGROUP_ID(tmp_disk)].state != epg_good) {
            tmp_disk++;
        } else if (gc[tmp_disk % disk_num]->free_list_number > MIN_RESERVE_EU) {
            current_disk = tmp_disk + 1;
            return (tmp_disk % disk_num);
        } else {
            tmp_disk++;
        }
    }

    return ((current_disk++) % disk_num);
}

void lfsm_resp_user_bio_free_bioc(lfsm_dev_t * td, struct bio_container *bio_c,
                                  int32_t err, int32_t id_bh_thread)
{
    post_rw_check_waitlist(td, bio_c, err < 0);
    bioc_resp_user_bio(bio_c->org_bio, err);

    put_bio_container(td, bio_c);
    return;
}

#define LFSM_AUTH_KEY_CHECK(X) \
    if (key_hdd!=(uint32_t)key_lfsm_in2) \
    { \
        err = -EFBIG; \
        msleep(X); \
        goto l_fail;         \
    }

/*
 * Description: check user setting about # of total ssds, # of ssds in a group
 *              # of group is well setting
 * Parameters: lfsm_cn_ssds: total ssds in pool
 *             cn_ssds_per_hpeu: # ssds in a group
 *             lfsm_cn_pgroup  : # of groups (max = 2)
 * return: true : not supported setting
 *         false: passing check
 */
static bool _lfsm_param_verify(int32_t lfsm_cn_ssds, int32_t cn_ssds_per_hpeu,
                               int32_t lfsm_cn_pgroup)
{
    bool ret;

    ret = true;

    do {
        if (lfsm_cn_ssds <= 0 || cn_ssds_per_hpeu <= 0 ||
            cn_ssds_per_hpeu > (1 << RDDCY_ECC_MARKER_BIT_NUM)) {
            LOGERR
                ("lfsm_cn_ssds cannot be 0 and cn_ssds_per_hpeu should be 1~%d\n",
                 1 << RDDCY_ECC_MARKER_BIT_NUM);
            break;
        }

        if (lfsm_cn_pgroup <= 0) {
            LOGERR("lfsm_cn_pgroup should be 1~2 "
                   "(lfsm_pgroup_selector can not support more than 2 group)\n");
            break;
        }

        if (lfsm_cn_ssds < (lfsm_cn_pgroup * cn_ssds_per_hpeu)) {
            LOGERR("(lfsm_cn_pgroup*cn_ssds_per_hpeu)>lfsm_cn_ssds\n");
            break;
        }

        if (lfsm_cn_ssds == 1 || cn_ssds_per_hpeu == 1) {
            LOGERR("RDDCY_GENECC cannot be 1 when HPEU disk num = 1\n");
            break;
        }

        ret = false;
    } while (0);

    return ret;
}

/*
 * Description: check whether user has authorized license key
 * return : 0 : pass license check
 *          non 0: not authorized license key
 */
static int32_t _lfsm_license_chk(int64_t user_key, int64_t * lfsm_lic_key,
                                 uint32_t * key_hdd)
{
    int8_t osdevPath[MAX_DEV_PATH];
    int64_t key_lfsm_in;
    uint32_t tm_spin_hour, tm_license_start;
    int32_t ret;

    ret = 0;
    key_lfsm_in = swbitslng(user_key, 1);

    memset(osdevPath, '\0', MAX_DEV_PATH);
    sprintf(osdevPath, "/dev/%s", lfsmCfg.osdisk_name);
    if (devio_get_identify(osdevPath, key_hdd) < 0) {
        return -ESPIPE;
    }

    if (devio_get_spin_hour(osdevPath, &tm_spin_hour) < 0) {
        return -ESPIPE;
    }
#if LFSM_RELEASE_VER == 0 || LFSM_RELEASE_VER == 3
    if (user_key == 0) {
        *key_hdd = 0;
    }
#endif

    if (*key_hdd != (uint32_t) user_key) {
        return -EPERM;
    }

    tm_license_start = swbits((uint64_t) user_key >> 32, 1);

    //LOGINFO("tm_spin: 0x%x tm_license_start: 0x%x, 0x%x 0x%x\n",tm_spin_hour, tm_license_start,
    //         encrype_bits(tm_license_start),(unsigned int)(((unsigned long)key_lfsm_in>>32)) );

#if LFSM_RELEASE_VER == 1
    if (ecompute_bits(tm_license_start) != ((uint64_t) user_key >> 32)) {
        return -EPERM;
    }

    if ((tm_spin_hour < tm_license_start) || (tm_spin_hour > (tm_license_start + 168))) {   // 168 hour
        return -EPERM;
    }
#endif

    *lfsm_lic_key = key_lfsm_in;
    return ret;
}

/*
 * Description: initial some member in lfsm device
 * return : 0 : init ok
 *          non 0 : init fail
 */
static int32_t _init_lfsm_dev(lfsm_dev_t * mydev, int32_t ssds_per_hpeu,
                              int32_t pgroup)
{
    int32_t ret;

    ret = 0;
    memset(mydev, 0, sizeof(lfsm_dev_t));

    atomic_set(&(mydev->dev_stat), LFSM_STAT_INIT);
    atomic_set(&(mydev->cn_alloc_bioext), 0);
    mutex_init(&(mydev->special_query_lock));   //Lego: todo removed casue perf. down?
    atomic64_set(&(mydev->tm_sum_bioc_life), 0);    //Lego: todo removed casue perf. down?
    atomic64_set(&(mydev->cn_sum_bioc_life), 0);    //Lego: todo removed casue perf. down?

#if LFSM_REVERSIBLE_ASSERT == 1
    init_waitqueue_head(&(mydev->wq_assert));
#endif

    mydev->cn_ssds = ssds_per_hpeu * pgroup;
    mydev->cn_pgroup = pgroup;

    gplfsm = mydev;

    return ret;
}

/*
 * Description: initial following components in early stage
 *     memory_tracker : trace the memory consuming
 *     hook           : hook (debug purpose)
 *     pgcache        : page cache for partial IO
 *     ersmgr         : erase manager
 *     hotswap        : hotswap when some disk failure
 * return: 0 : init ok
 *         non 0 : init fail
 */
static int32_t _init_subcomp_A(lfsm_dev_t * mydev)
{
    int32_t ret;

    ret = -ENOMEM;
    do {
        if (init_memory_manager(lfsmCfg.mem_bioset_size)) {
            break;
        }

        init_thd_perf_monitor();
        init_IOR_pStage_mon();
#ifdef THD_EXEC_STEP_MON
        init_thread_monitor();
#endif

        if (hook_init(&mydev->hook) < 0) {
            LOGERR("init hook fail\n");
            break;
        }

        if (pgcache_init(&mydev->pgcache) < 0) {
            LOGERR("init page cache fail\n");
            break;
        }

        if ((ret = ersmgr_init(&mydev->ersmgr)) < 0) {
            LOGERR("init erase manager fail\n");
            break;
        }

        if (hotswap_init(&mydev->hotswap, mydev) < 0) {
            LOGERR("init hotswap fail\n");
            break;
        }

        ret = 0;
    } while (0);

    return ret;
}

static int32_t _reset_data(lfsm_dev_t * mydev)
{
    sector64 erase_addr;
    int32_t i, ret;

    ret = 0;
    erase_addr = 0;

    //Lego: erase super map from starting address
    for (i = 0; i < mydev->cn_ssds; i++) {
        ret = devio_erase(erase_addr, NUM_EUS_PER_SUPER_EU, NULL);
        LOGINFO("REINSTALL=1: Force clean superblock...%s",
                (ret == 0) ? "done\n" : "fail\n");
        erase_addr += grp_ssds.ar_drives[i].disk_size;
    }

    if (mydev->cn_ssds == 1) {
        ret = devio_erase(grp_ssds.ar_drives[0].disk_size - SECTORS_PER_SEU,
                          NUM_EUS_PER_SUPER_EU, NULL);
        LOGINFO("REINSTALL=1: Force clean superblock...addr %llu-%d...%s",
                grp_ssds.ar_drives[0].disk_size, SECTORS_PER_SEU,
                (ret == 0) ? "done\n" : "fail\n");
    }

    if (ret != 0) {
        ret = -EIO;
    }

    return ret;
}

static void find_min_free_eu(struct HListGC **ar_gc,
                             uint32_t * ar_min_free, uint32_t * ar_min_diskid)
{
    int32_t i, j, id_disk;

    for (i = 0; i < lfsmdev.cn_pgroup; i++) {
        ar_min_free[i] = (uint32_t) (-1);
        LOGINFO("pgroup[%d]: ", i);

        for (j = 0; j < lfsmdev.hpeutab.cn_disk; j++) {
            id_disk = HPEU_EU2HPEU_OFF(j, i);
            printk("(%d)%lld ", id_disk, ar_gc[id_disk]->free_list_number);
            if (ar_gc[id_disk]->free_list_number < ar_min_free[i]) {
                ar_min_free[i] = ar_gc[id_disk]->free_list_number;
                ar_min_diskid[i] = id_disk;
            }
        }
        printk("min=%d id_disk = %d\n", ar_min_free[i], ar_min_diskid[i]);
    }
}

sector64 calculate_capacity(lfsm_dev_t * td, sector64 org_size_in_page)
{
    sector64 cn_total_capacity; //page
    uint32_t *ar_min_free;      //[LFSM_PGROUP_NUM];
    uint32_t *ar_min_diskid;    //[LFSM_PGROUP_NUM];
    int32_t i, cn_total_free;

    cn_total_capacity = 0;
    cn_total_free = 0;
    if (IS_ERR_OR_NULL(ar_min_free = (uint32_t *)
                       track_kmalloc(td->cn_pgroup * sizeof(uint32_t),
                                     GFP_KERNEL, M_OTHER))) {
        LOGERR("alloc mem for ar_min_free err\n");
        return cn_total_capacity;
    }
    if (IS_ERR_OR_NULL(ar_min_diskid = (uint32_t *)
                       track_kmalloc(td->cn_pgroup * sizeof(uint32_t),
                                     GFP_KERNEL, M_OTHER))) {
        LOGERR("alloc mem for ar_min_diskid err\n");
        goto l_free_min_free;
    }

    find_min_free_eu(gc, ar_min_free, ar_min_diskid);

    if (td->prev_state != sys_fresh) {
        for (i = 0; i < td->cn_pgroup; i++) {
            td->ar_pgroup[i].min_id_disk = ar_min_diskid[i];
        }
        cn_total_capacity = org_size_in_page;
        goto l_free_min_diskid;
    }

    for (i = 0; i < td->cn_pgroup; i++) {
        ar_min_free[i] -= (TEMP_LAYER_NUM - 1); //Chyi:TEMP_LAYER_NUM-1 = 2 :are the seus for Hot and Warm flag
        ar_min_free[i] -= MIN_RESERVE_EU;   // Weafon: For next turn on after a power-cut case
        td->ar_pgroup[i].min_id_disk = ar_min_diskid[i];
        cn_total_free += ar_min_free[i] * td->hpeutab.cn_disk;
    }

    cn_total_capacity =
        cn_total_free * (FPAGE_PER_SEU - METADATA_SIZE_IN_FLASH_PAGES);
    cn_total_capacity =
        cn_total_capacity * (hpeu_get_maxcn_data_disk(td->hpeutab.cn_disk)) /
        td->hpeutab.cn_disk;

    //cn_total_capacity -= cn_total_capacity >> RESERVED_RATIO;
    cn_total_capacity =
        cn_total_capacity -
        (cn_total_capacity * lfsmCfg.gc_seu_reserved) / 1000;
    if (org_size_in_page < cn_total_capacity) {
        LOGERR("final pg %lld, org pg %lld\n", cn_total_capacity,
               org_size_in_page);
    }

  l_free_min_diskid:
    if (ar_min_diskid) {
        track_kfree(ar_min_diskid, td->cn_pgroup * sizeof(uint32_t), M_OTHER);
    }
  l_free_min_free:
    if (ar_min_free) {
        track_kfree(ar_min_free, td->cn_pgroup * sizeof(uint32_t), M_OTHER);
    }

    return cn_total_capacity;

}

uint64_t get_lfsm_capacity(void)
{
    return calculate_capacity(&lfsmdev, lfsmdev.logical_capacity);
}

int32_t reinit_external_ecc(lfsm_dev_t * mydev, int32_t cn_org_ssd,
                            int32_t cn_new_ssd)
{
    int i;
    struct external_ecc *newExtEcc, *orgExtEcc;

    newExtEcc =
        (struct external_ecc *)track_kmalloc(cn_new_ssd *
                                             sizeof(struct external_ecc),
                                             GFP_KERNEL, M_PGROUP);
    if (IS_ERR_OR_NULL(newExtEcc)) {
        LOGERR("reinit_external_ecc allocate memory failure\n");
        return -ENOMEM;
    }
    orgExtEcc = mydev->ExtEcc;

    for (i = 0; i < cn_new_ssd; i++) {
        mutex_init(&newExtEcc[i].ecc_lock);
    }

    for (i = 0; i < cn_org_ssd; i++) {
        newExtEcc[i].eu_main[0] = orgExtEcc[i].eu_main[0];
        newExtEcc[i].eu_main[1] = orgExtEcc[i].eu_main[1];
        newExtEcc[i].state = orgExtEcc[i].state;
    }
    mydev->ExtEcc = newExtEcc;
    track_kfree(orgExtEcc, cn_org_ssd * sizeof(struct external_ecc), M_PGROUP);
    return 0;
}

static int32_t _init_external_ecc(lfsm_dev_t * mydev)
{
    int i;

    mydev->ExtEcc =
        (struct external_ecc *)track_kmalloc(mydev->cn_ssds *
                                             sizeof(struct external_ecc),
                                             GFP_KERNEL, M_PGROUP);

    for (i = 0; i < mydev->cn_ssds; i++) {
        mutex_init(&mydev->ExtEcc[i].ecc_lock);
    }

    return 0;
}

static int32_t _init_subcomp_B(lfsm_dev_t * mydev, int32_t ssds_hpeu,
                               sector64 * totalPages)
{
    int32_t ret, i;

    ret = -ENOMEM;
    do {
        if (HPEU_init(&mydev->hpeutab,
                      (grp_ssds.cn_sectors_per_dev >> SECTORS_PER_SEU_ORDER) *
                      mydev->cn_pgroup, &lfsmdev, ssds_hpeu)) {
            LOGERR("HPEU_init fail\n");
            break;
        }

        if (lfsm_pgroup_init(mydev) < 0) {
            LOGERR("pgroup_init init fail\n");
            break;
        }

        LOGINFO("Initializing BMT structures "
                "(Queues, Cache, Dependency lists, internal BMT(not used))\n");
        if (create_BMT(&mydev->ltop_bmt, mydev->logical_capacity)) {
            LOGERR("create bmt fail\n");
            break;
        }

        LOGINFO("BMT Structures initialized....Done.\n");
        //LOGINFO("METADATA_SIZE_IN_PAGES %d\n", METADATA_SIZE_IN_PAGES);
        //LOGINFO("NO_OF_METADATA_ENTRIES %d\n", NO_OF_METADATA_ENTRIES);

        // in rddcy max num of bioc used by gccopy is total page in DISK_PER_HPEU. tifa
        mydev->freedata_bio_capacity =
            (1 << (EU_ORDER - FLASH_PAGE_ORDER)) * mydev->cn_ssds * 2;
        // The total pre-reserved bios for realdata read/write

        for (i = 0; i < DATALOG_GROUP_NUM; i++) {
            spin_lock_init(&mydev->datalog_list_spinlock[i]);
            INIT_LIST_HEAD(&mydev->datalog_free_list[i]);
        }
        mydev->data_bio_max_size = MAX_MEM_PAGE_NUM;

        /*list for sharing the pre-reserved bios[127] for read data read/write */
        INIT_LIST_HEAD(&mydev->gc_bioc_list);

        //    ------------------------------------------
        //    create a pool of bio container to link user's bio and low-level bio
        //    ------------------------------------------
        LOGINFO("Allocating memory for %llu bio containers..\n",
                mydev->freedata_bio_capacity);
        //Tifa: don't change following "grow_bio_list" code style, for autotest parsing
        if (grow_bio_list(mydev, mydev->freedata_bio_capacity)) {
            LOGERR("Can't reserve enough space for free bios\n");
            break;
        }

        if (!biocbh_init(&mydev->biocbh, mydev)) {
            LOGERR("Cannot init biocbh\n");
            break;
        }

        if (!sflush_init(mydev, &mydev->crosscopy, ID_USER_CROSSCOPY)) {
            LOGERR("fail init crosscopy\n");
            break;
        }

        if (!sflush_init(mydev, &mydev->gcwrite, ID_USER_GCWRITE)) {
            LOGERR("fail init gcwrite\n");
            break;
        }

        mutex_init(&mydev->per_page_pool_list_lock);    //Lego: todo removed casue perf. down?
        spin_lock_init(&mydev->per_page_pool_lock);

        INIT_LIST_HEAD(&mydev->per_page_pool_list);

        if (!grow_per_page_list(mydev, mydev->freedata_bio_capacity * 2)) { //actually it won't be happened in this fun.
            LOGERR("Can't reserve enough space for per_page_list pool\n");
            break;
        }
        // move before recovery query because spare lpn need to be written in disk
        if (IS_ERR_OR_NULL(mydev->pMdpool = mdnode_pool_create())) {
            LOGERR("lfsm: can't init metadata pool\n");
            break;
        }

        if (dbmtE_BKPool_init(&mydev->dbmtapool, mydev->ltop_bmt->cn_ppq)) {
            LOGERR("lfsm: can't init Direct BMT array pool\n");
            break;
        }

        LOGINFO("Initializing External Ecc ........\n");
        _init_external_ecc(mydev);

        LOGINFO("Initializing sub-disks........\n");
        if ((ret = init_lfsm_subdisks(mydev, &mydev->ltop_bmt)) < 0) {
            LOGERR("init subdisk fail\n");
            break;
        }
        LOGINFO("diskgrp successfully initialized\n");

        //LFSM_AUTH_KEY_CHECK(100000);

        LOGINFO("lfsm create upper layer queue\n");
        if (bioboxpool_init(&mydev->bioboxpool, lfsmCfg.cnt_io_threads,
                            lfsmCfg.cnt_io_threads, request_dequeue, mydev,
                            lfsmCfg.ioT_vcore_map)) {
            LOGERR("lfsm: can't init bioboxpool\n");
            break;
        }

        *totalPages = calculate_capacity(mydev, mydev->logical_capacity);   //all the calculation of the cost eu is in the function
        if (mydev->prev_state == sys_fresh) {
            mydev->logical_capacity = *totalPages;
        }

        dedicated_map_logging(mydev);   // for log total capacity

        //lfsm_pgroup_valid_page_init(mydev->ar_pgroup,
        //        LFSM_CEIL_DIV(mydev->logical_capacity, mydev->cn_pgroup),
        //        LFSM_CEIL_DIV((mydev->logical_capacity>>RESERVED_RATIO),
        //                mydev->cn_pgroup));
        lfsm_pgroup_valid_page_init(mydev->ar_pgroup,
                                    LFSM_CEIL_DIV(mydev->logical_capacity,
                                                  mydev->cn_pgroup),
                                    LFSM_CEIL_DIV(((mydev->logical_capacity *
                                                    lfsmCfg.gc_seu_reserved) /
                                                   1000), mydev->cn_pgroup));

        atomic_set(&mydev->io_task_busy, 0);    //Lego: todo removed casue perf. down?
        atomic_set(&mydev->cn_conflict_pages_r, 0);
        atomic_set(&mydev->cn_conflict_pages_w, 0);

#if LFSM_PREERASE == 1
        if (ersmgr_createthreads(&mydev->ersmgr, mydev)) {  //ding
            LOGWARN("fail create erase thread\n");
            break;
        }
#endif

        //LFSM_AUTH_KEY_CHECK(1000000);
        init_waitqueue_head(&mydev->worker_queue);
        init_waitqueue_head(&mydev->gc_conflict_queue);
        LOGINFO("Run thread lfsm_bg_thread_fn\n");
        lfsmdev.partial_io_thread =
            kthread_run(lfsm_bg_thread_fn, mydev, "lfsm_bg_thread");

        if (IS_ERR_OR_NULL(mydev->partial_io_thread)) {
            LOGERR("Can't start bgthread\n");
            break;
        }
#ifdef THD_EXEC_STEP_MON
        reg_thread_monitor(lfsmdev.partial_io_thread->pid, "lfsm_bg_thread");
#endif

        if (lfsm_thread_init
            (&mydev->monitor, lfsm_idle_monitor, mydev, "lfsm_monitor") < 0) {
            break;
        }

        if (lfsm_readonly == 1) {
            atomic_set(&mydev->dev_stat, LFSM_STAT_READONLY);
        } else {
            atomic_set(&mydev->dev_stat, LFSM_STAT_NORMAL);
        }

        ret = 0;
    } while (0);

    return ret;
}

/*
 * Description: initialize global variable of lfsm
 */
static void _init_globar_var(void)
{
    int32_t i;

    memset(&grp_ssds, 0, sizeof(struct diskgrp));
    gc = NULL;
    pbno_eu_map = NULL;

    atomic_set(&cn_paused_thread, 0);
    //atomic_set(&ioreq_counter, 0);

    /* Create the mapping structures */
    for (i = 0; i < TEMP_LAYER_NUM; i++) {
        atomic_set(&wio_cn[i], 0);
    }
}

int32_t lfsm_init(lfsm_cfg_t * myCfg, int32_t ul_off, int32_t ronly,
                  int64_t lfsm_auth_key)
{
    sector64 TotalPages;
    int64_t key_lfsm_in, key_lfsm_in2;
    uint32_t key_hdd;
    int32_t err;

    TotalPages = 0;
    err = -ENOMEM;

    myCfg->testCfg.crashSimu = false;

    lfsm_resetData = myCfg->resetData;
    ulfb_offset = ul_off;
    lfsm_readonly = ronly;

    LOGINFO("----------------------------------------------------------\n");
    LOGINFO("| Start install lfsmdr module                            |\n");
    LOGINFO("| Parameter: lfsm_reinstall   =%d                        |\n",
            myCfg->resetData);
    LOGINFO("| Parameter: lfsm_cn_ssds     =%d                        |\n",
            myCfg->cnt_ssds);
    LOGINFO("| Parameter: cn_ssds_per_hpeu =%d                        |\n",
            myCfg->ssds_hpeu);
    LOGINFO("----------------------------------------------------------\n");

    if (lfsm_resetData == -1) {
        LOGERR("fail insmod lfsm by missing lfsm_reinstall\n");
        return -E2BIG;
    }

    if ((_lfsm_param_verify
         (myCfg->cnt_ssds, myCfg->ssds_hpeu, myCfg->cnt_pgroup))) {
        LOGERR("fail insmod lfsm by wrong parameters\n");
        err = -ECHILD;
        goto l_fail;
    }

    if ((err = _lfsm_license_chk(lfsm_auth_key, &key_lfsm_in, &key_hdd))) {
        LOGERR("fail insmod lfsm by wrong auth keys\n");
        return err;
    }
    key_lfsm_in2 = key_lfsm_in;

    _init_globar_var();

    if ((err = _init_lfsm_dev(&lfsmdev, myCfg->ssds_hpeu, myCfg->cnt_pgroup))) {
        goto l_fail;
    }

    if ((err = _init_subcomp_A(&lfsmdev))) {
        goto l_fail;
    }

    init_ssd_perf_monitor(myCfg->cnt_ssds);

    if (init_stripe_disk_map(&lfsmdev, myCfg->ssds_hpeu)) {
        goto l_fail;
    }

    if (open_diskgrps(&lfsmdev, myCfg->cnt_ssds) < 0) { //HPEU not init yet
        err = -ECHILD;
        goto l_fail;
    }

    if (lfsm_resetData == 1) {
        if ((err = _reset_data(&lfsmdev))) {
            goto l_fail;
        }
    }
    //LFSM_AUTH_KEY_CHECK(10000);

    if ((err = _init_subcomp_B(&lfsmdev, myCfg->ssds_hpeu, &TotalPages))) {
        goto l_fail;
    }

    lfsmdev.lfsm_major = init_blk_device("lfsm");
    if (lfsmdev.lfsm_major <= 0) {
        err = -EBUSY;
        goto l_fail;
    }

    if ((err = create_disk(&lfsmdev, TotalPages))) {
        goto l_fail;
    }

    init_procfs(&lfsmdev.procfs);

    LOGINFO("LFSM init successfully capacity: %llu page err: %d\n",
            lfsmdev.logical_capacity, err);

    return 0;

  l_fail:
    lfsm_cleanup(myCfg, false);
    return err;
}

void lfsm_cleanup(lfsm_cfg_t * myCfg, bool isNormalExit)
{
    printk("----------------------------------------------------------\n");
    printk("| %s: begin to remove lfsmdr module isNormal=%d  |\n", __FUNCTION__,
           isNormalExit);
    if (myCfg->testCfg.crashSimu)
        printk("| Crash simulation %d                                      |\n",
               myCfg->testCfg.crashSimu);
    printk("----------------------------------------------------------\n");
    if (atomic_read(&lfsmdev.dev_stat) == LFSM_STAT_CLEAN) {
        return;
    }
    atomic_set(&lfsmdev.dev_stat, LFSM_STAT_REJECT);

    release_procfs(&lfsmdev.procfs);
    errmgr_wait_falselist_clean();
    if (!IS_ERR_OR_NULL(lfsmdev.partial_io_thread)) {
        if (kthread_stop(lfsmdev.partial_io_thread) < 0) {
            LOGERR("can't stop partial_io_thread\n");
        }
    }

    ersmgr_destroy(&lfsmdev.ersmgr);    //ding

    bioboxpool_destroy(&lfsmdev.bioboxpool);

    if ((isNormalExit == true) && (lfsmCfg.testCfg.crashSimu == true)) {
        metalog_packet_cur_eus_all(&lfsmdev.mlogger, gc);
    }
    lfsm_thread_destroy(&lfsmdev.monitor);

    if (&lfsmdev.per_page_pool_list_lock) { //Lego: todo removed casue perf. down?
        mutex_destroy(&lfsmdev.per_page_pool_list_lock);
    }

    biocbh_destory(&lfsmdev.biocbh);

    if ((isNormalExit == true) && (lfsmCfg.testCfg.crashSimu == false)) {
        ECT_commit_all(&lfsmdev, false);
    }

    if ((isNormalExit == true) && (lfsmCfg.testCfg.crashSimu == false)) {
        HPEU_save(&lfsmdev.hpeutab);
    }

    if (!IS_ERR_OR_NULL(lfsmdev.ltop_bmt)) {
        flush_clear_bmt_cache(&lfsmdev, lfsmdev.ltop_bmt, isNormalExit);
        BMT_destroy(lfsmdev.ltop_bmt);
    }
    dbmtE_BKPool_destroy(&lfsmdev.dbmtapool);

    sflush_destroy(&lfsmdev.crosscopy);
    sflush_destroy(&lfsmdev.gcwrite);

//    HPEU_destroy(&lfsmdev.hpeutab); move after HPEU_GetTabItem in subdisk

//    super_map_logging_all(&lfsmdev);

    hotswap_destroy(&lfsmdev.hotswap);  // AN

    cleanup_lfsm_subdisks(&lfsmdev, &grp_ssds);
    mdnode_pool_destroy(lfsmdev.pMdpool);
    HPEU_destroy(&lfsmdev.hpeutab);

    metalog_destroy(&lfsmdev.mlogger);

    shrink_bio_list(&lfsmdev);

    free_per_page_active_list(&lfsmdev);
    //HPEU_destroy(&lfsmdev.hpeutab); //hpeu_destroy should before deconfigure_disks()
    lfsm_pgroup_destroy(&lfsmdev);
    pgcache_destroy(&lfsmdev.pgcache);

    tasklet_kill(&lfsmdev.io_task); //Lego: todo removed casue perf. down?
    remove_disk(&lfsmdev);

    rm_blk_device(lfsmdev.lfsm_major);

    if (&lfsmdev.special_query_lock) {  //Lego: todo removed casue perf. down?
        mutex_destroy(&lfsmdev.special_query_lock);
    }

    LOGINFO("unregister_blkdev...done\n");

    rel_IOR_pStage_mon();

#ifdef THD_EXEC_STEP_MON
    rel_thread_monitor();
#endif
    rel_thd_perf_monitor();

    rel_memory_manager();

    free_stripe_disk_map(&lfsmdev, lfsmCfg.ssds_hpeu);

    atomic_set(&lfsmdev.dev_stat, LFSM_STAT_CLEAN);
    LOGINFO("LFSMDEV has been removed successfully\n");
}
