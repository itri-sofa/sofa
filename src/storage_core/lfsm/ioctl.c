/*
 * SPDX-License-Identifier: GPL-2.0-or-later OR Apache-2.0
 * Copyright (C) 2015-2020 Industrial Technology Research Institute
 *
 *
 *
 *  
 */
#include <linux/fs.h>
#include <linux/math64.h>
#include <linux/types.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/bio.h>
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
#include "ioctl.h"
#include "bmt.h"
#include "bmt_commit.h"
#include "io_manager/io_common.h"
#include "GC.h"
#include "special_cmd.h"
#include "err_manager.h"
#include "lfsm_test.h"
#include "bmt_ppdm.h"
#include "dbmtapool.h"
#include "autoflush.h"
#include "system_metadata/Super_map.h"
#include "erase_count.h"
#include "system_metadata/Dedicated_map.h"
#include "bmt_ppdm.h"
#include "hpeu.h"
#include "hpeu_gc.h"
#include "ecc.h"
#include "EU_create.h"
#include "EU_access.h"
#include "io_manager/io_write.h"
#include "conflict.h"
#include "sysapi_linux_kernel.h"
#include "spare_bmt_ul.h"
#include "bio_ctrl.h"
#include "hook.h"

int32_t finished;
int32_t pid_ioctl;
int32_t fg_limit_iops = 0;
int32_t gmax_kiops = 280;
int32_t cn_udelay = 0;
static int32_t enable_disk_io(lfsm_dev_t *, int32_t, int32_t, int32_t, int32_t,
                              int8_t * data);

int32_t ioctl_change_dev_name(lfsm_dev_t * td, sftl_change_dev_name_t * arg)
{
    int32_t i;
    if (arg->disk_num >= td->cn_ssds) {
        return -1;
    }
    strcpy(grp_ssds.ar_drives[arg->disk_num].nm_dev, arg->nm_dev);
    for (i = 0; i < td->cn_ssds; i++) {
        LOGINFO("disk %d name %s \n", i, grp_ssds.ar_drives[i].nm_dev);
    }
    return 0;
}

int32_t ioctl_check_ddmap_updatelog_bmt(lfsm_dev_t * td,
                                        sftl_err_msg_t * perr_msg,
                                        sftl_ioctl_sys_dump_t * arg)
{
    int32_t i;
    if (arg) {
        dedicated_map_load(&td->DeMap, arg->num_item, td->cn_ssds);
    }

    for (i = 0; i < td->cn_ssds; i++) {
        supermap_dump(grp_ssds.ar_subdev, i);
    }

    if (!compare_whole_eu(td, td->DeMap.eu_backup, td->DeMap.eu_main,
                          LFSM_CEIL_DIV(ddmap_get_ondisk_size(td->cn_ssds),
                                        HARD_PAGE_SIZE), false, false, perr_msg,
                          ddmap_dump)) {
        printk("Dedecape different\n");
    } else {
        printk("Dedicate the same\n");
    }

    if (!compare_whole_eu(td, td->UpdateLog.eu_backup, td->UpdateLog.eu_main,
                          1, false, true, perr_msg, UpdateLog_dump)) {
        printk("update_log different\n");
    } else {
        printk("UpdateLog the same\n");
    }

    for (i = 0; i < td->ltop_bmt->BMT_num_EUs; i++) {
        if (!compare_whole_eu
            (td, td->ltop_bmt->BMT_EU[i], td->ltop_bmt->BMT_EU_backup[i], 1,
             false, true, perr_msg, NULL)) {
            printk(" BMT eu %d is different \n", i);
        } else {
            printk(" BMT eu %d is same \n", i);
        }
    }

    return perr_msg->cn_err;
}

int32_t ioctl_lfsm_properties(lfsm_dev_t * td,
                              struct sftl_ioctl_lfsm_properties *arg)
{

    struct sftl_lfsm_properties prop;
    int32_t i;
    prop.cn_total_eus = 0;
    for (i = 0; i < td->cn_ssds; i++)
        prop.cn_total_eus += gc[i]->total_eus;
    prop.cn_fpage_per_seu = FPAGE_PER_SEU;
    prop.sz_lfsm_in_page = td->logical_capacity;
    prop.cn_item_hpeutab = td->hpeutab.cn_total;

    prop.cn_ssds = td->cn_ssds;

    prop.cn_bioc = td->freedata_bio_capacity;

    LOGINFO
        ("Total eus:%d flasg_page_per_seu:%d capacity %d cn_ssds:%d, cn_bioc:%d\n",
         prop.cn_total_eus, prop.cn_fpage_per_seu, prop.sz_lfsm_in_page,
         prop.cn_ssds, prop.cn_bioc);
    if (copy_to_user(arg->pprop, &prop, sizeof(struct sftl_lfsm_properties)) !=
        0)
        return -EACCES;

    return 0;

}

int32_t ioctl_check_exec(lfsm_dev_t * td, enum eop_check op_check, sector64 idx,
                         sftl_err_msg_t * perr_msg)
{
    //printk(" get %lld idx \n",idx);

    switch (op_check) {
    case OP_EU_PBNO_MAPPING:
        return get_metadata_with_check_new(td, pbno_eu_map[idx], perr_msg);
        break;

    case OP_LPN_PBNO_MAPPING:
        return check_one_lpn(td, idx, perr_msg);
        break;

    case OP_EU_IDHPEU:
        return check_eu_idhpeu(td, pbno_eu_map[idx], perr_msg);
        break;

    case OP_HPEU_TAB:
        return (check_hpeu_table(td, idx, perr_msg));
        break;

    case OP_EU_MDNODE:
        return (check_eu_mdnode(td, pbno_eu_map[idx], perr_msg));
        break;

    case OP_EU_VALID_COUNT:
        return (check_eu_bitmap_validcount(td, pbno_eu_map[idx], perr_msg));
        break;

    case OP_EU_STATE:
        return (check_disk_eu_state(td, idx, perr_msg));
        break;

    case OP_ACTIVE_EU_FROTIER:
        return (check_active_eu_frontier(td, idx, perr_msg));
        break;

    case OP_CHECK_SYS_TABLE:
        return (check_sys_table(td, idx, perr_msg));
        break;
    case OP_CHECK_FREE_BIOC:
        return (check_freelist_bioc(td, idx, perr_msg));
        break;
    case OP_CHECK_FREE_BIOC_SIMPLE:
        return (check_freelist_bioc_simple(td, idx, perr_msg));
        break;

    case OP_SYSTABLE_HPEU:
        return (check_systable_hpeu(td, idx, perr_msg));
        break;

    default:
        LOGERR("Cannot get command %d \n", op_check);
    }

    return -EFAULT;

}

// check main function
int32_t ioctl_check(lfsm_dev_t * td, sftl_ioctl_chk_t * arg)
{
    sector64 i;
    int32_t ret = 0;
    int32_t cret = 0;
    sftl_err_msg_t *perr_msg;

    enum eop_check op_check = arg->op_check;
    int32_t sz_msg = arg->sz_err_msg;

    if ((perr_msg =
         track_vmalloc(sz_msg, GFP_KERNEL | __GFP_ZERO, M_IOCTL)) == NULL) {
        LOGERR("alloc size %d memory fail \n", sz_msg);
        return -ENOMEM;
    }
    perr_msg->cn_err = 0;

    for (i = 0; i < arg->cn_check; i++)
        ioctl_check_exec(td, op_check, i + arg->id_start, perr_msg);    // AN : err code in perr_msg

    if ((cret = copy_to_user(arg->perr_msg, perr_msg, sz_msg)) != 0) {
        LOGERR("Copy to user fail ret= %d\n", cret);
        ret = -EACCES;
    }
    track_vfree(perr_msg, sz_msg, M_IOCTL);
    //LOGINFO(" check return %d\n", ret);

    return ret;
}

int32_t ioctl_copy_to_kernel(lfsm_dev_t * td, sftl_cp2kern_t * arg)
{

    if (copy_from_user(arg->pkernel, arg->puser, arg->cn) > 0) {
        LOGERR("copy %d byte from %p to %p fail\n", arg->cn, arg->puser,
               arg->pkernel);
        return -ENOMEM;
    }
    return 0;
}

int32_t ioctl_scandev(lfsm_dev_t * td, sftl_ioctl_scandev_t * arg)
{
    // pick one dev name from spare disk
    // sdx, the 2 th word is the key dev name
    int8_t nm_new_disk[MAXLEN_DEV_NAME];
    sysapi_bdev *bdev;
    int32_t id_new_disk, id_drive, ret;
    sftl_ioctl_scandev_t kernel_arg;

    if (copy_from_user
        (&kernel_arg, (void __user *)arg, sizeof(sftl_ioctl_scandev_t)) != 0) {
        LOGERR
            ("Cannot copy data: arg from userspace to kernel space in ioctl_scandev().\n");
        return -EFAULT;
    }

    ret = 0;

    LOGINFO(" lfsm get scan-dev command : Action:%s Dev-name:%s\n",
            kernel_arg.action, kernel_arg.nm_dev);

    if ((id_new_disk = sysapi_bdev_name2id(kernel_arg.nm_dev)) < 0) {
        LOGINFO(" %s(%d) is not a lfsm handlable device name \n",
                kernel_arg.nm_dev, id_new_disk);
        return 0;
    }

    if (strcmp(kernel_arg.action, "add") == 0) {
        // erase before insert the disk
        LOGINFO("open bdev name %s id %d\n",
                sysapi_bdev_id2name(id_new_disk, nm_new_disk), id_new_disk);

        if (IS_ERR_OR_NULL((bdev =
                            sysapi_bdev_open(sysapi_bdev_id2name
                                             (id_new_disk, nm_new_disk))))) {
            LOGERR("Cannot open disk %s when try-to enqueue to hotswap pool\n",
                   kernel_arg.nm_dev);
            return -EACCES;
        }
        // no matter success or not, close then enqueue
        devio_erase(0, NUM_EUS_PER_SUPER_EU, bdev);

        sysapi_bdev_close(bdev);

        if ((ret = hotswap_euqueue_disk(&td->hotswap, id_new_disk, 1)) < 0) {
            return ret;
        }

        td->hotswap.fg_enable = true;
        wake_up_interruptible(&td->hotswap.wq);

        return 0;
    }

    if (strcmp(kernel_arg.action, "remove") == 0) {
        // serch if the removed disk in grps_ssd
        for (id_drive = 0; id_drive < td->cn_ssds; id_drive++) {
            if (strcmp(grp_ssds.ar_drives[id_drive].nm_dev,
                       sysapi_bdev_id2name(id_new_disk, nm_new_disk)) != 0) {
                continue;
            }
            diskgrp_switch(&grp_ssds, id_drive, 0);
            LOGINFO("Set disk %d name %s to inactivated\n", id_drive,
                    kernel_arg.nm_dev);
            break;
        }
        // search the removed disk in spare disk
        //hotswap_delete_disk(&td->hotswap,id_new_disk);// no need to handle return value
        return 0;
    }
    LOGINFO("invalid action %s\n", kernel_arg.action);
    return 0;
}

int32_t ioctl_trim(lfsm_dev_t * td, sector64 lpn, sector64 cn_pages)    // in flash page
{
    sector64 i;
    int32_t ret = 0;
    struct D_BMT_E dbmta;
    sector64 pbno;
    struct bio_container *pbioc;

    if (cn_pages > 8192) {
        LOGERR("cn_page %lld should be <=8192\n", cn_pages);
        return -EPERM;
    }

    if (IS_ERR(pbioc = bioc_lock_lpns_wait(td, lpn, cn_pages))) {
        return PTR_ERR(pbioc);
    }

    LOGINFO("lock lpn %lld %lld\n", lpn, cn_pages);
    for (i = 0; i < cn_pages; i++) {
        if ((ret = bmt_lookup(td, td->ltop_bmt, lpn + i, false, 1, &dbmta)) < 0) {
            LOGERR("Fail to bmt lookup for lpn: %lld\n", lpn + i);
            break;
        }
        pbno = (sector64) dbmta.pbno;
        if (pbno != PBNO_SPECIAL_NEVERWRITE) {
            update_old_eu(NULL, 1, &pbno);
            ret = PPDM_BMT_cache_insert_one_pending(td, lpn + i, PBNO_SPECIAL_NEVERWRITE, 1, true, DIRTYBIT_CLEAR, false);  // remove dirtybit and pininsert one pending
        } else
            ret = PPDM_BMT_cache_remove_pin(td->ltop_bmt, lpn + i, 1);
        if (ret < 0) {
            LOGERR("Fail to cache insert pin %d for pbno %lld\n", ret, pbno);
            break;
        }
    }
    bioc_unlock_lpns(td, pbioc);
    return ret;
}

int lfsm_ioctl(struct block_device *device, fmode_t mode, unsigned int cmd,
               unsigned long arg)
{
    int32_t ret = -ENOTTY;
    lfsm_dev_t *td;
    sftl_ioctl_simu_w_t *ioctl_simu = NULL;
    sftl_ioctl_trim_t *parg_trim = NULL;
    struct block_device *p_device;
    p_device =
        (struct block_device *)track_kmalloc(sizeof(struct block_device),
                                             GFP_KERNEL, M_OTHER);
    memset(p_device, 0, sizeof(struct block_device));
    memcpy((unsigned char *)p_device, (unsigned char *)device,
           sizeof(struct block_device));

    td = p_device->bd_disk->private_data;
    ioctl_simu = (sftl_ioctl_simu_w_t *) arg;

    if (!capable(CAP_SYS_ADMIN)) {
        return -EPERM;
    }

    switch (cmd) {
    case SFTL_IOCTL_TRIM:
        parg_trim = (sftl_ioctl_trim_t *) arg;
        ret = ioctl_trim(td, parg_trim->lpn, parg_trim->cn_pages);
        break;
    case SFTL_IOCTL_SIMU_IO:   /* for simulation */
        ret = 0;                //simulate_async_ios(td, ioctl_simu->m_nSum, ioctl_simu->m_nMd, ioctl_simu->m_nSize, ioctl_simu->m_nBatchSize, ioctl_simu->m_nType, ioctl_simu->m_nRw, ioctl_simu->m_start_sector);
        break;
    case SFTL_IOCTL_INIT_FIN:  /* for unit test */
        ret =
            enable_disk_io(td, ioctl_simu->m_nType, ioctl_simu->m_nSum,
                           ioctl_simu->m_nSize, ioctl_simu->m_nBatchSize,
                           ioctl_simu->data);
        break;
    case SFTL_IOCTL_SCANDEV:
        ret = ioctl_scandev(td, (sftl_ioctl_scandev_t *) arg);
        break;
    case SFTL_IOCTL_SYS_DUMPALL:
        ret =
            (ioctl_check_ddmap_updatelog_bmt
             (td, NULL, (sftl_ioctl_sys_dump_t *) arg));
        break;
    case SFTL_IOCTL_CHANGE_DEVNAME_DUMPALL:
        ret = (ioctl_change_dev_name(td, (sftl_change_dev_name_t *) arg));
        break;
    case SFTL_IOCTL_GET_LFSM_PROPERTY:
        ret =
            (ioctl_lfsm_properties
             (td, (struct sftl_ioctl_lfsm_properties *)arg));
        break;
    case SFTL_IOCTL_COPY_TO_KERNEL:
        ret = (ioctl_copy_to_kernel(td, (sftl_cp2kern_t *) arg));
        break;
    case SFTL_IOCTL_CHECK:
        ret = (ioctl_check(td, (sftl_ioctl_chk_t *) arg));
        break;
    }
    track_kfree(p_device, sizeof(struct block_device), M_OTHER);

    return ret;                 /* unknown command */
}

/* 
** Enable read I/O workload and clear the in-memory cache
*/
int32_t enable_disk_io(lfsm_dev_t * td, int32_t type, int32_t num, int32_t len,
                       int32_t destaddr, int8_t * data)
{
    struct D_BMT_E dbmta;
    HPEUTabItem_t *pItem;
    int8_t *buffer;
    sector64 pbno_af_adj;
    int32_t ret, i, dev_id;

    ret = 0;

    switch (type) {
    case 32:
        ret = bmt_lookup(td, td->ltop_bmt, num, false, false, &dbmta);
        printk("LFSM ioctl: pbno: %llu sector for lpn:%d page ret=%d\n",
               (sector64) dbmta.pbno, num, ret);
        ret = dbmta.pbno;
        break;
    case 45:
        bmt_lookup(td, td->ltop_bmt, num, false, false, &dbmta);
        pbno_lfsm2bdev(dbmta.pbno, &pbno_af_adj, &dev_id);

        printk("\nLFSM ioctl: af_adj_pageno %llu  disk num %d\n",
               pbno_af_adj >> SECTORS_PER_FLASH_PAGE_ORDER, dev_id);
        ret = pbno_af_adj;
        break;

    case 49:
        print_eu_erase_count(gc[0]);
        break;
    case 59:
        for (i = 0; i < 10; i++) {
            PPDM_BMT_dbmta_dump(&td->ltop_bmt->per_page_queue[i]);
        }
        break;
    case 66:
        pbno_lfsm2bdev(num, &pbno_af_adj, &dev_id);
        printk("\nLFSM ioctl af_adj_pageno %llu  disk num %d\n",
               pbno_af_adj / SECTORS_PER_FLASH_PAGE_ORDER, dev_id);
        ret = pbno_af_adj;
        break;
    case 75:
        pItem = HPEU_GetTabItem(&td->hpeutab, num);

        for (i = 0; i < td->hpeutab.cn_disk; i++) {
            printk("LFSM ioctl: %d th ar_peu: %p\n", i, pItem->ar_peu[i]);
        }

        break;
    case 76:
        for (i = 0; i < MAXCN_HOTSWAP_SPARE_DISK; i++) {
            lfsmdev.hotswap.ar_actived[num] = 1;
        }
        break;
    case 79:
        lfsmdev.hotswap.ar_actived[num] = len;
        break;
    case 80:
        diskgrp_switch(&grp_ssds, num, len);
        break;
    case 81:
        grp_ssds.ar_subdev[num].load_from_prev =
            (len == 0) ? non_ready : all_ready;
        break;
    case 82:
        printk("LFSM ioctl: ioctl cmd 82 send id_dev=%d , start rebuild \n",
               num);
        if (rddcy_rebuild_hpeus_A(&td->hpeutab, &grp_ssds, (num |= (1 << 24))) <
            0) {
            printk("LFSM ioctl: rebuild fail\n");
            ret = -EIO;
        } else {
            printk("LFSM ioctl: rebuild pass\n");
        }
        break;
    case 85:
        ret = rddcy_check_hpeus_data_ecc(&td->hpeutab, &grp_ssds);
        break;
    case 108:
        buffer = (int8_t *) track_kmalloc(81920, GFP_KERNEL, M_IOCTL);
        dump_bio_active_list(td, buffer);
        printk("%s", buffer);
        track_kfree(buffer, 81920, M_IOCTL);
        break;
    case 114:
        FreeList_Dump(gc[0]);
        break;
    case 116:
        conflict_vec_dumpall(td, num);
        break;
    case 118:
        bioc_dump_missing(td);
        break;
    case 119:
        printk
            ("==== Do idle metalog log current act EU packet (no lock)====\n");
        for (i = 0; i < td->cn_ssds; i++) {
            ret = metalog_packet_cur_eus(&td->mlogger, i, gc);
        }
        break;
    case 200:
        ret = check_allEU_valid_lpn_new(td);
        break;
    case 201:
        check_all_lpn(td);
        break;
    case 300:
        for (i = 0; i < td->cn_ssds; i++) {
            freelist_scan_and_check(gc[i]);
        }
        break;
    case 301:
        mdnode_check_exist_all();
        break;
    default:
        printk("LFSM ioctl: not support this now type %d\n", type);
        break;
    }

    return ret;
}
