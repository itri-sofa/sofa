/*
 * SPDX-License-Identifier: GPL-2.0-or-later OR Apache-2.0
 * Copyright (C) 2015-2020 Industrial Technology Research Institute
 *
 *
 *
 *  
 */

#include <linux/bio.h>
#include <linux/types.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include "config/lfsm_setting.h"
#include "config/lfsm_feature_setting.h"
#include "config/lfsm_cfg.h"
#include "common/common.h"
#include "common/rmutex.h"
#include "io_manager/mqbiobox.h"
#include "io_manager/biocbh.h"
#include "nextprepare.h"
#include "diskgrp.h"
#include "system_metadata/Dedicated_map.h"
#include "lfsm_core.h"
#include "io_manager/io_common.h"
#include "GC.h"
#include "hotswap.h"
#include "sysapi_linux_kernel.h"
#ifdef THD_EXEC_STEP_MON
#include "lfsm_thread_monitor.h"
#endif

int32_t hotswap_info_read(int8_t * buffer, uint32_t size)
{
    shotswap_t *phs;
    int32_t len, i;
    int8_t nm_dev[MAXLEN_DEV_NAME];

    phs = &lfsmdev.hotswap;
    len = 0;
    len +=
        sprintf(len + buffer, "hot_swap : %s\n",
                (phs->fg_enable) ? "ON" : "OFF");

    len += sprintf(len + buffer, "-----------------------\n");

    len += sprintf(len + buffer, "Spare Disk :\n");

    for (i = 0; i < phs->cn_spare_disk; i++) {
        len += sprintf(len + buffer, "%02d : %s\n", i,
                       sysapi_bdev_id2name(phs->ar_id_spare_disk[i], nm_dev));
    }

    len += sprintf(len + buffer, "--------------\nDisk in use :\n");

    for (i = 0; i < grp_ssds.cn_drives; i++) {
        len += sprintf(len + buffer, "%02d : %s  %s \n", i,
                       grp_ssds.ar_drives[i].nm_dev,
                       (grp_ssds.ar_subdev[i].load_from_prev ==
                        non_ready) ? "non_ready" : (grp_ssds.ar_subdev[i].
                                                    load_from_prev ==
                                                    sys_ready) ? "sys_ready" :
                       "all_ready");
    }

    len +=
        sprintf(len + buffer, "--------------------------------------------\n");

    len += sprintf(len + buffer, "%s %d \n", phs->char_msg, phs->waittime);

    return len;
}

static int32_t hotswap_process(void *vphs)
{
    shotswap_t *phs;
    int32_t idx_slot, i, ret;
    int32_t id_disk_try[MAX_FAIL_DISK] = { -1 }, id_fail[MAX_FAIL_DISK], disk;
    int8_t nm_dev[MAXLEN_DEV_NAME];
    struct rddcy_fail_disk fail_disk;
    unsigned long flags;

    idx_slot = -1;
    i = 0;
    ret = -1;
    //id_fail=-1;

    phs = (shotswap_t *) vphs;

    current->flags |= PF_MEMALLOC;
    while (true) {
        sprintf(phs->char_msg, "wait Evevt");
        wait_event_interruptible(phs->wq,
                                 (((idx_slot =
                                    rddcy_search_failslot(&grp_ssds)) >= 0)
                                  && phs->fg_enable
                                  && atomic_read(&phs->td->dev_stat) !=
                                  LFSM_STAT_INIT)
                                 || kthread_should_stop());

        if (kthread_should_stop()) {
            break;
        }

        rddcy_fail_ids_to_disk(lfsmdev.hpeutab.cn_disk, idx_slot, &fail_disk);

        LOGINFO("Fail disks idx_slot 0x%x count %d\n", idx_slot,
                fail_disk.count);

        spin_lock_irqsave(&phs->lock, flags);
        if (phs->cn_spare_disk < fail_disk.count) {
            LOGINFO("No enough spare disk to rebuild fail disks %x\n",
                    idx_slot);
            phs->fg_enable = false;
            spin_unlock_irqrestore(&phs->lock, flags);
            continue;
        }
        spin_unlock_irqrestore(&phs->lock, flags);

        for (i = 0; i < fail_disk.count; i++) {
            disk = HPEU_EU2HPEU_OFF(fail_disk.idx[i], fail_disk.pg_id);
            id_fail[i] = sysapi_bdev_name2id(grp_ssds.ar_drives[disk].nm_dev);
            if (id_fail[i] < 0) {
                LOGERR("hotswap err fail find disk id for %s\n",
                       grp_ssds.ar_drives[disk].nm_dev);
                return 0;
            }
            if ((id_disk_try[i] = hotswap_dequeue_disk(phs, id_fail[i])) < 0) {

                LOGINFO("No spare disk can be rebuild %d id_fail disk\n",
                        id_fail[i]);
                phs->fg_enable = false;
                break;
            }
            sprintf(phs->char_msg,
                    "found a fail slot %d and use idx_disk %s to rebuild after",
                    disk, sysapi_bdev_id2name(id_disk_try[i], nm_dev));
        }

        if (i < fail_disk.count)
            continue;

        phs->waittime = 10;
        while (phs->waittime > 0) {
            msleep(1000);
            phs->waittime--;
        }

        for (i = 0; i < fail_disk.count; i++) {
            disk = HPEU_EU2HPEU_OFF(fail_disk.idx[i], fail_disk.pg_id);
            sprintf(phs->char_msg,
                    "found a fail slot %d and use idx_disk %s rebuilding",
                    disk, sysapi_bdev_id2name(id_disk_try[i], nm_dev));

            diskgrp_attach(&grp_ssds, disk, id_disk_try[i]);
        }

        if ((ret =
             rddcy_rebuild_fail_disk(&lfsmdev.hpeutab, &grp_ssds,
                                     &fail_disk)) < 0) {
            for (i = 0; i < fail_disk.count; i++) {
                disk = HPEU_EU2HPEU_OFF(fail_disk.idx[i], fail_disk.pg_id);
                LOGINFO
                    ("rebuild disk id %d disk name %s fail, swap %d, ret= %d\n",
                     disk, grp_ssds.ar_drives[disk].nm_dev, i, ret);
                diskgrp_detach(&grp_ssds, disk);
            }
            continue;
        }

        for (i = 0; i < fail_disk.count; i++) {
            disk = HPEU_EU2HPEU_OFF(fail_disk.idx[i], fail_disk.pg_id);
            LOGINFO
                ("rebuild disk id %d disk name %s ssuccess, swap %d, ret= %d\n",
                 disk, grp_ssds.ar_drives[disk].nm_dev, i, ret);

            /*if (id_fail[i] != id_disk_try[i]) {
               if (hotswap_euqueue_disk(&lfsmdev.hotswap, id_fail[i], 0) < 0) {
               LOGERR("Can't insert fail disk %d\n", id_fail[i]);
               break;
               }
               } */
        }
        /*if(i == fail_disk.count)
           continue;
         */
    }

    return 0;
}

int32_t hotswap_euqueue_disk(shotswap_t * phs, int32_t id_disk, int32_t actived)
{
    int8_t nm_dev[MAXLEN_DEV_NAME];
    unsigned long flags;
    int32_t i, ret;

    ret = 0;
    LOGINFO("add disk %s id %d to hotswap spare pool\n",
            sysapi_bdev_id2name(id_disk, nm_dev), id_disk);
    spin_lock_irqsave(&phs->lock, flags);

    if (phs->cn_spare_disk >= MAXCN_HOTSWAP_SPARE_DISK) {
        LOGWARN("add disk %d spare disk num %d \n", id_disk,
                phs->cn_spare_disk);
        ret = -ENOSPC;
        goto l_end;
    }

    for (i = 0; i < phs->cn_spare_disk; i++) {
        if (phs->ar_id_spare_disk[i] == id_disk) {
            LOGINFO("id disk %d spare disk num %d active %d\n",
                    id_disk, phs->cn_spare_disk, actived);
            phs->ar_actived[i] = actived;
            phs->fg_enable = true;
            goto l_end;
        }
    }

    phs->ar_id_spare_disk[phs->cn_spare_disk] = id_disk;
    phs->ar_actived[phs->cn_spare_disk] = actived;
    phs->cn_spare_disk++;
    phs->fg_enable = true;

  l_end:
    spin_unlock_irqrestore(&phs->lock, flags);
    wake_up_interruptible(&phs->wq);
    return ret;
}

int32_t hotswap_dequeue_disk(shotswap_t * phs, int32_t old_id)
{
    int32_t id_disk, i, start_idx;
    unsigned long flags;

    id_disk = 0;
    start_idx = 0;
    spin_lock_irqsave(&phs->lock, flags);

    if (phs->cn_spare_disk == 0) {
        id_disk = -ENODEV;
        goto l_end;
    }

    if (old_id >= 0) {
        for (i = 0; i < phs->cn_spare_disk; i++) {
            if (phs->ar_actived[i] == 0) {
                continue;
            }
            if (phs->ar_id_spare_disk[i] == old_id) {
                id_disk = old_id;
                start_idx = i;
                goto _FOUND_DISK;
            }
        }
    }

    for (i = 0; i < phs->cn_spare_disk; i++) {
        if (phs->ar_actived[i] == 0) {
            continue;
        }
        id_disk = phs->ar_id_spare_disk[i];
        start_idx = i;
        break;
    }

    if (i == phs->cn_spare_disk) {
        id_disk = -ENODEV;
        goto l_end;
    }

  _FOUND_DISK:
    phs->cn_spare_disk--;

    for (i = start_idx; i < phs->cn_spare_disk; i++) {
        phs->ar_id_spare_disk[i] = phs->ar_id_spare_disk[i + 1];
        phs->ar_actived[i] = phs->ar_actived[i + 1];
    }

  l_end:
    spin_unlock_irqrestore(&phs->lock, flags);

    return id_disk;
}

int32_t hotswap_delete_disk(shotswap_t * phs, int32_t id_disk)
{

    int32_t idx, i, ret;
    unsigned long flags;

    ret = -1;

    spin_lock_irqsave(&phs->lock, flags);

    for (idx = 0; idx < phs->cn_spare_disk; idx++) {
        if (phs->ar_id_spare_disk[idx] == id_disk) {
            phs->cn_spare_disk--;
            ret = 0;
            break;
        }
    }

    for (i = idx; i < phs->cn_spare_disk; i++) {
        phs->ar_id_spare_disk[i] = phs->ar_id_spare_disk[i + 1];
    }

    spin_unlock_irqrestore(&phs->lock, flags);

    return ret;
}

int32_t hotswap_init(shotswap_t * phs, lfsm_dev_t * td)
{
    int32_t ret;

    ret = 0;
    init_waitqueue_head(&phs->wq);
    spin_lock_init(&phs->lock);
    phs->cn_spare_disk = 0;
    phs->td = td;

    // fill in the data structure of ar_spare_disk
    phs->fg_enable = true;

    phs->pthread = kthread_run(hotswap_process, phs, "lfsm_hotswap");
    if (IS_ERR_OR_NULL(phs->pthread)) {
        LOGERR("can't start rebuild swap thread \n");
        ret = -EIO;
    } else {
#ifdef THD_EXEC_STEP_MON
        reg_thread_monitor(phs->pthread->pid, "lfsm_hotswap");
#endif
    }

    return ret;
}

int32_t hotswap_destroy(shotswap_t * phs)
{
    if (!IS_ERR_OR_NULL(phs->pthread)) {
        if (kthread_stop(phs->pthread) < 0) {
            LOGERR("can't stop hot_swap_thread\n");
        }
    }

    phs->pthread = NULL;

    return 0;
}
