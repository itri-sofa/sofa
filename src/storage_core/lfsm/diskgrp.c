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
#include <linux/fs.h>
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
#include "io_manager/io_common.h"
#include "system_metadata/Dedicated_map.h"
#include "sysapi_linux_kernel.h"
#include "system_metadata/Super_map.h"
#include "diskgrp_private.h"
#include "common/tools.h"

int32_t diskgrp_get_erase_upbound(sector64 lsn)
{
#if 0                           // LFSM_ENABLE_TRIM == 2
// avoid calculate lsn to device twice, this code is disable now; this section didn't test
    struct block_device *bdev;
    int32_t id_dev;

    if (!pbno_lfsm2bdev(lsn, NULL, &id_dev)) {
        return -EINVAL;
    }

    if (IS_ERR_OR_NULL(bdev = diskgrp_get_drive(&grp_ssds, id_dev))) {  //pgrp->ar_drives[dev_id].bdev;
        return -ENODEV;
    }

    if (bdev_get_queue(bdev)->limits.max_discard_sectors == 0) {
        return -EPERM;
    }

    return min(bdev_get_queue(bdev)->limits.max_discard_sectors,
               UINT_MAX >> SECTOR_ORDER);
#else
    return (UINT_MAX >> SECTOR_ORDER);
#endif
}

/*
 * Description: check whether disk [id_disk] is fail disk
 * return: true : yes, fail disk
 *         false: no, healthy disk
 */
bool diskgrp_isfaildrive(diskgrp_t * pgrp, int32_t id_disk)
{
    if ((atomic_read(&pgrp->ar_drives[id_disk].cn_err_read) >= LFSM_MAX_RETRY)
        || (pgrp->ar_drives[id_disk].actived == 0)) {
        return true;
    } else {
        return false;
    }
}

/*
 * Description: check the disk number of pbno belong to is fail disk
 */
bool diskgrp_isfaildrive_by_pbno(sector64 pbno)
{
    int32_t id_dev;

    if (!pbno_lfsm2bdev_A(pbno, 0, &id_dev, &grp_ssds)) {
        return true;
    }

    return diskgrp_isfaildrive(&grp_ssds, id_dev);
}

// the function is obselated. you should not use it in the future.
struct block_device *diskgrp_getbdev(diskgrp_t * pgrp, int32_t id_dev)
{
    if (id_dev > pgrp->cn_drives) {
        return NULL;
    }

    return pgrp->ar_drives[id_dev].bdev;
}

/*
 * Description: erase the device name when rebuild fail
 * Parameter: id_failslot: the fail disk in raid group
 */
int32_t diskgrp_detach(diskgrp_t * pgrp, int32_t id_failslot)
{
    int32_t id_org_disk;

    id_org_disk = sysapi_bdev_name2id(pgrp->ar_drives[id_failslot].nm_dev);
    if (id_org_disk < 0) {
        LOGERR("diskgrp_detach err fail find disk id for %s\n",
               pgrp->ar_drives[id_failslot].nm_dev);
    } else {
        memset(pgrp->ar_drives[id_failslot].nm_dev, 0, MAXLEN_DEV_NAME);
    }

    return id_org_disk;
}

/*
 * Description: check whether id_disk is validate one,
 *              if yes, change to slot to new disk
 *              if no, warning.
 * Parameter: id_disk : new disk id to rebuild
 *            idx_failslot: fail disk
 *
 */
void diskgrp_attach(diskgrp_t * pgrp, int32_t idx_failslot, int32_t id_disk)
{
    if (IS_ERR_OR_NULL
        (sysapi_bdev_id2name(id_disk, pgrp->ar_drives[idx_failslot].nm_dev))) {
        LOGWARN("Invalid id_disk %d \n", id_disk);
    }

    return;
}

/*
 * Description: reset error count
 */
void diskgrp_cleanlog(diskgrp_t * pgrp, int32_t id_disk)
{
    atomic_set(&pgrp->ar_drives[id_disk].cn_err_read, 0);
}

/*
 * Description: increase error count when access disk fail
 */
bool diskgrp_logerr(diskgrp_t * pgrp, int32_t id_disk)
{
    atomic_inc(&pgrp->ar_drives[id_disk].cn_err_read);

    if (diskgrp_isfaildrive(pgrp, id_disk)) {
        diskgrp_switch(pgrp, id_disk, 0);
    }

    return true;
}

static struct block_device *diskgrp_reopen_bdev(diskgrp_t * pgrp,
                                                int8_t * nm_dev)
{
    struct block_device *bdev;

    if (IS_ERR_OR_NULL(bdev = sysapi_bdev_open(nm_dev))) {
        LOGERR("cannot_open %s %p \n", nm_dev, bdev);
        return NULL;
    }

    return bdev;
}

/*
 * Description: get capacity from disk and align with size of EU
 * Parameter: assign_sz_in_sector (works for test)
 * return : # of disk size in SEU
 */
// if assign_sz_in_eu==0, then get from system and cut 1 seu
static sector64 _get_align_capacity(struct block_device *bdev,
                                    sector64 assign_sz_in_sector)
{
    sector64 sz;

    sz = (assign_sz_in_sector >
          0) ? assign_sz_in_sector : (sector64) get_capacity(bdev->bd_disk);
    sz = LFSM_FLOOR(sz, SECTORS_PER_SEU);
    if (assign_sz_in_sector == 0) { // Prevent a bug in FW //TODO: Lego still happens in firmware free version?
        sz -= SECTORS_PER_SEU;
    }

    return sz;
}

static int32_t diskgrp_reopen(diskgrp_t * pgrp, int32_t id_dev)
{
    struct block_device *bdev;
    subdisk_info_t *drive;
    sector64 sz;

    drive = &pgrp->ar_drives[id_dev];
    if (!IS_ERR_OR_NULL(drive->bdev)) {
        return -EAGAIN;
    }

    bdev = diskgrp_reopen_bdev(pgrp, drive->nm_dev);
    if (IS_ERR_OR_NULL(bdev)) {
        drive->cn_open_fail++;
        if (is2order(drive->cn_open_fail)) {
            LOGERR("fail to open disk %s to active due to open failure (%ld) "
                   "retry=%d\n", drive->nm_dev, PTR_ERR(bdev),
                   drive->cn_open_fail);
        }
        return -ENODEV;
    }

    // AN : after reopen, the supermap table shall be clean
    if (devio_erase(0, NUM_EUS_PER_SUPER_EU, bdev) < 0) {
        LOGERR
            ("Cannot not set disk %s to active due to cannot clean supermap area\n",
             drive->nm_dev);
        sysapi_bdev_close(bdev);
        return -EIO;
    }

    sz = _get_align_capacity(bdev,
                             SECTORS_PER_FLASH_PAGE * FPAGE_PER_SEU *
                             SSD_SIZE_IN_SEU);

    if (sz < drive->disk_size) {
        LOGERR("fail to switch disk %s to active. "
               "smaller capacity of new disk %lld<%lld\n", drive->nm_dev, sz,
               drive->disk_size);
        //close_bdev_exclusive(bdev,O_RDWR);
        sysapi_bdev_close(bdev);
        return -EPERM;
    } else if (sz > drive->disk_size) {
        LOGWARN("larger capacity of new disk %s %lld>%lld\n", drive->nm_dev, sz,
                drive->disk_size);
    }

    LOGINFO("Success set disk %s to active, but not recovered\n",
            drive->nm_dev);
    drive->bdev = bdev;
    drive->cn_open_fail = 0;
    atomic_set(&drive->cn_err_read, 0);
    drive->actived = 1;
    return 0;
}

static void diskgrp_close_bdev(struct block_device *bdev, int32_t id_disk,
                               int8_t * nm_dev)
{
    //close_bdev_exclusive(bdev, O_RDWR);
    sysapi_bdev_close(bdev);
    LOGINFO("Close disk %d (%s) done\n", id_disk, nm_dev);

    metalog_packet_cur_eus_all(&lfsmdev.mlogger, gc);

    wake_up_interruptible(&lfsmdev.hotswap.wq);
}

/*
 * Description: if bswitch != 0, try open again
 *              if bswitch 0, close device
 */
void diskgrp_switch(diskgrp_t * pgrp, int32_t id_disk, bool bswitch)
{
    struct block_device *bdev;
    unsigned long flag;
    bool to_log;

//    int old_state = pgrp->ar_drives[id_disk].actived;

    bdev = NULL;
    to_log = false;
    if (bswitch == 0) {
//        if (cmpxchg(&pgrp->ar_drives[id_disk].actived,old_state,0)==1) //just switch
        spin_lock_irqsave(&pgrp->ar_drives[id_disk].lock, flag);
        if (pgrp->ar_drives[id_disk].actived) {
            to_log = true;
        }

        pgrp->ar_drives[id_disk].actived = 0;

        if (pgrp->ar_drives[id_disk].cn_bio == 0) {
            bdev = pgrp->ar_drives[id_disk].bdev;
            pgrp->ar_drives[id_disk].bdev = NULL;
        }
        spin_unlock_irqrestore(&pgrp->ar_drives[id_disk].lock, flag);

        if (to_log) {
            pgrp->ar_subdev[id_disk].load_from_prev = non_ready;    //LFSM_BAD_DISK;

            if (HPEU_set_allstate(&lfsmdev.hpeutab, id_disk, damege) < 0) {
                LOGWARN("fail to change all hpeus of disk %d to damege\n",
                        id_disk);
            }
            lfsmdev.ar_pgroup[PGROUP_ID(id_disk)].state = epg_damege;

            /* NOTE: why log super map as sys_loadded, it seem meanless */
            dedicated_map_logging(&lfsmdev);    //log_super
            LOGINFO("Disable disk %d (%s) done, but close later\n", id_disk,
                    pgrp->ar_drives[id_disk].nm_dev);
        }

        if (bdev) {
            diskgrp_close_bdev(bdev, id_disk, pgrp->ar_drives[id_disk].nm_dev);
        }
    } else {
        diskgrp_reopen(pgrp, id_disk);
    }
}

/*
 * Description: reference a disk by close it
 * TODO: Don't mess up NULL with ERR_PTR since not all caller check with IS_ERR_OR_NULL
 */
static struct block_device *diskgrp_get_drive_A(subdisk_info_t * psdi)
{
    struct block_device *bdev;
    unsigned long flag;

    spin_lock_irqsave(&psdi->lock, flag);

    if (psdi->actived == 0) {
        bdev = NULL;
    } else {
        psdi->cn_bio++;
        LFSM_ASSERT(psdi->bdev != NULL);
        bdev = psdi->bdev;
    }
    spin_unlock_irqrestore(&psdi->lock, flag);
    return bdev;
}

/*
 * Description: API for reference a disk
 */
struct block_device *diskgrp_get_drive(diskgrp_t * pgrp, int32_t id_dev)
{
    return diskgrp_get_drive_A(&pgrp->ar_drives[id_dev]);
}

/*
 * Description: deference a disk by close it
 */
static void diskgrp_put_drive_A(subdisk_info_t * psdi)
{
    struct block_device *bdev;
    unsigned long flag;

    bdev = NULL;
    spin_lock_irqsave(&psdi->lock, flag);
    LFSM_ASSERT(psdi->cn_bio > 0);
    psdi->cn_bio--;
    if ((psdi->cn_bio == 0) && (psdi->actived == 0)) {
        bdev = psdi->bdev;
        psdi->bdev = NULL;
    }
    spin_unlock_irqrestore(&psdi->lock, flag);

    if (bdev) {
        diskgrp_close_bdev(bdev, (psdi - (grp_ssds.ar_drives)), psdi->nm_dev);
    }
    return;
}

/*
 * Description: API for deference a disk
 */
void diskgrp_put_drive(diskgrp_t * pgrp, int32_t id_dev)
{
    diskgrp_put_drive_A(&pgrp->ar_drives[id_dev]);
    return;
}

void diskgrp_calc_capacity(diskgrp_t * pgrp)
{
    int i;
    for (i = 0; i < pgrp->cn_max_drives; i++) {
        pgrp->ar_drives[i].disk_size = pgrp->cn_sectors_per_dev;
    }
    pgrp->capacity = pgrp->cn_sectors_per_dev * pgrp->cn_max_drives;
}

/*
 * Description: check whether all working disks be open success,
 *              if only 1 disk fail and has healthy disk in spare,
 *              replace with healthy
 * return : 0 : ok
 *         non 0: has fail
 * TODO JIRA: FSS-65
 */
int32_t diskgrp_check_and_complete_disknum(diskgrp_t * pgrp)
{
    //TODO : max -> expected .. weafon
    int32_t id_disk;

    /* all disk be open successfully */
    if (pgrp->cn_drives == pgrp->cn_max_drives) {
        goto _calc_capacity;
    }

    if (pgrp->cn_drives >= pgrp->cn_max_drives - 1) {   //
        LOGWARN("Current pgrp disk num %d is smaller than %d max disk num\n",
                pgrp->cn_drives, pgrp->cn_max_drives);

        LOGINFO("Try to pickup one disk from spare DISK and open\n");
        while ((id_disk = hotswap_dequeue_disk(&lfsmdev.hotswap, -1)) >= 0) {
            diskgrp_open_dev(&lfsmdev, id_disk, true);
            if (pgrp->cn_drives == pgrp->cn_max_drives) {
                goto _calc_capacity;
            }
        }

        LOGERR("CANNOT get disk from spare disk\n");
    }
    LOGERR("Current pgrp disk num %d is smaller than %d max disk num\n",
           pgrp->cn_drives, pgrp->cn_max_drives);
    return -EINVAL;
  _calc_capacity:
    diskgrp_calc_capacity(pgrp);
    return 0;
}

/*
 * Description:
 *     add disk to specific slot (id_disk), copy ssd's io scheduler for future restore
 *     change io scheduler.
 *     initialize ar_drives of diskgroup
 *
 * return : 0 : add ok
 *          non 0 : add fail
 */
static int32_t diskgrp_assign_drive(diskgrp_t * pgrp, int8_t * nm_dev,
                                    struct block_device *bdev,
                                    sector64 sz_disks, int32_t id_disk)
{
    if (id_disk >= pgrp->cn_max_drives || id_disk < 0) {
        LOGINFO("Fail to add %s to diskgrp because the pgrp cn_max is %d, "
                "id_disk is %d\n", nm_dev, pgrp->cn_max_drives, id_disk);
        return -ENODEV;
    }

    if (!IS_ERR_OR_NULL(pgrp->ar_drives[id_disk].bdev)) {
        LOGERR("Fail to add %s Because %dth logic disk is not null, "
               "lfsm abort\n", nm_dev, id_disk);
        return -EEXIST;
    }

    if (IS_ERR_OR_NULL(nm_dev)) {
        LOGERR("assign driver fail null device name\n");
        return -ENODEV;
    }

    pgrp->ar_drives[id_disk].bdev = bdev;
    pgrp->ar_drives[id_disk].disk_size = _get_align_capacity(bdev, sz_disks);
    pgrp->ar_drives[id_disk].disk_size =
        LFSM_FLOOR(pgrp->ar_drives[id_disk].disk_size, SECTORS_PER_SEU);

    pgrp->ar_drives[id_disk].actived = 1;
    pgrp->ar_drives[id_disk].cn_bio = 0;
    pgrp->ar_drives[id_disk].cn_open_fail = 0;

    spin_lock_init(&pgrp->ar_drives[id_disk].lock);
    strncpy(pgrp->ar_drives[id_disk].nm_dev, nm_dev, 16);
    atomic_set(&pgrp->ar_drives[id_disk].cn_err_read, 0);
    /*
     * TODO: FSS-65 if different size of each disk,
     * the cn_sectors_per_dev might be the value with biggest disk
     */
    if (pgrp->cn_sectors_per_dev > pgrp->ar_drives[id_disk].disk_size
        || pgrp->cn_sectors_per_dev == 0) {
        pgrp->cn_sectors_per_dev = pgrp->ar_drives[id_disk].disk_size;
    }
    //pgrp->capacity += pgrp->ar_drives[id_disk].disk_size;

    pgrp->cn_drives++;
    LOGINFO("Success to add %s %dth logic disk\n", nm_dev, id_disk);
    return 0;
}

/*
 * Description: find a free slot and call diskgrp_assign_drive to put dev into slow
 * Parameter: nm_dev : disk name
 *            bdev : linux block device
 *            sz_disks : disk size in sector (should 0)
 * return : 0 : add ok
 *          non 0 : add fail
 */
static int32_t diskgrp_add_drive(diskgrp_t * pgrp, int8_t * nm_dev,
                                 struct block_device *bdev, sector64 sz_disks)
{
    int32_t id_slot, i, ret;

    ret = -ENOSPC;

    for (i = 0; i < pgrp->cn_max_drives; i++) {
        //NOTE: cn_drives be initialized as 0
        id_slot = (i + pgrp->cn_drives) % pgrp->cn_max_drives;

        if (IS_ERR_OR_NULL(pgrp->ar_drives[id_slot].bdev)) {
            ret = diskgrp_assign_drive(pgrp, nm_dev, bdev, sz_disks, id_slot);
            break;
        }
    }

    return ret;
}

/*
 * Description: Add disk to ar_drives of disk group
 */
static int32_t diskgrp_add_ssd(int8_t * nm_dev, struct block_device *bdev,
                               bool assign_empty_slot)
{
    int32_t id_disk_logic;

    id_disk_logic = -1;
    if (lfsm_resetData != 0 || assign_empty_slot) {
        LOGINFO("clean super map area, disk %s\n", nm_dev);
        //NOTE: Why erase 64 EUs
        devio_erase(0, NUM_EUS_PER_SUPER_EU, bdev);
        //NOTE: SSD_SIZE_IN_SEU defined in makefile, default is 0
        return (diskgrp_add_drive(&grp_ssds, nm_dev, bdev,
                                  (sector64) SECTORS_PER_FLASH_PAGE *
                                  FPAGE_PER_SEU * SSD_SIZE_IN_SEU));
    }

    // must assign the disk supermap indicate
    if ((id_disk_logic = super_map_get_diskid(bdev)) < 0) { // cannot find
        return -EFAULT;
    }

    return (diskgrp_assign_drive(&grp_ssds, nm_dev, bdev,
                                 (sector64) SECTORS_PER_FLASH_PAGE *
                                 FPAGE_PER_SEU * SSD_SIZE_IN_SEU,
                                 id_disk_logic));
}

/*
 * Description: open device
 * Parameters: id_disk: disk id to open
 *             assign_empty_slot : assign to an empty slot when true
 * return: 0 : OK or open fail, should retry
 *         non 0 : fail
 */
int32_t diskgrp_open_dev(lfsm_dev_t * td, int32_t id_disk,
                         bool assign_empty_slot)
{
    int8_t nm_disk[MAXLEN_DEV_NAME];
    struct block_device *bdev;
    int32_t ret;

    sysapi_bdev_id2name(id_disk, nm_disk);
    LFSM_ASSERT(strstr(nm_disk, lfsmCfg.osdisk_name) == 0);

    ret = 0;
    bdev = NULL;
    if (IS_ERR_OR_NULL(bdev = sysapi_bdev_open(nm_disk))) {
        LOGINFO("Fail to open disk %s to active due to open failure (%ld)\n",
                nm_disk, PTR_ERR(bdev));
        return 0;
    }

    if ((ret = diskgrp_add_ssd(nm_disk, bdev, assign_empty_slot)) < 0) {
        if (ret == -EEXIST) {   // the supermap disk id is occupied by a disk, lfsm abort
            sysapi_bdev_close(bdev);
            LOGERR("the same disk id, lfsm abort\n");
            return ret;
        }

        /*
         * Assign fail, a spare disk and put to hotswap
         */
        devio_erase(0, NUM_EUS_PER_SUPER_EU, bdev); // erase before send to hotswap_spare_disk
        sysapi_bdev_close(bdev);
        hotswap_euqueue_disk(&td->hotswap, id_disk, 1); // log err inside hotswap_enqueue
    }

    return 0;
}

/*
 * Description: initial disk group data structure
 * Parameter: pgrp : disk group data structure in system
 *            max_drives: max number of disks
 *            phymedia: type of disk : SSD or HD (Not support HD now)
 *            cn_pgroup : # of protection group
 *            nm_io_scheduler: scheduler of disk
 */
void diskgrp_init(diskgrp_t * pgrp, int32_t max_drives, int32_t phymedia,
                  int32_t cn_pgroup, int8_t * nm_io_scheduler)
{
    pgrp->ar_drives = (subdisk_info_t *)
        track_kmalloc(max_drives * sizeof(subdisk_info_t),
                      GFP_KERNEL | __GFP_ZERO, M_DISKGRP);
    pgrp->ar_subdev = (lfsm_subdev_t *)
        track_kmalloc(max_drives * sizeof(lfsm_subdev_t),
                      GFP_KERNEL | __GFP_ZERO, M_DISKGRP);
    strcpy(pgrp->nm_io_scheduler, nm_io_scheduler);
    pgrp->cn_max_drives = max_drives;
    pgrp->cn_drives = 0;
    pgrp->cn_sectors_per_dev = 0;
    pgrp->capacity = 0;
    pgrp->phymedia = phymedia;
    pgrp->ar_cn_selected = (atomic_t *)
        track_kmalloc(cn_pgroup * sizeof(atomic_t), GFP_KERNEL, M_DISKGRP); //ding: unsuitable location
}

/*
 * Description: free all data structure and change disk's io scheduler to original one
 */
void diskgrp_destory(diskgrp_t * pgrp)
{
    int32_t i;

    if (pgrp->ar_drives == 0) {
        return;
    }

    for (i = 0; i < pgrp->cn_drives; i++) {
        if (!IS_ERR_OR_NULL(pgrp->ar_drives[i].bdev)) {
            //close_bdev_exclusive(pgrp->ar_drives[i].bdev,O_RDWR);
            sysapi_bdev_close(pgrp->ar_drives[i].bdev);
        }
    }
    track_kfree(pgrp->ar_drives, pgrp->cn_max_drives * sizeof(subdisk_info_t),
                M_DISKGRP);
    track_kfree(pgrp->ar_subdev, pgrp->cn_max_drives * sizeof(lfsm_subdev_t),
                M_DISKGRP);
    track_kfree(pgrp->ar_cn_selected, lfsmdev.cn_pgroup * sizeof(atomic_t),
                M_DISKGRP);
}
