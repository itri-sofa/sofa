/*
 * SPDX-License-Identifier: GPL-2.0-or-later OR Apache-2.0
 * Copyright (C) 2015-2020 Industrial Technology Research Institute
 *
 *
 *
 *  
 */
#include <linux/major.h>
#include <linux/time.h>
#include <linux/blkdev.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/sched.h>
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
#include <linux/types.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include "../config/lfsm_setting.h"
#include "../config/lfsm_feature_setting.h"
#include "../config/lfsm_cfg.h"
#include "../common/common.h"
#include "../common/mem_manager.h"
#include "../common/rmutex.h"
#include "../io_manager/mqbiobox.h"
#include "../io_manager/biocbh.h"
#include "../nextprepare.h"
#include "../diskgrp.h"
#include "Dedicated_map.h"
#include "../lfsm_core.h"
#include "../io_manager/io_common.h"
#include "Super_map.h"
#include "../special_cmd.h"
#include "../GC.h"
#include "../EU_create.h"
#include "../EU_access.h"
#include "../devio.h"

// tifa: 2^nK safety 2012/2/13
/*
 * Description: bio end function
 * Parameter: err : 0: bio execute ok
 *                  ~0: bio execute fail
 */
static void end_bio_super_sector(struct bio *bio)
{
    int32_t dev_id;

    LFSM_ASSERT(bio->bi_private != NULL);
    dev_id = BiSectorAdjust_bdev2lfsm(bio);
    LFSM_ASSERT2(dev_id != -1, " super map bio end "
                 "dev_id %d, bi_sector %llu, bi_disk %p\n",
                 dev_id, (sector64) bio->bi_iter.bi_sector, bio->bi_disk);

    if (!bio->bi_status) {
        grp_ssds.ar_subdev[dev_id].super_flag = 1;
    } else {
        grp_ssds.ar_subdev[dev_id].super_flag =
            blk_status_to_errno(bio->bi_status);
    }

    bio->bi_iter.bi_size = SECTOR_SIZE; //tf:question
    wake_up_interruptible(&grp_ssds.ar_subdev[dev_id].super_queue);

    return;
}

/*
 * Description: Write a new super map record to disk
 *              This should be called every time the dedicated eu changes.
 * return : <= 0: false
 *             1: true
 */
static int32_t super_map_logging(lfsm_dev_t * td, int32_t disk_num,
                                 lfsm_subdev_t * ar_subdev)
{
    lfsm_subdev_t *subdev;
    super_map_entry_t *rec;
    sector64 frontier_local;
    int8_t *addr;
    int32_t i, ret;             //1:correct

    if (lfsm_readonly == 1) {
        return 0;
    }

    subdev = &ar_subdev[disk_num];
    ret = 1;
    mutex_lock(&subdev->supmap_lock);
    frontier_local = subdev->super_frontier;

    LFSM_ASSERT(subdev);
    LFSM_ASSERT(subdev->super_bio);
    LFSM_ASSERT(subdev->super_bio->bi_io_vec[0].bv_page);

    addr = (int8_t *) kmap(subdev->super_bio->bi_io_vec[0].bv_page) +
        subdev->super_bio->bi_io_vec[0].bv_offset;
    LFSM_ASSERT(addr != NULL);
    rec = (super_map_entry_t *) addr;
    memcpy(rec->magicword, TAG_SUPERMAP, sizeof(rec->magicword));
    rec->dedicated_eu = td->DeMap.eu_main->start_pos;
    rec->dedicated_eu_copy = td->DeMap.eu_backup->start_pos;
    rec->signature = subdev->load_from_prev;
    rec->seq = subdev->super_sequence;
    rec->logic_disk_id = disk_num;
    kunmap(subdev->super_bio->bi_io_vec[0].bv_page);

    // Log for every Super Map
    for (i = 0; i < subdev->Super_num_EUs; i++) {
        LFSM_ASSERT(frontier_local >= 0);
        LFSM_ASSERT(frontier_local <= SUPERMAP_SIZE);
        if (frontier_local == SUPERMAP_SIZE) {
            //tf todo: crash recovery need to handle supermap during erase 2011.12.30
            subdev->super_frontier = 0;

            subdev->cn_super_cycle++;

            if ((ret = EU_erase(subdev->Super_Map[i])) < 0) {
                //erase fail then return false
                LOGWARN("super map erase failure of disk %d\n", disk_num);
                continue;
            }
        }

        subdev->super_bio->bi_iter.bi_size = HARD_PAGE_SIZE;
        subdev->super_bio->bi_next = NULL;
        subdev->super_bio->bi_iter.bi_idx = 0;
        subdev->super_bio->bi_opf = REQ_OP_WRITE;
        //subdev->super_bio->bi_flags = subdev->super_bio_flag;
        subdev->super_bio->bi_status = BLK_STS_OK;
        subdev->super_bio->bi_flags &= ~(1 << BIO_SEG_VALID);
        subdev->super_bio->bi_disk = NULL;  //subdev->bdev;
        // Log to Every Super EU at the same frontier.
        subdev->super_bio->bi_iter.bi_sector = subdev->super_frontier +
            subdev->Super_Map[i]->start_pos;
        LOGINFO("logging supermap at sector %llu\n",
                (sector64) subdev->super_bio->bi_iter.bi_sector);
        subdev->super_flag = 0;
        if (my_make_request(subdev->super_bio) == 0) {
            lfsm_wait_event_interruptible(subdev->super_queue,
                                          subdev->super_flag != 0);
        } else {
            subdev->super_flag = -EIO;
        }
        ret = subdev->super_flag;
        //write fail return fail
    }

    subdev->super_frontier += SECTORS_PER_HARD_PAGE;
    subdev->super_sequence++;

    mutex_unlock(&subdev->supmap_lock);
    return ret;
}

/*
 * Description: Write a new super map record to all disks of group
 *              This should be called every time the dedicated eu changes.
 */

void super_map_logging_append(lfsm_dev_t * td, diskgrp_t * pgrp,
                              int32_t cn_ssd_orig)
{

    int32_t i, ret;

    for (i = cn_ssd_orig; i < td->cn_ssds; i++) {
        ret = super_map_logging(td, i, pgrp->ar_subdev);
        if (ret <= 0) {
            pgrp->ar_subdev[i].load_from_prev = non_ready;
        }
    }
}

void super_map_logging_all(lfsm_dev_t * td, diskgrp_t * pgrp)
{
    int32_t i, ret;

    for (i = 0; i < td->cn_ssds; i++) {
        ret = super_map_logging(td, i, pgrp->ar_subdev);
        if (ret <= 0) {
            pgrp->ar_subdev[i].load_from_prev = non_ready;
        }
    }

    td->super_map_primary_disk = super_map_get_primary(td);
}

/*
 * Description: get logicl disk id from super map on disk
 * return value: < 0 : fail
 *               >= 0 : disk id
 */
int32_t super_map_get_diskid(struct block_device *bdev)
{
    super_map_entry_t *psbmap;
    int8_t *buffer;
    int32_t ret;

    ret = -EIO;

    if (IS_ERR_OR_NULL(buffer = (int8_t *)
                       track_kmalloc(HARD_PAGE_SIZE, GFP_KERNEL, M_OTHER))) {
        LOGERR("alloc page to get disk id from super map fail\n");
        return -ENOMEM;
    }

    if (!devio_raw_rw_page(0, bdev, buffer, 1, REQ_OP_READ)) {
        goto l_free;
    }

    psbmap = (super_map_entry_t *) buffer;
    if (memcmp(psbmap->magicword, TAG_SUPERMAP, sizeof(psbmap->magicword)) == 0) {
        ret = psbmap->logic_disk_id;
    }

  l_free:
    track_kfree(buffer, HARD_PAGE_SIZE, M_OTHER);
    return ret;
}

/*
 * Description: read super map from disk
 * return:  0        : success
 *          otherwise: fail
 */
static int32_t super_map_diskread(lfsm_subdev_t * ar_subdev, sector64 pos,
                                  int32_t disk_num)
{
    lfsm_subdev_t *subdev;
    int32_t ret;

    subdev = &ar_subdev[disk_num];
    subdev->super_bio->bi_iter.bi_size = HARD_PAGE_SIZE;
    subdev->super_bio->bi_next = NULL;
    subdev->super_bio->bi_iter.bi_idx = 0;
    //subdev->super_bio->bi_flags = subdev->super_bio_flag;
    subdev->super_bio->bi_status = BLK_STS_OK;
    subdev->super_bio->bi_flags &= ~(1 << BIO_SEG_VALID);

    subdev->super_bio->bi_opf = REQ_OP_READ;
    subdev->super_bio->bi_iter.bi_sector = pos;
    subdev->super_bio->bi_disk = NULL;  //adjust in my_make_request
    dprintk("super read: bi_sector=%llu, bi_disk=%p\n",
            subdev->super_bio->bi_iter.bi_sector, subdev->super_bio->bi_disk);

    subdev->super_flag = 0;
    if (my_make_request(subdev->super_bio) == 0) {
        lfsm_wait_event_interruptible(subdev->super_queue,
                                      subdev->super_flag != 0);
    } else {
        return -EIO;
    }

    if (subdev->super_flag == 1) {
        ret = 0;
    } else {
        ret = subdev->super_flag;   //error
    }

    return ret;
}

/*
 * Description: get the EU of dedicated map on disk by reading super map
 * return:  0        : success
 *          otherwise: fail
 */
int32_t super_map_set_dedicated_map(lfsm_dev_t * td, lfsm_subdev_t * ar_subdev)
{
    lfsm_subdev_t *subdev;
    super_map_entry_t *rec;
    sector64 dedicated_eu_main_pos, dedicated_eu_backup_pos;
    int8_t *addr;
    int32_t ret;

    subdev = &ar_subdev[td->super_map_primary_disk];
    LOGINFO(" set dediceated map in super map start\n");
    if ((ret = super_map_diskread(ar_subdev,
                                  subdev->Super_Map[0]->start_pos +
                                  subdev->super_frontier -
                                  SECTORS_PER_HARD_PAGE,
                                  td->super_map_primary_disk)) < 0) {
        return ret;
    }

    addr = (int8_t *) kmap(subdev->super_bio->bi_io_vec[0].bv_page) +
        subdev->super_bio->bi_io_vec[0].bv_offset;
    if (NULL == addr) {
        LOGERR(" Allocating fail\n");
        return -ENOMEM;
    }

    rec = (super_map_entry_t *) addr;
    LOGINFO("record on sb is {ddmap pos: %llu, seq: %llu, sign: %x}\n",
            rec->dedicated_eu, rec->seq, rec->signature);
    dedicated_eu_main_pos = rec->dedicated_eu;
    dedicated_eu_backup_pos = rec->dedicated_eu_copy;
    kunmap(subdev->super_bio->bi_io_vec[0].bv_page);

    LOGINFO("Got dedicated main and backup EUs at (%llus, %llus)\n",
            dedicated_eu_main_pos, dedicated_eu_backup_pos);
    //AN:2012/10/2  there is a case : the disk is specified as good disk when read supermap
    // but the dedicate EU is mark as bad block in bootBBM, if use "FreeList_delete_by_pos"
    // the return value is NULL, the system is halt. so we use eu_find, and call FreeList_delete_by_pos later.
    //td->DeMap.eu_main = FreeList_delete_by_pos(td,dedicated_eu_main_pos);
    td->DeMap.eu_main = eu_find(dedicated_eu_main_pos);
    if (NULL == td->DeMap.eu_main) {
        LOGERR("td->DeMap.eu_main < 0\n");
        return -ENOMEM;
    }
    //td->DeMap.eu_backup = FreeList_delete_by_pos(td,dedicated_eu_backup_pos);
    td->DeMap.eu_backup = eu_find(dedicated_eu_backup_pos);
    if (NULL == td->DeMap.eu_backup) {
        LOGERR("td->DeMap.eu_backup<0\n");
        return -ENOMEM;
    }

    return ret;
}

/**
    returns the frontier and writes max entry into store_rec.
**/

static sector64 find_max_seq_entry_A(lfsm_subdev_t * ar_subdev,
                                     super_map_entry_t * store_rec,
                                     int32_t disk_num, int32_t id_supermap)
{
    lfsm_subdev_t *subdev;
    super_map_entry_t *rec;
    sector64 frontier;
    int8_t *addr;
    int32_t i, frontier_start, num_of_sectors_in_supermap;

    subdev = &ar_subdev[disk_num];
    store_rec->signature = non_ready;
    store_rec->seq = 0;
    frontier = 0;
    num_of_sectors_in_supermap = SUPERMAP_SIZE;

    frontier_start = 0;

    for (i = frontier_start; i < num_of_sectors_in_supermap;
         i += SECTORS_PER_HARD_PAGE) {
        if (super_map_diskread
            (ar_subdev, subdev->Super_Map[id_supermap]->start_pos + i,
             disk_num) != 0) {
            store_rec->signature = non_ready;
            return -1;
        }

        addr = (int8_t *) kmap(subdev->super_bio->bi_io_vec[0].bv_page)
            + subdev->super_bio->bi_io_vec[0].bv_offset;
        LFSM_ASSERT(addr != NULL);
        rec = (super_map_entry_t *) addr;
        LOGINFO
            ("supermap %dth sector: {ddmap pos:0x%llx,seq: 0x%llx,sign: %x}\n",
             i, rec->dedicated_eu, rec->seq, rec->signature);
        if ((rec->seq > store_rec->seq) && ((rec->signature == all_ready) || (rec->signature == sys_ready))) {  //==LFSM_USED_DISK))
            memcpy(store_rec, rec, sizeof(super_map_entry_t));
            frontier += SECTORS_PER_HARD_PAGE;
            LFSM_ASSERT(!(frontier % SECTORS_PER_HARD_PAGE));
            kunmap(subdev->super_bio->bi_io_vec[0].bv_page);
        } else {
            frontier = i;       // AN :frontier sometimes may not start from 0
            kunmap(subdev->super_bio->bi_io_vec[0].bv_page);
            break;
        }
    }

    return frontier;
}

/*
 * Description: find out the last entry (valid one) in super map
 * Parameter:
 *            disk_num: disk num in disk group
 */
sector64 find_max_seq_entry(diskgrp_t * pgrp, super_map_entry_t * store_rec,
                            int32_t disk_num)
{
    struct EUProperty *tmp;
    sector64 ret;
    ret = find_max_seq_entry_A(pgrp->ar_subdev, store_rec, disk_num, 0);

    if (pgrp->cn_max_drives == 1 && ret == -1) {
        ret = find_max_seq_entry_A(pgrp->ar_subdev, store_rec, disk_num, 1);
        tmp = pgrp->ar_subdev[disk_num].Super_Map[0];
        pgrp->ar_subdev[disk_num].Super_Map[0] =
            pgrp->ar_subdev[disk_num].Super_Map[1];
        pgrp->ar_subdev[disk_num].Super_Map[1] = tmp;
    }
    return ret;
}

/*
 * Description: find out the last entry (valid one) in disk group of disk
 * Parameter: disk_num: disk num in disk group
 */
int32_t super_map_get_last_entry(int32_t disk_num, diskgrp_t * pgrp)
{
    super_map_entry_t rec;
    int32_t ret;

    memset(&rec, 0, sizeof(super_map_entry_t));
    ret = find_max_seq_entry(pgrp, &rec, disk_num);
    if (ret < 0) {
        return -EINVAL;
    }

    pgrp->ar_subdev[disk_num].super_frontier = ret;
    pgrp->ar_subdev[disk_num].super_sequence = rec.seq + 1;
    pgrp->ar_subdev[disk_num].load_from_prev = rec.signature;
    if (lfsm_resetData == 2) {
        pgrp->ar_subdev[disk_num].load_from_prev = non_ready;
    }

    /*
     * TODO: JIRA: FSS-58
     * rescue the bad super by erase,
     * if ok => copy other good super in same frontier,
     * if fail => mark as bad
     */
    return ret;
}

/*
 *setting the eu pointer to each super map
*/
static int32_t super_map_initEU_A(lfsm_subdev_t * ar_subdev, struct HListGC *h,
                                  sector64 log_start, int32_t offset,
                                  int32_t disk_num)
{
    EUProperty_t *e;
    int32_t i;

    for (i = 0; i < SUPERMAP_SEU_NUM; i++) {    //tf_super
        e = pbno_eu_map[log_start + i];
        // Add super map entries to an array.
        ar_subdev[disk_num].Super_Map[offset + i] = e;
        FreeList_delete_by_pos_A(e->start_pos, EU_SUPERMAP);
        LOGINFO("SB init for disk %d : starting at abs EU%d (%llus)\n",
                disk_num, (int32_t) log_start + i, e->start_pos);
    }
    return 0;
}

/*
 * Description: init EU of super map
 * Parameter: log_start: starting address of super map EU, in global addressing space
 *                disk 0 start : 0
 *                disk n start : disk (n-1) start + disk (n-1) size
 *                unit : EU
 *            free_number: free number of EU of disk unit: EU
 *            disk_num: disk num in disk group
 */
void super_map_initEU(diskgrp_t * pgrp, struct HListGC *h,
                      sector64 log_start, int32_t free_number, int32_t disk_num)
{
    super_map_initEU_A(pgrp->ar_subdev, h, log_start, 0, disk_num);

    if (pgrp->cn_max_drives == 1) {
        super_map_initEU_A(pgrp->ar_subdev, h,
                           log_start + free_number - SUPERMAP_SEU_NUM,
                           SUPERMAP_SEU_NUM, disk_num);
    }
}

/*
 * Description: free super map eu in all disks
 */
void super_map_freeEU(diskgrp_t * pgrp)
{
    lfsm_subdev_t *dev;
    int32_t i;

    for (i = 0; i < pgrp->cn_max_drives; i++) {
        dev = &pgrp->ar_subdev[i];

        if (NULL == dev->Super_Map) {
            continue;
        }

        track_kfree(dev->Super_Map, dev->Super_num_EUs * sizeof(EUProperty_t *),
                    M_SUPERMAP);
        dev->Super_Map = 0;
    }
}

/*
* *   Author: tifa
**    If number of disks = 1, then log the super map at both start and end of disk.
**    Else log the super map at the start of each disk.
**
**    return -ENOMEM is alloc fail, return 0 is alloc success
*/
int32_t super_map_allocEU(diskgrp_t * pgrp, int32_t idx_disk)
{
    lfsm_subdev_t *dev;
    int32_t i;

    for (i = idx_disk; i < pgrp->cn_max_drives; i++) {
        dev = &pgrp->ar_subdev[i];
        dev->Super_num_EUs = SUPERMAP_SEU_NUM;

        if (pgrp->cn_max_drives == 1) {
            dev->Super_num_EUs *= 2;
        }
        // Create an array holding the start sector of Supermap on every disk.
        dev->Super_Map = (EUProperty_t **)
            track_kmalloc(dev->Super_num_EUs * sizeof(EUProperty_t *),
                          GFP_KERNEL, M_SUPERMAP);

        if (NULL == dev->Super_Map) {
            LOGWARN("fail alloc mem for super map EU of disk %d\n", i);
            goto l_fail;
        }
    }
    return 0;

  l_fail:
    super_map_freeEU(pgrp);
    return -ENOMEM;
}

/*
 * Description: get the first healthy ssd and mark as primary disk
 * return: disk number of first healthy one in group
 */
int32_t super_map_get_primary(lfsm_dev_t * td)
{
    int32_t i, ret;

    ret = 0;
    for (i = 0; i < td->cn_ssds; i++) {
        if (DEV_SYS_READY(grp_ssds, i)) {
            ret = i;
            break;
        }
    }

    if (i == td->cn_ssds) {
//        if(td->subdev[0].load_from_prev==non_ready)
        LOGINFO("All super map are non_ready, use disk 0 as primary.\n");
        return 0;
    }

    LOGINFO("Super map primary disk id %d\n", ret);
    return ret;
}

/*
 * Description: rescue super map on each disk
 * Parameter: pgrp : disk group
 * return: false: init success
 *         true : init fail
 */
bool super_map_rescue(lfsm_dev_t * td, diskgrp_t * pgrp)
{
    int32_t i;
    bool ret;

    ret = false;
    if (lfsm_readonly == 1) {
        return ret;
    }

    for (i = 0; i < td->cn_ssds; i++) {
        if (DEV_SYS_READY(grp_ssds, i) || pgrp->ar_drives[i].actived == 0) {
            continue;
        }

        mutex_lock(&pgrp->ar_subdev[i].supmap_lock);
        if (EU_erase(pgrp->ar_subdev[i].Super_Map[0]) < 0) {
            ret = true;
            LOGWARN("super map erase failure of disk %d\n", i);
            mutex_unlock(&pgrp->ar_subdev[i].supmap_lock);
            break;
        }

        pgrp->ar_subdev[i].super_frontier = 0;
        pgrp->ar_subdev[i].load_from_prev = sys_ready;
        mutex_unlock(&pgrp->ar_subdev[i].supmap_lock);

        super_map_logging(td, i, pgrp->ar_subdev);
    }

    return ret;
}

/*
 * Description: dump super map information to OS system log
 * Parameter: disk_num : disk number of disk pool
 * return: false: init success
 *         true : init fail
 */
void supermap_dump(lfsm_subdev_t * ar_subdev, int32_t disk_num)
{
    lfsm_subdev_t *subdev;
    super_map_entry_t *rec;
    int8_t *addr;
    int32_t i, sects_smap;

    sects_smap = SUPERMAP_SIZE;
    rec = NULL;
    subdev = &ar_subdev[disk_num];

    for (i = 0; i < sects_smap; i += SECTORS_PER_HARD_PAGE) {
        if (super_map_diskread(ar_subdev,
                               ar_subdev[disk_num].Super_Map[0]->start_pos + i,
                               disk_num) != 0) {
            LOGERR("fail read super map from disk %d\n", disk_num);
            break;
        }

        addr = (int8_t *) kmap(subdev->super_bio->bi_io_vec[0].bv_page) +
            subdev->super_bio->bi_io_vec[0].bv_offset;
        LFSM_ASSERT(addr != NULL);
        rec = (super_map_entry_t *) addr;
        LOGINFO("supermap diskid %d sector %d has {%llu, %llu, %x}\n",
                disk_num, i, rec->dedicated_eu, rec->seq, rec->signature);
        if (rec->seq != ((i << SECTORS_PER_HARD_PAGE_ORDER) + 1)) {
            break;
        }
    }
}

/*
 * Description: initial in memory data structure of super map in lfsm_subdev_t on disk [disk_num]
 * Parameter: disk_num : disk number of disk pool
 * return: false: init success
 *         true : init fail
 */
bool super_map_init(lfsm_dev_t * td, int32_t disk_num,
                    lfsm_subdev_t * ar_subdev)
{
    if (compose_bio
        (&ar_subdev[disk_num].super_bio, NULL, end_bio_super_sector, td,
         HARD_PAGE_SIZE, MEM_PER_HARD_PAGE) < 0) {
        LOGERR("fail init super map: compose bio fail\n");
        return true;
    }
    //ar_subdev[i].super_bio_flag = ar_subdev[i].super_bio->bi_flags;
    init_waitqueue_head(&ar_subdev[disk_num].super_queue);
    ar_subdev[disk_num].super_flag = 0;
    mutex_init(&ar_subdev[disk_num].supmap_lock);
    ar_subdev[disk_num].cn_super_cycle = 0;
    ar_subdev[disk_num].super_frontier = 0;

    return false;
}

/*
 * Description: free data structure of super map in lfsm_subdev_t on disk [disk_num]
 *               (in memory representation)
 * Parameter: disk_num : disk number of disk pool
 * return: false: init success
 *         true : init fail
 */
void super_map_destroy(lfsm_dev_t * td, int32_t disk_num,
                       lfsm_subdev_t * ar_subdev)
{
    if (!ar_subdev) {
        return;
    }

    if (IS_ERR_OR_NULL(ar_subdev[disk_num].super_bio)) {
        return;
    }

    free_composed_bio(ar_subdev[disk_num].super_bio, MEM_PER_HARD_PAGE);
    ar_subdev[disk_num].super_bio = NULL;
    mutex_destroy(&ar_subdev[disk_num].supmap_lock);
}
