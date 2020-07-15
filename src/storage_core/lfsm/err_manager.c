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
#include "err_manager.h"
#include "special_cmd.h"
#include "EU_create.h"
#include "io_manager/io_common.h"
#include "EU_access.h"
#include "GC.h"
#include "bmt.h"
#include "bmt_ppq.h"
#include "bmt_ppdm.h"
#include "spare_bmt_ul.h"

void move_to_FalseList(struct HListGC *h, struct EUProperty *vic_eu)
{
    int32_t ret;
    bool eu_invalid;

    LOGINFO("move_to_false eu start%llu hwflag=%d\n",
            vic_eu->start_pos, atomic_read(&vic_eu->state_list));

    switch (atomic_read(&vic_eu->state_list)) {
    case -1:
        //list_del_init(&vic_eu->list);
        //h->free_list_number--;
        break;
    case 0:
        eu_invalid = LVP_heap_delete(h, vic_eu);
        vic_eu->index_in_LVP_Heap = -1;
        break;
    case 1:
        if ((ret = HList_delete(h, vic_eu)) < 0) {
            LOGERR("Internal# hlist_delete fail ret %d\n", ret);
            LFSM_FORCE_RUN(return);
            LFSM_ASSERT(0);
        }
        break;
    case 2:
    default:
        LFSM_ASSERT(0);         // vic_eu in false_list isn't able to enter this function
        break;
    }

    FalseList_insert(h, vic_eu);    //Inserting the element to the tailof FalseList.
}

void errmgr_wait_falselist_clean(void)
{
    int32_t idx_disk;
    bool isWait;

    if (IS_ERR_OR_NULL(gc)) {
        return;
    }

    do {
        isWait = false;
        for (idx_disk = 0; idx_disk < lfsmdev.cn_ssds; idx_disk++) {
            if (IS_ERR_OR_NULL(gc[idx_disk])) {
                continue;
            }

            if (atomic_read(&gc[idx_disk]->false_list_number) != 0) {
                isWait = true;
            }
        }

        if (isWait) {
            schedule();
        }
    } while (isWait);
}

#if 0
void safe_move_eu_to_FalseList(lfsm_dev_t * td, struct EUProperty *eu)
{
    struct HListGC *h;

    h = gc[eu->disk_num];

    if (atomic_read(&eu->state_list) != 2 && eu->erase_count != EU_UNUSABLE) {
        //put victim eu to false_list, set bitmap invalid, set bmt cache fail marker
        LFSM_RMUTEX_LOCK(&h->whole_lock);
        mp_printk("errmgr_enter_move");
        move_to_FalseList(h, eu);

        if (eu_is_active(eu) >= 0) {
            h->active_eus[eu->temper] =
                FreeList_delete_with_log(td, h, EU_DATA, true);
            UpdateLog_logging(td, h->active_eus[eu->temper], eu->temper,
                              UL_NORMAL_MARK);
        }
        LFSM_RMUTEX_UNLOCK(&h->whole_lock);
    }
}
#endif

// vic = victim
void errmgr_bad_process(sector64 vic_pbno_start, page_t vic_lpn_start,
                        page_t vic_lpn_end, lfsm_dev_t * td, bool isMetadata,
                        bool to_remove_pin)
{
    struct EUProperty *eu;
    struct HListGC *h;

    eu = pbno_eu_map[(vic_pbno_start - eu_log_start) >> EU_BIT_SIZE];
//    page_t i_lpn;

    h = gc[eu->disk_num];
    err_handle_printk("%s: pbno %llu lpn %llu \n", __FUNCTION__, vic_pbno_start,
                      vic_lpn_start);

    diskgrp_logerr(&grp_ssds, eu->disk_num);

    return;
}
