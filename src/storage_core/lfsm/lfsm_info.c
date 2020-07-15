/*
 * SPDX-License-Identifier: GPL-2.0-or-later OR Apache-2.0
 * Copyright (C) 2015-2020 Industrial Technology Research Institute
 *
 *
 *
 *  
 */

#include <linux/version.h>
#include <linux/proc_fs.h>
#include <linux/swap.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include "config/lfsm_setting.h"
#include "config/lfsm_feature_setting.h"
#include "config/lfsm_cfg.h"
#include "common/common.h"
#include "common/mem_manager.h"
#include "common/rmutex.h"
#include "sysapi_linux_kernel.h"
#include "io_manager/mqbiobox.h"
#include "io_manager/biocbh.h"
#include "nextprepare.h"
#include "diskgrp.h"
#include "system_metadata/Dedicated_map.h"
#include "lfsm_core.h"
#include "io_manager/mqbiobox.h"
#include "io_manager/io_common.h"
#include "dbmtapool.h"
#include "bmt.h"
#include "autoflush.h"
#include "batchread.h"
#include "stripe.h"
#include "pgroup.h"
#include "erase_count.h"
#include "metalog.h"
#include "hotswap.h"
#include "hpeu.h"
#ifdef THD_EXEC_STEP_MON
#include "lfsm_thread_monitor.h"
#endif
#include "perf_monitor/thd_perf_mon.h"
#include "perf_monitor/ssd_perf_mon.h"

static ssize_t qdepth_read(struct file *file, char __user * buffer,
                           size_t nbytes, loff_t * ppos)
{
    int32_t len, sum, cp_len;
    int8_t *local_buf;
    int32_t ret_toUser;
    static uint32_t finished = 0;

    if (finished) {
        finished = 0;
        return 0;
    }

    local_buf = kmalloc(sizeof(int8_t) * PAGE_SIZE, GFP_KERNEL);
    sum = 0;

    sum = get_total_waiting_io(&lfsmdev.bioboxpool);

    len = sprintf(local_buf, "%d\n", sum);

    cp_len = len > nbytes ? nbytes : len;

    ret_toUser = copy_to_user(buffer, local_buf, cp_len);
    finished = 1;

    kfree(local_buf);

    return cp_len;
}

static uint64_t tm_last = 0;
static int32_t last_cn_r = 0, last_cn_w = 0;

static ssize_t iodone_read(struct file *file, char __user * buffer,
                           size_t nbytes, loff_t * ppos)
{
    int32_t sum, len, cp_len, cn_r, cn_w, i;
    int64_t tm_curr;
    lfsm_dev_t *td;
    int8_t *local_buf;
    int32_t ret_toUser;
    static uint32_t finished = 0;

    if (finished) {
        finished = 0;
        return 0;
    }

    sum = 0;
    len = 0;
    cn_r = 0;
    cn_w = 0;
    tm_curr = 0;
    td = &lfsmdev;

    cn_r = atomic_read(&td->biocbh.cnt_readNOPPNIO_done);
    for (i = 0; i < lfsmCfg.cnt_bh_threads; i++) {
        cn_r += td->biocbh.cnt_readIO_done[i];
        cn_w += td->biocbh.cnt_writeIO_done[i];
    }

    tm_curr = jiffies_to_msecs(get_jiffies_64());
    local_buf = kmalloc(sizeof(int8_t) * PAGE_SIZE, GFP_KERNEL);

    sum = get_total_waiting_io(&td->bioboxpool);

    if (tm_last == 0) {
        len = sprintf(local_buf, "0 0 1 0\n");
    } else {
        len = sprintf(local_buf, "%d %d %lld %d\n",
                      (cn_r - last_cn_r), (cn_w - last_cn_w),
                      (tm_curr - tm_last), sum);
    }

    tm_last = tm_curr;
    last_cn_r = cn_r;
    last_cn_w = cn_w;

    cp_len = len > nbytes ? nbytes : len;

    ret_toUser = copy_to_user(buffer, local_buf, cp_len);
    finished = 1;

    kfree(local_buf);
    return cp_len;
}

static int32_t _show_disk_status(int8_t * buffer, int32_t buf_size)
{
    int8_t nm_dev[32];
    struct block_device *bdev;
    struct shotswap *phs;
    int8_t *devName;
    int32_t i, j, len, diskIndex;
    subdisk_info_t *disk;

    len = 0;
    len += sprintf(buffer + len, "drive name group state\n");
    for (i = 0; i < lfsmdev.cn_pgroup; i++) {
        if (lfsmdev.ar_pgroup[i].state == epg_good) {
            for (j = 0; j < lfsmdev.hpeutab.cn_disk; j++) {
                diskIndex = HPEU_EU2HPEU_OFF(j, i);
                disk = &grp_ssds.ar_drives[diskIndex];
                len +=
                    sprintf(buffer + len, "%d %s %d normal\n", diskIndex,
                            disk->nm_dev, i);
            }
        } else {
            for (j = 0; j < lfsmdev.hpeutab.cn_disk; j++) {
                diskIndex = HPEU_EU2HPEU_OFF(j, i);
                disk = &grp_ssds.ar_drives[diskIndex];
                bdev = lookup_bdev(disk->nm_dev);
                devName = disk->nm_dev;
                if (IS_ERR_OR_NULL(bdev)) {
                    len +=
                        sprintf(buffer + len, "%d %s %d removed\n", diskIndex,
                                devName, i);
                } else if (diskgrp_isfaildrive(&grp_ssds, diskIndex)) {
                    len +=
                        sprintf(buffer + len, "%d %s %d failed\n", diskIndex,
                                devName, i);
                } else if (grp_ssds.ar_subdev[diskIndex].load_from_prev ==
                           sys_ready) {
                    len +=
                        sprintf(buffer + len, "%d %s %d rebuilding\n",
                                diskIndex, devName, i);
                } else {
                    len +=
                        sprintf(buffer + len, "%d %s %d read-only\n", diskIndex,
                                devName, i);
                }
            }
        }
    }

    phs = &lfsmdev.hotswap;
    for (i = 0; i < phs->cn_spare_disk; i++) {
        if (IS_ERR_OR_NULL
            (lookup_bdev
             (sysapi_bdev_id2name(phs->ar_id_spare_disk[i], nm_dev)))) {
            len +=
                sprintf(buffer + len, "%d %s %d removed\n", i,
                        sysapi_bdev_id2name(phs->ar_id_spare_disk[i], nm_dev),
                        -1);
        } else if (phs->ar_actived[i] == 0) {
            len += sprintf(buffer + len, " %d %s %d failed\n",
                           i, sysapi_bdev_id2name(phs->ar_id_spare_disk[i],
                                                  nm_dev), -1);
        } else {
            len += sprintf(buffer + len, "%d %s %d normal\n",
                           i, sysapi_bdev_id2name(phs->ar_id_spare_disk[i],
                                                  nm_dev), -1);
        }
    }

    return len;
}

static ssize_t disk_status_read(struct file *file, char __user * buffer,
                                size_t nbytes, loff_t * ppos)
{
    int8_t *local_buf;
    int32_t len, cp_len;

    int32_t ret_toUser;
    static uint32_t finished = 0;

    if (finished) {
        finished = 0;
        return 0;
    }

    local_buf = kmalloc(sizeof(int8_t) * PAGE_SIZE, GFP_KERNEL);

    if (!IS_ERR_OR_NULL(local_buf)) {
        len = _show_disk_status(local_buf, nbytes);

        cp_len = len > nbytes ? nbytes : len;
        local_buf[cp_len] = '\0';
        ret_toUser = copy_to_user(buffer, local_buf, cp_len);
        finished = 1;
        kfree(local_buf);
    } else {
        cp_len = 0;
        LOGINFO("fail alloc mem when read disk status\n");
    }

    return cp_len;
}

#ifdef  HLIST_GC_WHOLE_LOCK_DEBUG
static ssize_t hlistGC_lock_status_read(struct file *file, char __user * buffer,
                                        size_t nbytes, loff_t * ppos)
{
    int8_t *local_buf;
    int32_t len, cp_len;

    int32_t ret_toUser;
    static uint32_t finished = 0;

    if (finished) {
        finished = 0;
        return 0;
    }

    local_buf = kmalloc(sizeof(int8_t) * PAGE_SIZE, GFP_KERNEL);

    if (!IS_ERR_OR_NULL(local_buf)) {
        len = show_hlist_lock_status(local_buf, nbytes);

        cp_len = len > nbytes ? nbytes : len;
        local_buf[cp_len] = '\0';
        ret_toUser = copy_to_user(buffer, local_buf, cp_len);
        finished = 1;
        kfree(local_buf);
    } else {
        cp_len = 0;
        LOGINFO("fail alloc mem when read disk status\n");
    }

    return cp_len;
}
#endif

#ifdef THD_EXEC_STEP_MON
static ssize_t thd_monitor_read(struct file *file, char __user * buffer,
                                size_t nbytes, loff_t * ppos)
{
    int8_t *local_buf;
    int32_t len, cp_len;

    int32_t ret_toUser;
    static uint32_t finished = 0;

    if (finished) {
        finished = 0;
        return 0;
    }

    local_buf = kmalloc(sizeof(int8_t) * PAGE_SIZE, GFP_KERNEL);

    if (!IS_ERR_OR_NULL(local_buf)) {
        len = show_thread_monitor(local_buf, nbytes);

        cp_len = len > nbytes ? nbytes : len;
        local_buf[cp_len] = '\0';
        ret_toUser = copy_to_user(buffer, local_buf, cp_len);
        finished = 1;
        kfree(local_buf);
    } else {
        cp_len = 0;
        LOGINFO("fail alloc mem when read disk status\n");
    }

    return cp_len;
}
#endif

static ssize_t monitor_read(struct file *file, char __user * buffer,
                            size_t nbytes, loff_t * ppos)
{
    int32_t i, cp_len, len;
    int8_t *local_buf;
    lfsm_dev_t *td;
    sector64 cn_eu_total, cn_eu_free;
    struct HListGC *h;

    int32_t ret_toUser;
    static uint32_t finished = 0;

    if (finished) {
        finished = 0;
        return 0;
    }

    td = &lfsmdev;
    len = 0;
    cn_eu_total = 0;
    cn_eu_free = 0;

    local_buf = kmalloc(sizeof(int8_t) * PAGE_SIZE, GFP_KERNEL);

    len += bioboxpool_dump(&td->bioboxpool, local_buf);

    len += bmt_dump(td->ltop_bmt, local_buf + len);
    len += sprintf(local_buf + len, "metalog onthefly packet %d\n",
                   atomic_read(&td->mlogger.par_mlmgr[0].cn_full));

    len += sprintf(local_buf + len,
                   "bmt log frontier=%d pages conflict r=%d w=%d\nfree_bioc=%d (extra:%d) batchR=%d\n",
                   td->UpdateLog.eu_main->log_frontier / SECTOR_SIZE,
                   atomic_read(&td->cn_conflict_pages_r),
                   atomic_read(&td->cn_conflict_pages_w),
                   atomic_read(&td->len_datalog_free_list),
                   atomic_read(&td->cn_extra_bioc), -1);

    len += sprintf(local_buf + len, "updatelog forntier: %dp thres: %ld\n",
                   td->UpdateLog.eu_main->
                   log_frontier >> SECTORS_PER_FLASH_PAGE_ORDER, FPAGE_PER_SEU);
    len +=
        sprintf(local_buf + len,
                "multilen: %d padding due to hot WL: %d , gc giveup %d\n",
                atomic_read(&td->cn_alloc_bioext),
                atomic_read(&lfsm_pgroup_cn_padding_page),
                atomic_read(&gcn_gc_conflict_giveup));

    len += sprintf(local_buf + len, "group_valid(%lld, %lld): ",
                   td->ar_pgroup[0].ideal_valid_page,
                   td->ar_pgroup[0].max_valid_page);
    for (i = 0; i < td->cn_pgroup; i++) {
        len += sprintf(local_buf + len, "[%d]=%lld, status %s \n",
                       i, atomic64_read(&td->ar_pgroup[i].cn_valid_page),
                       (td->ar_pgroup[i].state) ? "Open" : "Close");
    }
    len += sprintf(local_buf + len, "metalog Synced %s\n",
                   metalog_packet_flushed(&td->mlogger,
                                          gc) ? "Finish" : "Non-Finish");

#if LFSM_FLUSH_SUPPORT == 1
    len +=
        sprintf(local_buf + len, "cn_flush-bio %d\n",
                atomic_read(&td->cn_flush_bio));
#endif

    len +=
        sprintf(local_buf + len, "cn_trim-bio%d \n",
                atomic_read(&td->cn_trim_bio));

    for (i = 0; i < lfsmdev.cn_ssds; i++) {
        h = gc[i];
        cn_eu_total += h->total_eus;
        cn_eu_free += h->free_list_number;
    }

    atomic64_set(&lfsmdev.ltop_bmt->cn_dirty_page_ssd,
                 (cn_eu_total - cn_eu_free) << (FLASH_PAGE_PER_BLOCK_ORDER +
                                                SUPER_EU_ORDER));
    len +=
        sprintf(local_buf + len,
                "total valid page %llu total dirty page %llu\n",
                atomic64_read(&lfsmdev.ltop_bmt->cn_valid_page_ssd),
                atomic64_read(&lfsmdev.ltop_bmt->cn_dirty_page_ssd));

    cp_len = len > nbytes ? nbytes : len;

    ret_toUser = copy_to_user(buffer, local_buf, cp_len);
    finished = 1;

    kfree(local_buf);
    return cp_len;
}

static ssize_t activeq_read(struct file *file, char __user * buffer,
                            size_t nbytes, loff_t * ppos)
{
    uint32_t len, cp_len;
    int8_t *local_buf;
    lfsm_dev_t *td;
    int32_t ret_toUser;
    static uint32_t finished = 0;

    if (finished) {
        finished = 0;
        return 0;
    }

    len = 0;
    td = &lfsmdev;
    local_buf = kmalloc(sizeof(int8_t) * PAGE_SIZE, GFP_KERNEL);

    len += dump_bio_active_list(td, local_buf);

    cp_len = len > nbytes ? nbytes : len;

    ret_toUser = copy_to_user(buffer, local_buf, cp_len);
    finished = 1;

    kfree(local_buf);
    return cp_len;
}

static ssize_t frontier_read(struct file *file, char __user * buffer,
                             size_t nbytes, loff_t * ppos)
{
    int32_t len, i, cp_len;
    struct HPEUTab *hpeutab;
    int8_t *local_buf;

    int32_t ret_toUser;
    static uint32_t finished = 0;

    if (finished) {
        finished = 0;
        return 0;
    }

    local_buf = kmalloc(sizeof(int8_t) * PAGE_SIZE, GFP_KERNEL);

    hpeutab = &lfsmdev.hpeutab;
    len = 0;
    len += sprintf(local_buf, "          C       W       H\n");
    for (i = 0; i < lfsmdev.cn_ssds; i++) {
        len +=
            sprintf(local_buf + len, "%2d: %2llx:%4d %2llx:%4d %2llx:%4d\n", i,
                    HPEU_get_birthday(hpeutab,
                                      gc[i]->
                                      active_eus[0]) & 0x0FFFFFFFFFFFFFFF,
                    gc[i]->active_eus[0]->
                    log_frontier >> SECTORS_PER_FLASH_PAGE_ORDER,
                    HPEU_get_birthday(hpeutab,
                                      gc[i]->
                                      active_eus[1]) & 0x0FFFFFFFFFFFFFFF,
                    gc[i]->active_eus[1]->
                    log_frontier >> SECTORS_PER_FLASH_PAGE_ORDER,
                    HPEU_get_birthday(hpeutab,
                                      gc[i]->
                                      active_eus[2]) & 0x0FFFFFFFFFFFFFFF,
                    gc[i]->active_eus[2]->
                    log_frontier >> SECTORS_PER_FLASH_PAGE_ORDER);
    }
    cp_len = len > nbytes ? nbytes : len;

    ret_toUser = copy_to_user(buffer, local_buf, cp_len);
    finished = 1;

    kfree(local_buf);
    return cp_len;
}

static ssize_t cnlist_read(struct file *file, char __user * buffer,
                           size_t nbytes, loff_t * ppos)
{
    int32_t i, len, cp_len;
    int8_t *local_buf;
    lfsm_dev_t *td;
    struct HListGC *h;

    int32_t ret_toUser;
    static uint32_t finished = 0;

    if (finished) {
        finished = 0;
        return 0;
    }

    len = 0;
    td = &lfsmdev;
    local_buf = kmalloc(sizeof(int8_t) * PAGE_SIZE, GFP_KERNEL);

    for (i = 0; i < td->cn_ssds; i++) {
        h = gc[i];
        len += sprintf(local_buf + len,
                       "disk[%d] cn_clean %d, cn_free %llu, cn_heap %d, cn_hlist %d\n",
                       i, atomic_read(&h->cn_clean_eu), h->free_list_number,
                       h->LVP_Heap_number, h->hlist_number);
    }

    for (i = 0; i < td->cn_pgroup; i++) {
        len += sprintf(local_buf + len, "grp[%d] (select %d) min_id %d\n",
                       i, atomic_read(&grp_ssds.ar_cn_selected[i]),
                       td->ar_pgroup[i].min_id_disk);
    }
    cp_len = len > nbytes ? nbytes : len;
    ret_toUser = copy_to_user(buffer, local_buf, cp_len);
    finished = 1;

    kfree(local_buf);
    return cp_len;
}

static int32_t _hlist_read_gut(int8_t * buffer, int32_t size, int32_t id_disk)
{
    int32_t i, len, total_page;
    struct HListGC *h;
    struct EUProperty *pEU;

    len = 0;
    total_page = 0;
    h = gc[id_disk];

    // scan from hlist
    i = 0;
    list_for_each_entry_reverse(pEU, &h->hlist, list) {
        len += sprintf(buffer + len, "%d eu(%p) %llu vn %d fg %d\n",
                       i, pEU, pEU->start_pos, atomic_read(&pEU->eu_valid_num),
                       atomic_read(&pEU->state_list));
        total_page += atomic_read(&pEU->eu_valid_num) / SECTORS_PER_FLASH_PAGE;
        i++;
        if ((len + 40) > size) {
            len += sprintf(buffer + len, "......\n");
            break;
        }
    }

    len += sprintf(buffer + len, "freelist num= %llu clean num %d\n",
                   h->free_list_number, atomic_read(&h->cn_clean_eu));
    len += sprintf(buffer + len, "Total_Valid_Page = %d\n", total_page);

    return len;
}

static int32_t _get_diskNum_by_file(struct file *file, int8_t * path_prefix)
{
    int8_t *tmp, *pathname, *ptr_disk_id;
    struct path path_p;
    int32_t disk_id;

    disk_id = 0;

    do {
        tmp = (int8_t *) __get_free_page(GFP_KERNEL);
        if (!tmp) {
            printk(KERN_ERR "lfsm : fail alloc to get disk num\n");
            break;
        }

        path_p = file->f_path;
        path_get(&file->f_path);
        pathname = d_path(&path_p, tmp, PAGE_SIZE);
        path_put(&path_p);

        if (IS_ERR(pathname)) {
            free_page((unsigned long)tmp);
            printk(KERN_ERR "lfsm : fail alloc to get path\n");
            break;
        }

        if (strncmp(pathname, path_prefix, strlen(path_prefix)) == 0) {
            ptr_disk_id = pathname + strlen(path_prefix);
            sscanf(ptr_disk_id, "%d", &disk_id);
        } else {
            printk(KERN_ERR "lfsm : fail get disk id from path %s\n", pathname);
        }

        free_page((unsigned long)tmp);

    } while (0);

    return disk_id;
}

static ssize_t hlist_read(struct file *file, char __user * buffer,
                          size_t nbytes, loff_t * ppos)
{
    int32_t len, cp_len, ret_toUser;
    int8_t *local_buf;
    static uint32_t finished = 0;

    if (finished) {
        finished = 0;
        return 0;
    }

    local_buf = kmalloc(sizeof(int8_t) * PAGE_SIZE, GFP_KERNEL);
    len =
        _hlist_read_gut(buffer, PAGE_SIZE,
                        _get_diskNum_by_file(file, "/proc/lfsm/hlist/disk"));
    cp_len = len > nbytes ? nbytes : len;

    ret_toUser = copy_to_user(buffer, local_buf, cp_len);
    finished = 1;

    kfree(local_buf);

    return cp_len;
}

static ssize_t system_eu_frontier_proc_read(struct file *file,
                                            char __user * buffer, size_t nbytes,
                                            loff_t * ppos)
{
    int32_t len, frontier, frontier_in_hpage, backward_hpage;
    int32_t super_eu_backward_hpage, cp_len;
    int8_t *local_buf;

    int32_t ret_toUser;
    static uint32_t finished = 0;

    if (finished) {
        finished = 0;
        return 0;
    }

    len = 0;
    backward_hpage = 0;
    super_eu_backward_hpage = 0;

    local_buf = kmalloc(sizeof(int8_t) * PAGE_SIZE, GFP_KERNEL);

    len +=
        sprintf(local_buf + len,
                "system eu log frontier start in %lup(%lus)\n "
                "super map log frontier start in %lup(%lus)\n "
                "HPAGE_PER_SEU=%lu\n\n", (HPAGE_PER_SEU - backward_hpage),
                (HPAGE_PER_SEU - backward_hpage) << SECTORS_PER_HARD_PAGE_ORDER,
                (HPAGE_PER_SEU - super_eu_backward_hpage),
                (HPAGE_PER_SEU -
                 super_eu_backward_hpage) << SECTORS_PER_HARD_PAGE_ORDER,
                HPAGE_PER_SEU);

    len += sprintf(local_buf + len, "%20s   %30s   %25s   %25s\n",
                   "table_name", "frontier in sectors(hpages)",
                   "pages remain in EU", "reset count");

    frontier = grp_ssds.ar_subdev[0].super_frontier;
    frontier_in_hpage = frontier / SECTORS_PER_HARD_PAGE;
    len += sprintf(local_buf + len, "%20s   %25d(%6d)   %21ld   %25d\n",
                   "DISK 0 SUPER MAP", frontier, frontier_in_hpage,
                   HPAGE_PER_SEU - frontier_in_hpage,
                   grp_ssds.ar_subdev[0].cn_super_cycle);

    frontier = lfsmdev.DeMap.eu_main->log_frontier;
    frontier_in_hpage = frontier / SECTORS_PER_HARD_PAGE;
    len += sprintf(local_buf + len, "%20s   %25d(%6d)   %21ld   %25d\n",
                   "DEDICATED MAP", frontier, frontier_in_hpage,
                   HPAGE_PER_SEU - frontier_in_hpage, lfsmdev.DeMap.cn_cycle);

    frontier = lfsmdev.UpdateLog.eu_main->log_frontier;
    frontier_in_hpage = frontier / SECTORS_PER_HARD_PAGE;
    len += sprintf(local_buf + len, "%20s   %25d(%6d)   %21ld   %25d\n",
                   "UPDATE_LOG", frontier, frontier_in_hpage,
                   HPAGE_PER_SEU - frontier_in_hpage,
                   lfsmdev.UpdateLog.cn_cycle);

    frontier = gc[0]->ECT.eu_curr->log_frontier;
    frontier_in_hpage = frontier / SECTORS_PER_HARD_PAGE;
    len += sprintf(local_buf + len, "%20s   %25d(%6d)   %21ld   %25d\n",
                   "DISK 0 ECT", frontier, frontier_in_hpage,
                   HPAGE_PER_SEU - frontier_in_hpage, gc[0]->ECT.cn_cycle);

    frontier = lfsmdev.hpeutab.pssl.eu[0]->log_frontier;
    frontier_in_hpage = frontier / SECTORS_PER_HARD_PAGE;
    len += sprintf(local_buf + len, "%20s   %25d(%6d)   %21ld   %25d\n",
                   "HPEU", frontier, frontier_in_hpage,
                   HPAGE_PER_SEU - frontier_in_hpage,
                   lfsmdev.hpeutab.pssl.cn_cycle);

    cp_len = len > nbytes ? nbytes : len;
    ret_toUser = copy_to_user(buffer, local_buf, cp_len);
    finished = 1;

    kfree(local_buf);
    return cp_len;
}

static int32_t _heap_read_gut(int8_t * buffer, int32_t size, int32_t id_disk)
{
    int32_t i, len, total_page;
    struct HListGC *h;

    len = 0;
    total_page = 0;
    h = gc[id_disk];
    // scan from heap
    for (i = 0; i < h->LVP_Heap_number; i++) {
        len += sprintf(buffer + len, "%d eu(%p) %llu vn %d fg %d\n", i,
                       h->LVP_Heap[i], h->LVP_Heap[i]->start_pos,
                       atomic_read(&h->LVP_Heap[i]->eu_valid_num),
                       atomic_read(&h->LVP_Heap[i]->state_list));
        total_page +=
            atomic_read(&h->LVP_Heap[i]->eu_valid_num) / SECTORS_PER_FLASH_PAGE;
        if ((len + 40) > size) {
            len += sprintf(buffer + len, "......\n");
            break;
        }
    }

    len += sprintf(buffer + len, "freelist num= %llu, clean num %d\n",
                   h->free_list_number, atomic_read(&h->cn_clean_eu));
    len += sprintf(buffer + len, "Total_Valid_Page = %d\n", total_page);

    return len;
}

static ssize_t heap_read(struct file *file, char __user * buffer, size_t nbytes,
                         loff_t * ppos)
{
    int32_t len, cp_len, ret_toUser;
    int8_t *local_buf;
    static uint32_t finished = 0;

    if (finished) {
        finished = 0;
        return 0;
    }

    local_buf = kmalloc(sizeof(int8_t) * PAGE_SIZE, GFP_KERNEL);
    len =
        _heap_read_gut(buffer, PAGE_SIZE,
                       _get_diskNum_by_file(file, "/proc/lfsm/heap/disk"));
    cp_len = len > nbytes ? nbytes : len;

    ret_toUser = copy_to_user(buffer, local_buf, cp_len);
    finished = 1;

    kfree(local_buf);

    return cp_len;
}

static ssize_t ect_alldisk_proc_read(struct file *file, char __user * buffer,
                                     size_t nbytes, loff_t * ppos)
{
    uint32_t len, cp_len;
    int8_t *local_buf;
    int32_t ret_toUser;
    static uint32_t finished = 0;

    if (finished) {
        finished = 0;
        return 0;
    }

    local_buf = kmalloc(sizeof(int8_t) * PAGE_SIZE, GFP_KERNEL);
    len = ECT_dump_alldisk(local_buf, PAGE_SIZE);
    cp_len = len > nbytes ? nbytes : len;

    ret_toUser = copy_to_user(buffer, local_buf, cp_len);
    finished = 1;

    kfree(local_buf);
    return cp_len;
}

static ssize_t rddcy_proc_pgroup_num_read(struct file *file,
                                          char __user * buffer, size_t nbytes,
                                          loff_t * ppos)
{
    uint32_t len, cp_len;
    int8_t *local_buf;
    int32_t ret_toUser;
    static uint32_t finished = 0;

    if (finished) {
        finished = 0;
        return 0;
    }

    local_buf = kmalloc(sizeof(int8_t) * PAGE_SIZE, GFP_KERNEL);
    len = rddcy_pgroup_num_get(local_buf, PAGE_SIZE);
    cp_len = len > nbytes ? nbytes : len;

    ret_toUser = copy_to_user(buffer, local_buf, cp_len);
    finished = 1;

    kfree(local_buf);
    return cp_len;
}

static ssize_t rddcy_proc_pgroup_size_read(struct file *file,
                                           char __user * buffer, size_t nbytes,
                                           loff_t * ppos)
{
    uint32_t len, cp_len;
    int8_t *local_buf;
    int32_t ret_toUser;
    static uint32_t finished = 0;

    if (finished) {
        finished = 0;
        return 0;
    }

    local_buf = kmalloc(sizeof(int8_t) * PAGE_SIZE, GFP_KERNEL);
    len = rddcy_pgroup_size_get(local_buf, PAGE_SIZE);
    cp_len = len > nbytes ? nbytes : len;

    ret_toUser = copy_to_user(buffer, local_buf, cp_len);
    finished = 1;

    kfree(local_buf);
    return cp_len;
}

static ssize_t rddcy_proc_faildrive_read(struct file *file,
                                         char __user * buffer, size_t nbytes,
                                         loff_t * ppos)
{
    uint32_t len, cp_len;
    int8_t *local_buf;
    int32_t ret_toUser;
    static uint32_t finished = 0;

    if (finished) {
        finished = 0;
        return 0;
    }

    local_buf = kmalloc(sizeof(int8_t) * PAGE_SIZE, GFP_KERNEL);
    len = rddcy_faildrive_get(local_buf, PAGE_SIZE);
    cp_len = len > nbytes ? nbytes : len;

    ret_toUser = copy_to_user(buffer, local_buf, cp_len);
    finished = 1;

    kfree(local_buf);
    return cp_len;
}

static ssize_t HPEU_cn_damage_read(struct file *file, char __user * buffer,
                                   size_t nbytes, loff_t * ppos)
{
    uint32_t len, cp_len;
    int8_t *local_buf;
    int32_t ret_toUser;
    static uint32_t finished = 0;

    if (finished) {
        finished = 0;
        return 0;
    }

    local_buf = kmalloc(sizeof(int8_t) * PAGE_SIZE, GFP_KERNEL);
    len = HPEU_cn_damage_get(local_buf, PAGE_SIZE);
    cp_len = len > nbytes ? nbytes : len;

    ret_toUser = copy_to_user(buffer, local_buf, cp_len);
    finished = 1;

    kfree(local_buf);
    return cp_len;
}

static ssize_t hotswap_proc_read(struct file *file, char __user * buffer,
                                 size_t nbytes, loff_t * ppos)
{
    uint32_t len, cp_len;
    int8_t *local_buf;
    int32_t ret_toUser;
    static uint32_t finished = 0;

    if (finished) {
        finished = 0;
        return 0;
    }

    local_buf = kmalloc(sizeof(int8_t) * PAGE_SIZE, GFP_KERNEL);
    len = hotswap_info_read(local_buf, PAGE_SIZE);
    cp_len = len > nbytes ? nbytes : len;

    ret_toUser = copy_to_user(buffer, local_buf, cp_len);
    finished = 1;

    kfree(local_buf);
    return cp_len;
}

static ssize_t thd_perfMon_read(struct file *file, char __user * buffer,
                                size_t nbytes, loff_t * ppos)
{
    uint32_t len, cp_len;
    int8_t *local_buf;
    int32_t ret_toUser;
    static uint32_t finished = 0;

    if (finished) {
        finished = 0;
        return 0;
    }

    local_buf = kmalloc(sizeof(int8_t) * PAGE_SIZE, GFP_KERNEL);
    len = show_thd_perf_monitor(local_buf, PAGE_SIZE);
    cp_len = len > nbytes ? nbytes : len;

    ret_toUser = copy_to_user(buffer, local_buf, cp_len);
    finished = 1;
    kfree(local_buf);
    return cp_len;
}

static ssize_t thd_perfMon_write(struct file *file, const char __user * buffer,
                                 size_t count, loff_t * ppos)
{
    int8_t local_buf[128];
    uint32_t cp_len;

    memset(local_buf, 0, 128);
    cp_len = 128 > count ? count : 128;

    if (copy_from_user(local_buf, buffer, cp_len)) {
        return -EFAULT;
    }

    local_buf[cp_len] = '\0';

    exec_thd_perfMon_cmdstr(local_buf, cp_len);

    return count;
}

static ssize_t ssd_perfMon_read(struct file *file, char __user * buffer,
                                size_t nbytes, loff_t * ppos)
{
    uint32_t len, cp_len;
    int8_t *local_buf;
    int32_t ret_toUser;
    static uint32_t finished = 0;

    if (finished) {
        finished = 0;
        return 0;
    }

    local_buf = kmalloc(sizeof(int8_t) * PAGE_SIZE, GFP_KERNEL);
    len = show_ssd_perf_monitor(local_buf, PAGE_SIZE);
    cp_len = len > nbytes ? nbytes : len;

    ret_toUser = copy_to_user(buffer, local_buf, cp_len);
    finished = 1;

    kfree(local_buf);
    return cp_len;
}

static ssize_t ssd_perfMon_write(struct file *file, const char __user * buffer,
                                 size_t count, loff_t * ppos)
{
    int8_t local_buf[128];
    uint32_t cp_len;

    memset(local_buf, 0, 128);
    cp_len = 128 > count ? count : 128;

    if (copy_from_user(local_buf, buffer, cp_len)) {
        return -EFAULT;
    }

    local_buf[cp_len] = '\0';

    exec_ssd_perfMon_cmdstr(local_buf, cp_len);

    return count;
}

static ssize_t cfg_read(struct file *file, char __user * buffer, size_t nbytes,
                        loff_t * ppos)
{
    uint32_t len, cp_len;
    int8_t *local_buf;
    int32_t ret_toUser;
    static uint32_t finished = 0;

    if (finished) {
        finished = 0;
        return 0;
    }

    local_buf = kmalloc(sizeof(int8_t) * PAGE_SIZE, GFP_KERNEL);
    len = show_lfsm_config(local_buf, PAGE_SIZE);
    cp_len = len > nbytes ? nbytes : len;

    ret_toUser = copy_to_user(buffer, local_buf, cp_len);
    finished = 1;

    kfree(local_buf);
    return cp_len;
}

static ssize_t cfg_write(struct file *file, const char __user * buffer,
                         size_t count, loff_t * ppos)
{
    int8_t local_buf[128];
    uint32_t cp_len;

    memset(local_buf, 0, 128);
    cp_len = 128 > count ? count : 128;

    if (copy_from_user(local_buf, buffer, cp_len)) {
        return -EFAULT;
    }

    local_buf[cp_len] = '\0';

    exec_lfsm_config_cmdstr(local_buf, cp_len);

    return count;
}

static ssize_t mem_usage_read(struct file *file, char __user * buffer,
                              size_t nbytes, loff_t * ppos)
{
    uint32_t len, cp_len;
    int8_t *local_buf;
    int32_t ret_toUser;
    static uint32_t finished = 0;

    if (finished) {
        finished = 0;
        return 0;
    }

    local_buf = kmalloc(sizeof(int8_t) * PAGE_SIZE, GFP_KERNEL);
    len = 0;
    len += dump_dbmtE_BKPool_usage(&lfsmdev.dbmtapool, local_buf + len);
    len += dump_memory_usage(local_buf + len, PAGE_SIZE - len);
    cp_len = len > nbytes ? nbytes : len;

    ret_toUser = copy_to_user(buffer, local_buf, cp_len);
    finished = 1;

    kfree(local_buf);
    return cp_len;
}

const struct file_operations iodone_fops = {
    .owner = THIS_MODULE,
    .read = iodone_read,
};

//const struct file_operations hpeutbl_fops = {
//.owner = THIS_MODULE,
//.read = HPEU_dump,
//};

const struct file_operations qdepth_fops = {
    .owner = THIS_MODULE,
    .read = qdepth_read,
};

const struct file_operations cnlist_fops = {
    .owner = THIS_MODULE,
    .read = cnlist_read,
};

const struct file_operations monitor_fops = {
    .owner = THIS_MODULE,
    .read = monitor_read,
};

const struct file_operations frontier_fops = {
    .owner = THIS_MODULE,
    .read = frontier_read,
};

const struct file_operations activeQ_fops = {
    .owner = THIS_MODULE,
    .read = activeq_read,
};

const struct file_operations hlist_fops = {
    .owner = THIS_MODULE,
    .read = hlist_read,
};

const struct file_operations heap_fops = {
    .owner = THIS_MODULE,
    .read = heap_read,
};

//const struct file_operations ect_disk_fops = {
//.owner = THIS_MODULE,
//.read = ECT_dump_disk,
//};

const struct file_operations ect_alldisks_fops = {
    .owner = THIS_MODULE,
    .read = ect_alldisk_proc_read,
};

const struct file_operations pgroupNum_fops = {
    .owner = THIS_MODULE,
    .read = rddcy_proc_pgroup_num_read,
};

const struct file_operations pgroupSize_fops = {
    .owner = THIS_MODULE,
    .read = rddcy_proc_pgroup_size_read,
};

const struct file_operations faildrive_fops = {
    .owner = THIS_MODULE,
    .read = rddcy_proc_faildrive_read,
};

const struct file_operations HPEU_cn_damage_fops = {
    .owner = THIS_MODULE,
    .read = HPEU_cn_damage_read,
};

const struct file_operations hotswap_fops = {
    .owner = THIS_MODULE,
    .read = hotswap_proc_read,
};

const struct file_operations syseu_front_fops = {
    .owner = THIS_MODULE,
    .read = system_eu_frontier_proc_read,
};

const struct file_operations disk_status_fops = {
    .owner = THIS_MODULE,
    .read = disk_status_read,
};

#ifdef HLIST_GC_WHOLE_LOCK_DEBUG
const struct file_operations hlistGC_lock_status_fops = {
    .owner = THIS_MODULE,
    .read = hlistGC_lock_status_read,
};
#endif

#ifdef THD_EXEC_STEP_MON
const struct file_operations thd_monitor_fops = {
    .owner = THIS_MODULE,
    .read = thd_monitor_read,
};
#endif

const struct file_operations thd_perfMon_fops = {
    .owner = THIS_MODULE,
    .read = thd_perfMon_read,
    .write = thd_perfMon_write,
};

const struct file_operations ssd_perfMon_fops = {
    .owner = THIS_MODULE,
    .read = ssd_perfMon_read,
    .write = ssd_perfMon_write,
};

const struct file_operations lfsm_cfg_fops = {
    .owner = THIS_MODULE,
    .read = cfg_read,
    .write = cfg_write,
};

const struct file_operations mem_usage_fops = {
    .owner = THIS_MODULE,
    .read = mem_usage_read,
};

static struct proc_dir_entry *_create_sofa_proc(const int8_t * name,
                                                mode_t mode,
                                                struct proc_dir_entry *parent,
                                                const struct file_operations
                                                *fop)
{
    struct proc_dir_entry *proc_entry;

    proc_entry = proc_create(name, mode, parent, fop);

    if (IS_ERR(proc_entry)) {
        printk(KERN_ERR "lfsm : Couldn't create proc entry %s\n", name);
    }

    return proc_entry;
}

void init_procfs(SProcLFSM_t * pproc)
{
    int32_t i;
    int8_t name[8];

    pproc->dir_main = proc_mkdir("lfsm", NULL);
    pproc->dir_hlist = proc_mkdir("hlist", pproc->dir_main);
    pproc->dir_heap = proc_mkdir("heap", pproc->dir_main);
    pproc->dir_ect = proc_mkdir("erasetab", pproc->dir_main);
    pproc->dir_rddcy = proc_mkdir("pgroup", pproc->dir_main);
    pproc->dir_perfMon = proc_mkdir("perf_monitor", pproc->dir_main);

    /*----------------- create proc for dir_hlist (hlist) -----------------*/
    for (i = 0; i < lfsmdev.cn_ssds; i++) {
        //TODO: Lego Support after 3.10
        sprintf(name, "disk%02d", i);
        //_create_sofa_proc(name, 0644, pproc->dir_hlist, &hlist_fops);
    }

    /*----------------- create proc for dir_heap (heap) -----------------*/
    for (i = 0; i < lfsmdev.cn_ssds; i++) {
        //TODO: Lego Support after 3.10
        sprintf(name, "disk%02d", i);
        //_create_sofa_proc(name, 0644, pproc->dir_heap, &heap_fops);
    }

    /*----------------- create proc for dir_ect (erasetab) -----------------*/
    for (i = 0; i < lfsmdev.cn_ssds; i++) {
        sprintf(name, "disk%02d", i);
        //TODO: Lego Support after 3.10
        //_create_sofa_proc(name, 0644, pproc->dir_ect, &ect_disk_fops);
    }
    _create_sofa_proc("all", 0644, pproc->dir_ect, &ect_alldisks_fops);

    /*----------------- create proc for dir_rddcy (pgroup) -----------------*/
    _create_sofa_proc("num", 0644, pproc->dir_rddcy, &pgroupNum_fops);
    _create_sofa_proc("size", 0644, pproc->dir_rddcy, &pgroupSize_fops);
    _create_sofa_proc("faildrive", 0644, pproc->dir_rddcy, &faildrive_fops);
    _create_sofa_proc("damege_hpeu", 0644, pproc->dir_rddcy,
                      &HPEU_cn_damage_fops);

    /*----------------- create proc for dir_main (lfsm) -----------------*/
    _create_sofa_proc("iodone", 0644, pproc->dir_main, &iodone_fops);
    //TODO: Lego Support after 3.10
    //_create_sofa_proc("hpeu_tab", 0644, pproc->dir_main, &hpeutbl_fops);

    _create_sofa_proc("qdepth", 0644, pproc->dir_main, &qdepth_fops);
    _create_sofa_proc("listall", 0644, pproc->dir_main, &cnlist_fops);
    _create_sofa_proc("monitor", 0644, pproc->dir_main, &monitor_fops);
    _create_sofa_proc("frontier", 0644, pproc->dir_main, &frontier_fops);
    _create_sofa_proc("activeQ", 0644, pproc->dir_main, &activeQ_fops);
    _create_sofa_proc("hotswap", 0644, pproc->dir_main, &hotswap_fops);
    _create_sofa_proc("system_eu_frontier", 0644, pproc->dir_main,
                      &syseu_front_fops);
    _create_sofa_proc("status", 0644, pproc->dir_main, &disk_status_fops);

#ifdef HLIST_GC_WHOLE_LOCK_DEBUG
    _create_sofa_proc("hlistGC_lock_status", 0644, pproc->dir_main,
                      &hlistGC_lock_status_fops);
#endif

#ifdef THD_EXEC_STEP_MON
    _create_sofa_proc("thd_monitor", 0644, pproc->dir_main, &thd_monitor_fops);
#endif

    _create_sofa_proc("thread_perf", 0644, pproc->dir_perfMon,
                      &thd_perfMon_fops);
    _create_sofa_proc("ssd_perf", 0644, pproc->dir_perfMon, &ssd_perfMon_fops);

    _create_sofa_proc("lfsm_config", 0644, pproc->dir_main, &lfsm_cfg_fops);
    _create_sofa_proc("memory_usage", 0644, pproc->dir_main, &mem_usage_fops);

    for (i = 0; i < lfsmdev.cn_pgroup; i++) {
        atomic_set(&grp_ssds.ar_cn_selected[i], 0);
    }
}

void release_procfs(SProcLFSM_t * pproc)
{
    int32_t i;

    i = 0;
    if (IS_ERR_OR_NULL(pproc->dir_main) || IS_ERR_OR_NULL(pproc->dir_ect)) {
        return;
    }

    remove_proc_entry("qdepth", pproc->dir_main);
    remove_proc_entry("iodone", pproc->dir_main);
    remove_proc_entry("listall", pproc->dir_main);
    remove_proc_entry("monitor", pproc->dir_main);
    remove_proc_entry("frontier", pproc->dir_main);

    remove_proc_entry("activeQ", pproc->dir_main);
    remove_proc_entry("hotswap", pproc->dir_main);

    remove_proc_entry("status", pproc->dir_main);
    remove_proc_entry("system_eu_frontier", pproc->dir_main);

    remove_proc_entry("all", pproc->dir_ect);
    remove_proc_entry("hlist", pproc->dir_main);
    remove_proc_entry("heap", pproc->dir_main);
    remove_proc_entry("erasetab", pproc->dir_main);

    remove_proc_entry("num", pproc->dir_rddcy);
    remove_proc_entry("size", pproc->dir_rddcy);
    remove_proc_entry("faildrive", pproc->dir_rddcy);
    remove_proc_entry("damege_hpeu", pproc->dir_rddcy);
    remove_proc_entry("pgroup", pproc->dir_main);

#ifdef HLIST_GC_WHOLE_LOCK_DEBUG
    remove_proc_entry("hlistGC_lock_status", pproc->dir_main);
#endif
#ifdef THD_EXEC_STEP_MON
    remove_proc_entry("thd_monitor", pproc->dir_main);
#endif

    remove_proc_entry("thread_perf", pproc->dir_perfMon);
    remove_proc_entry("ssd_perf", pproc->dir_perfMon);
    //remove_proc_entry("io_perf", pproc->dir_perfMon);
    remove_proc_entry("perf_monitor", pproc->dir_main);
    remove_proc_entry("lfsm_config", pproc->dir_main);
    remove_proc_entry("memory_usage", pproc->dir_main);

    remove_proc_entry("lfsm", NULL);
}
