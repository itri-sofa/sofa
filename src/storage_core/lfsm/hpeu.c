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
#include "hpeu.h"
#include "persistlog.h"
#include "EU_access.h"
#include "xorbmp.h"
#include "spare_bmt_ul.h"
#include "bmt_ppdm.h"
#include "EU_create.h"
#include "metabioc.h"
#include "stripe.h"
#include "pgroup.h"
#include "mdnode.h"

static int32_t _HPEU_GetTabItem_size(HPEUTab_t * hpeutab)
{
    return (sizeof(HPEUTabItem_t) +
            (sizeof(struct EUProperty *) * hpeutab->cn_disk));
}

int32_t dump_metadata(sector64 ** p_metadata, int32_t idx_fail,
                      int32_t cn_ssds_per_hpeu, int32_t next)
{
    int32_t i, j;
    LOGINFO("faildisk is %d\n", idx_fail);

    for (i = 0; i < next + 100; i++) {
        printk(" %d ", i);
        for (j = 0; j < cn_ssds_per_hpeu; j++) {
            //if(p_metadata[j][i] > 1000000)
            //    printk("xxxxx ");
            //else
            printk(" %20llx", p_metadata[j][i]);
        }
        msleep(1);
        printk("\n");
    }

    return 0;
}

// return cn_valid_page, =0 is no metainfo, <0 is fail
// The function will guarantee that all ar_lpn will be read from disk even under the case where one disk is fail
int32_t HPEU_metadata_smartread(lfsm_dev_t * td, int32_t id_hpeu,
                                struct EUProperty **ar_seu, sector64 ** ar_lpn)
{
    int32_t ret, idx_faildisk;
    struct rddcy_fail_disk fail_disk;

    ret = 0;
    idx_faildisk =
        rddcy_search_failslot_by_pg_id(&grp_ssds,
                                       PGROUP_ID(ar_seu[0]->disk_num));
    rddcy_fail_ids_to_disk(td->hpeutab.cn_disk, idx_faildisk, &fail_disk);
    //idx_faildisk = (idx_faildisk<0) ? idx_faildisk :
    //        (idx_faildisk % td->hpeutab.cn_disk);

    if ((ret = HPEU_read_metadata(td, ar_seu, ar_lpn, &fail_disk)) < 0) {
        LOGERR("Can't get metadata info from metadata_area and spare_area, "
               "recovery failure!!\n");
        return ret;
    }
#if 0
    if (lfsm_readonly == 1) {
        idx_faildisk = _get_faildisk(td, ar_lpn, ar_seu);
    }
#endif

    if (fail_disk.count == 1) {
        if ((ret = hpeu_get_eu_frontier(ar_lpn, fail_disk.idx[0], td->hpeutab.cn_disk)) <= 0) { //no valid_page
            LOGINFO("id_hpeu %d no valid page ret %d\n", id_hpeu, ret);
            return 0;
        }

        if ((ret =
             rddcy_rebuild_metadata(&td->hpeutab, ar_seu[0]->start_pos,
                                    &fail_disk, ar_lpn))
            < 0) {
            return ret;
        }
        //for (i=0; i<ret; i++)
        //    printk("%s: %d parity %d lpn= %llu pbn %llu\n",__FUNCTION__, i,RDDCY_IS_ECCLPN(ar_lpn[idx_faildisk][i]),ar_lpn[idx_faildisk][i],ar_seu[idx_faildisk]->start_pos+(i<<SECTORS_PER_FLASH_PAGE_ORDER));
    } else if (fail_disk.count == 0) {
        if ((ret = hpeu_get_eu_frontier(ar_lpn, 0, td->hpeutab.cn_disk)) <= 0) {    //no valid_page
            LOGINFO("id_hpeu %d no valid page ret %d\n", id_hpeu, ret);
            return 0;
        }
    }

    return ret;
}

int32_t HPEU_metadata_commit(lfsm_dev_t * td, struct EUProperty **ar_seu,
                             sector64 ** ar_lpn)
{
    int32_t i, idx_faildisk, idx;
    struct rddcy_fail_disk fail_disk;

    if (lfsm_readonly == 1) {
        return 0;
    }

    idx_faildisk = rddcy_search_failslot(&grp_ssds);
    rddcy_fail_ids_to_disk(td->hpeutab.cn_disk, idx_faildisk, &fail_disk);

    for (i = 0; i < td->hpeutab.cn_disk; i++) {
        idx = HPEU_HPEUOFF2EUIDX(ar_seu[i]->disk_num);
        if (idx == fail_disk.idx[0]) {
            continue;
        }

        if (ar_seu[i]->fg_metadata_ondisk) {
            continue;
        }

        if (!metabioc_commit(td, (onDisk_md_t *) ar_lpn[i], ar_seu[i])) {
            return -ENOMEM;
        }
    }
    return 0;
}

int32_t HPEU_cn_damage_get(int8_t * buffer, int32_t size)
{
    return sprintf(buffer, "%d\n", atomic_read(&lfsmdev.hpeutab.cn_damege));
}

void HPEU_get_eupos(HPEUTab_t * hpeutab, sector64 * pmain, sector64 * pbackup,
                    int32_t * pidx_next)
{
    *pmain = hpeutab->pssl.eu[PSSLOG_ID_EUMAIN]->start_pos;
    *pbackup = hpeutab->pssl.eu[PSSLOG_ID_EUBACKUP]->start_pos;
    *pidx_next = hpeutab->pssl.eu[PSSLOG_ID_EUMAIN]->log_frontier;
}

/*
void HPEU_set_eupos(HPEUTab_t *hpeutab,sector64 main, sector64 backup)
{
    hpeutab->pssl.eu[PSSLOG_ID_EUMAIN]->start_pos = main;
    hpeutab->pssl.eu[PSSLOG_ID_EUBACKUP]->start_pos = backup;
}
*/

HPEUTabItem_t *HPEU_GetTabItem(HPEUTab_t * hpeutab, int32_t idx)
{
    if (idx < 0 || idx >= hpeutab->cn_total) {
        LOGERR
            ("unexpected index of HPEUTabItem: %d (expected between 0 and %d)\n",
             idx, hpeutab->cn_total);
        dump_stack();
        return NULL;
    }
    return (HPEUTabItem_t *) ((int8_t *) (hpeutab->ar_item) +
                              (idx * _HPEU_GetTabItem_size(hpeutab)));
}

void HPEU_reset_birthday(HPEUTab_t * hpeutab)
{
    HPEUTabItem_t *pItem;
    int32_t i;

    for (i = 0; i < hpeutab->cn_total; i++) {
        pItem = HPEU_GetTabItem(hpeutab, i);
        if (pItem->cn_added != -1) {
            pItem->birthday = 0;
        }
    }
}

/*
 * Description: alloc EU for persist log of HPEU
 */
bool HPEU_alloc_log_EU(HPEUTab_t * hpeutab, int32_t id_dev0, int32_t id_dev1)
{
    int32_t ar_id_dev[PSSLOG_CN_EU] = { id_dev0, id_dev1 };

    if (PssLog_AllocEU(&hpeutab->pssl, ar_id_dev, NULL, false)) {
        LOGERR("psslog alloc failure\n");
        return false;
    }

    return true;
}

static int32_t _HPEU_alloc(HPEUTab_t * hpeutab)
{
    HPEUTabItem_t *pItem;
    unsigned long flag;
    int32_t i, ret_id;

    ret_id = 0;

    spin_lock_irqsave(&hpeutab->lock, flag);
    if (hpeutab->cn_used >= hpeutab->cn_total) {
        goto l_fail;
    }

    for (i = 0; i < hpeutab->cn_total; i++) {
        hpeutab->curri = (hpeutab->curri + 1) % hpeutab->cn_total;
        if (NULL == (pItem = HPEU_GetTabItem(hpeutab, hpeutab->curri))) {
            goto l_fail;
        }

        if (pItem->cn_added == -1) {
            break;
        }
    }

    hpeutab->cn_used++;
    ret_id = hpeutab->curri;
    if (NULL == (pItem = HPEU_GetTabItem(hpeutab, ret_id))) {
        goto l_fail;
    }
    pItem->cn_added = 0;
    atomic_set(&pItem->cn_unfinish_fpage,
               (DATA_FPAGE_IN_SEU * hpeutab->cn_disk));
    spin_unlock_irqrestore(&hpeutab->lock, flag);

    return ret_id;
  l_fail:
    spin_unlock_irqrestore(&hpeutab->lock, flag);
    return -1;
}

static bool _HPEU_add_eu(HPEUTab_t * hpeutab, int32_t id_hpeu,
                         struct EUProperty *peu)
{
    HPEUTabItem_t *pItem;
    unsigned long flag;
    bool ret;

    ret = false;
    spin_lock_irqsave(&hpeutab->lock, flag);
    if (NULL == (pItem = HPEU_GetTabItem(hpeutab, id_hpeu))) {
        goto l_fail;
    }

    if ((pItem->cn_added < 0) || (pItem->cn_added >= hpeutab->cn_disk)) {
        LOGERR("over add: ar_item[%d].cn_added=%d diskid=%d invalid\n",
               id_hpeu, pItem->cn_added, peu->disk_num);

        goto l_fail;
    }

    pItem->ar_peu[peu->disk_num % hpeutab->cn_disk] = peu;
    if (PGROUP_ID(pItem->ar_peu[0]->disk_num) != PGROUP_ID(peu->disk_num)) {
        LOGINERR("try to add wrong disk eu into a hpeu %d eu_new=%p\n", id_hpeu,
                 peu);
        goto l_fail;
    }

    pItem->cn_added++;
    ret = true;
//    printk("[%d]%s: id_hpeu %d eu pos=%lld temp=%d is set in. cnt=%d\n",current->pid,__FUNCTION__, id_hpeu, peu->start_pos,peu->temper,hpeutab->ar_item[id_hpeu].cn_added );
  l_fail:
    spin_unlock_irqrestore(&hpeutab->lock, flag);
    return ret;
}

static int32_t _HPEU_alloc_and_add(HPEUTab_t * hpeutab, struct EUProperty *peu)
{
    HPEUTabItem_t *pItem;
    int32_t id_hpeu;

    id_hpeu = _HPEU_alloc(hpeutab);
    if (id_hpeu == -1) {
        return -1;
    }

    if (!_HPEU_add_eu(hpeutab, id_hpeu, peu)) {
        HPEU_free(hpeutab, id_hpeu);
        LOGERR("_HPEU_add_eu fail eu %llu\n", peu->start_pos);
        return -1;
    }

    if (NULL == (pItem = HPEU_GetTabItem(hpeutab, id_hpeu))) {
        return -1;
    }

    pItem->birthday =
        lfsm_pgroup_birthday_next(hpeutab->td, PGROUP_ID(peu->disk_num),
                                  peu->temper);
//    printk("%s: id_hpeu %d pgrp = %d allocated birthday %llx\n",__FUNCTION__, id_hpeu, PGROUP_ID(peu->disk_num),hpeutab->ar_item[id_hpeu].birthday);
    return id_hpeu;
}

bool HPEU_free(HPEUTab_t * hpeutab, int32_t id_hpeu)
{
    HPEUTabItem_t *pItem;
    unsigned long flag;
    int32_t i;
    bool ret;

    ret = false;
    spin_lock_irqsave(&hpeutab->lock, flag);
    if (NULL == (pItem = HPEU_GetTabItem(hpeutab, id_hpeu))) {
        goto l_end;
    }

    pItem->cn_added = -1;
    atomic_set(&pItem->cn_unfinish_fpage, 0);
    for (i = 0; i < hpeutab->cn_disk; i++) {
        pItem->ar_peu[i] = NULL;
    }
    hpeutab->cn_used--;
    ret = true;

  l_end:
    spin_unlock_irqrestore(&hpeutab->lock, flag);
    return ret;
}

sector64 HPEU_get_frontier(sector64 pbno)
{
    struct EUProperty *peu;
    sector64 voffset, hoffset;

    peu = eu_find(pbno);
    voffset = pbno - peu->start_pos;
    hoffset = peu->disk_num % lfsmdev.hpeutab.cn_disk;
    return voffset * lfsmdev.hpeutab.cn_disk + hoffset;
}

bool HPEU_is_older_pbno(sector64 prev, sector64 curr)
{
    sector64 a, b;
    a = HPEU_get_frontier(prev);
    b = HPEU_get_frontier(curr);
    if (a > b) {
        return true;
    } else if (a == b) {
        if (PGROUP_ID(eu_find(prev)->disk_num) >
            PGROUP_ID(eu_find(curr)->disk_num)) {
            return true;
        }
    }

    return false;
}

int32_t HPEU_get_eus(HPEUTab_t * hpeutab, int32_t id_hpeu,
                     struct EUProperty **ret_ar_peu)
{
    HPEUTabItem_t *pItem;
    unsigned long flag;
    int32_t i, ret, cn_alloc;

    ret = -EINVAL;
    cn_alloc = 0;
    spin_lock_irqsave(&hpeutab->lock, flag);
    if (NULL == (pItem = HPEU_GetTabItem(hpeutab, id_hpeu))) {
        goto l_fail;
    }

    if (pItem->cn_added < 0) {
        goto l_fail;
    }

    for (i = 0; i < hpeutab->cn_disk; i++) {
        ret_ar_peu[i] = pItem->ar_peu[i];
        if (NULL == (ret_ar_peu[i])) {
            LOGWARN("id_hpeu %d %dth eu is null\n", id_hpeu, i);
        } else if (eu_find(ret_ar_peu[i]->start_pos) != ret_ar_peu[i]) {
            LOGINERR("id_hpeu %d %dth eu %p crash\n", id_hpeu, i,
                     ret_ar_peu[i]);
        } else {
            cn_alloc++;
        }
    }
    ret = cn_alloc;

  l_fail:
    spin_unlock_irqrestore(&hpeutab->lock, flag);
    return ret;
}

static int32_t _HPEU_get_total_valid(HPEUTabItem_t * pItem,
                                     int32_t cn_ssds_per_hpeu)
{
    struct EUProperty *eu;
    int32_t i, total_valid;

    total_valid = 0;

    for (i = 0; i < cn_ssds_per_hpeu; i++) {
        eu = pItem->ar_peu[i];
        total_valid += atomic_read(&eu->eu_valid_num);
    }

    return total_valid;
}

// without any concurrent protection. call only during recovery
// if id_faildisk < 0 then no skip
int32_t HPEU_iterate(HPEUTab_t * hpeutab, int32_t id_faildisk,
                     int32_t(*func) (struct EUProperty * eu, int32_t usage))
{
    HPEUTabItem_t *pItem;
    int32_t i, j, idx_skip, ret;

    idx_skip = -1;
    ret = 0;
    for (i = 0; i < hpeutab->cn_total; i++) {
        if (NULL == (pItem = HPEU_GetTabItem(hpeutab, i))) {
            ret = -EINVAL;
            goto l_fail;
        }

        if (pItem->cn_added == -1) {
            continue;
        }

        if ((id_faildisk >= 0)
            && (HPEU_get_pgroup_id(hpeutab, i) == PGROUP_ID(id_faildisk))) {
            idx_skip = HPEU_HPEUOFF2EUIDX(id_faildisk);
        } else {
            idx_skip = -1;
        }

        if (_HPEU_get_total_valid(pItem, hpeutab->cn_disk) == 0) {
            LOGINFO("free id_hpeu %d because it's all invalid\n", i);
            HPEU_free(hpeutab, i);
            continue;
        }

        for (j = 0; j < hpeutab->cn_disk; j++) {
            if ((pItem->ar_peu[j]) && (j != idx_skip)) {
                if ((ret = func(pItem->ar_peu[j], EU_DATA)) < 0) {
                    LOGERR("exec func %p failure (ret=%d)\n", func, ret);
                    goto l_fail;
                }
            }
        }
    }

  l_fail:
    return ret;
}

/*
 * Description: API save HPEU table
 * return   :   true : fail
 *          :   false: ok
 */
bool HPEU_save(HPEUTab_t * hpeutab)
{
    HPEUTabItem_t *pItem;
    unsigned long flag;
    int32_t *buf;
    int32_t i, j, align_sz;
    bool ret;

    ret = false;
    if (lfsm_readonly == 1) {
        return true;
    }

    align_sz = PssLog_get_entry_size(&hpeutab->pssl);

    if (NULL ==
        (buf = (int32_t *) track_kmalloc(align_sz, GFP_KERNEL, M_OTHER))) {
        return true;
    }

    spin_lock_irqsave(&hpeutab->lock, flag);
    for (i = 0; i < hpeutab->cn_total; i++) {
        if (NULL == (pItem = HPEU_GetTabItem(hpeutab, i))) {
            goto l_fail;
        }

        for (j = 0; j < hpeutab->cn_disk; j++) {
            buf[i * hpeutab->cn_disk + j] = -1;
            if (pItem->cn_added == hpeutab->cn_disk) {
                if (NULL == (pItem->ar_peu[j])) {
                    LOGERR("ar_item[%d].ar_peu[%d]=null\n", i, j);
                    LFSM_FORCE_RUN(continue);
                    goto l_fail;
                }

                buf[i * hpeutab->cn_disk + j] =
                    pItem->ar_peu[j]->start_pos >> SECTORS_PER_SEU_ORDER;
            } else if (pItem->cn_added != -1) {
                LOGWARN("ar_item[%d].cn_addr=%d<%d peu %p\n",
                        i, pItem->cn_added, hpeutab->cn_disk, pItem->ar_peu[j]);
            }
        }
    }
    spin_unlock_irqrestore(&hpeutab->lock, flag);

    /*
     * TODO: log entry is 4 bytes, but the start_pos of ar_peu is 8 bytes
     *       we might encount some issues
     *       JIRA FSS-61
     */
    ret = PssLog_Append(&hpeutab->pssl, (int8_t *) buf);
    track_kfree(buf, align_sz, M_OTHER);
    LOGINFO("HPEU save done ret=%d\n", ret);

    return ret;

  l_fail:
    spin_unlock_irqrestore(&hpeutab->lock, flag);
    track_kfree(buf, align_sz, M_OTHER);
    return true;
}

// idx_next in sector
bool HPEU_load(HPEUTab_t * hpeutab, sector64 pos_main, sector64 pos_backup,
               int32_t idx_next)
{
    HPEUTabItem_t *pItem;
    int32_t *buf;
    int32_t i, j, id_eu, cn_added, align_sz;
    bool ret;

    cn_added = 0;
    align_sz = PssLog_get_entry_size(&hpeutab->pssl);
    ret = false;

    if (NULL == (buf = (int32_t *)
                 track_kmalloc(align_sz, GFP_KERNEL, M_OTHER))) {
        return false;
    }

    if (PssLog_Assign
        (&hpeutab->pssl, pos_main, pos_backup, idx_next, (int8_t *) buf)) {
        goto l_fail;
    }

    for (i = 0; i < hpeutab->cn_total; i++) {
        if (NULL == (pItem = HPEU_GetTabItem(hpeutab, i))) {
            goto l_fail;
        }
        memset(pItem->ar_peu, 0,
               sizeof(struct EUProperty *) * hpeutab->cn_disk);
        if (buf[i * hpeutab->cn_disk] == -1) {
            pItem->cn_added = -1;
            continue;
        }

        cn_added = 0;
        for (j = 0; j < hpeutab->cn_disk; j++) {
            id_eu = buf[i * hpeutab->cn_disk + j];
            if (id_eu > 0) {
                pItem->ar_peu[j] = pbno_eu_map[id_eu];
                pbno_eu_map[id_eu]->id_hpeu = i;
                cn_added++;
            }
        }
        pItem->cn_added = cn_added;
    }

    ret = true;
    LOGINFO("HPEU load done ret=%d\n", ret);
  l_fail:
    if (!ret) {
        LOGERR("HPEU load failure\n");
    }
    track_kfree(buf, align_sz, M_OTHER);
    return ret;
}

// Weafon: should be remove!!!
static int32_t _HPEU_get_idx_eu(HPEUTab_t * hpeutab, sector64 pos_eu)
{
    int32_t id_dev;
    if (!pbno_lfsm2bdev(pos_eu, 0, &id_dev)) {
        return -1;
    }

    return (id_dev % hpeutab->cn_disk);
}

// return 1 means curr is younger (latest)
// return 0 means curr is invalid data
// return <0 means error
int32_t HPEU_is_younger(HPEUTab_t * hpeutab, sector64 prev, sector64 curr,
                        sector64 lpn)
{
    struct EUProperty *eu_prev, *eu_curr;
    sector64 birthday_prev, birthday_curr, ft_prev, ft_curr;
    int32_t pg_prev, pg_curr;

    ft_prev = -1;
    ft_curr = -1;
    pg_prev = -1;
    pg_curr = -1;

    eu_prev = eu_find(prev);
    eu_curr = eu_find(curr);

    // weafon: if prev eu log has been flush out from updatelog and committed to hpeutab and bmt
    // then we cannot get its id_hpeu during recovery or said before read hpeutab
    // -1 implies the eu has been committed
    // so prev must be invalid and curr must be younger (new)
    if (eu_prev->id_hpeu == -1) {
        return 1;
    }

    if ((birthday_prev = HPEU_get_birthday(hpeutab, eu_prev)) == ~0) {
        goto l_err;
    }

    if ((birthday_curr = HPEU_get_birthday(hpeutab, eu_curr)) == ~0) {
        goto l_err;
    }

    if (birthday_prev < birthday_curr) {
        return 1;
    } else if (birthday_prev > birthday_curr) {
        goto l_older;
    }
    // means birthday_prev == birthday_curr
    ft_prev = HPEU_get_frontier(prev);
    ft_curr = HPEU_get_frontier(curr);

    if (ft_prev < ft_curr) {
        return 1;
    } else if (ft_prev > ft_curr) {
        goto l_older;
    }
    // means frontier equally
    if ((pg_prev = PGROUP_ID(eu_prev->disk_num)) < (pg_curr =
                                                    PGROUP_ID(eu_curr->
                                                              disk_num))) {
        return 1;
    } else {
        goto l_older;
    }

  l_err:
//    printk("%s: wrong result birthday_prev %llx birthday_curr %llx\n",__FUNCTION__,birthday_prev,birthday_curr);
//    printk("%s: lpn %lld prev pbno %lld [birthday %lld ft %lld pg %d] vs. curr pbno %lld [birthday %lld ft %lld pg %d]\n",
//        __FUNCTION__,prev, birthday_prev,ft_prev,pg_prev, curr, birthday_curr,ft_curr,pg_curr);

    return -EINVAL;
  l_older:
//    printk("%s: lpn %lld prev %lld [birthday 0x%llx ft %lld pg %d] older curr %lld [birthday 0x%llx ft %lld pg %d]\n",
//        __FUNCTION__,lpn, prev, birthday_prev,ft_prev,pg_prev, curr, birthday_curr,ft_curr,pg_curr);
    return 0;
}

bool HPEU_force_update(HPEUTab_t * hpeutab, int32_t id_hpeu, sector64 pos_eu,
                       sector64 birthday)
{
    HPEUTabItem_t *pItem;
    unsigned long flag;
    int32_t id_eu, idx_in_hpeu;
    bool ret;

    idx_in_hpeu = _HPEU_get_idx_eu(hpeutab, pos_eu);

    if (idx_in_hpeu == -1) {
        LOGERR("bad idx_in_hpeu %d addr %llu\n", idx_in_hpeu, pos_eu);
        return false;
    }

    spin_lock_irqsave(&hpeutab->lock, flag);
    id_eu = pos_eu >> (SECTORS_PER_SEU_ORDER);
    if (NULL == (pItem = HPEU_GetTabItem(hpeutab, id_hpeu))) {
        ret = false;
        goto l_end;
    }

    if (pItem->cn_added == -1) {
        pItem->cn_added = 0;
        if (birthday == 0) {
            LOGWARN(" id_hpeu %d pos_eu %lld birthday=0?\n", id_hpeu, pos_eu);
        }
        pItem->birthday = birthday;
    }

    if (NULL == (pItem->ar_peu[idx_in_hpeu])) {
        pItem->ar_peu[idx_in_hpeu] = pbno_eu_map[id_eu];
        pItem->cn_added++;
    } else if (pItem->ar_peu[idx_in_hpeu] != pbno_eu_map[id_eu]) {
        LOGINERR("hpeutab ar_eu(%p) %llu is no equal to eu(%p) %llu\n",
                 pItem->ar_peu[idx_in_hpeu],
                 pItem->ar_peu[idx_in_hpeu]->start_pos, pbno_eu_map[id_eu],
                 pbno_eu_map[id_eu]->start_pos);
        ret = false;
        goto l_end;
    }

    dprintk("%s eu %llu id_hpeu %d (%d) cn_add %d birth %llx\n",
            __FUNCTION__, pos_eu, id_hpeu, idx_in_hpeu, pItem->cn_added,
            pItem->birthday);
    ret = true;

  l_end:
    spin_unlock_irqrestore(&hpeutab->lock, flag);
    return ret;
}

void HPEU_sync_all_eus(HPEUTab_t * hpeutab)
{
    HPEUTabItem_t *pItem;
    unsigned long flag;
    int32_t i, j;

    spin_lock_irqsave(&hpeutab->lock, flag);
    for (i = 0; i < hpeutab->cn_total; i++) {
        pItem = HPEU_GetTabItem(hpeutab, i);
        if (pItem->cn_added <= 0) {
            continue;
        }

        for (j = 0; j < hpeutab->cn_disk; j++) {
            pItem->ar_peu[j]->id_hpeu = i;
        }
    }
    spin_unlock_irqrestore(&hpeutab->lock, flag);

    return;
}

static struct EUProperty *_HPEU_detach(HPEUTab_t * hpeutab, int32_t id_hpeu,
                                       int32_t idx_fail)
{
    struct EUProperty *peu_ret;
    HPEUTabItem_t *pItem;
    unsigned long flag;

    peu_ret = NULL;
    spin_lock_irqsave(&hpeutab->lock, flag);
    if (NULL == (pItem = HPEU_GetTabItem(hpeutab, id_hpeu))) {
        goto l_end;
    }

    if (pItem->cn_added <= 0) {
        goto l_end;
    }

    if (pItem->ar_peu[idx_fail]) {
        pItem->ar_peu[idx_fail]->id_hpeu = -1;
    }

    peu_ret = pItem->ar_peu[idx_fail];
    pItem->ar_peu[idx_fail] = NULL;
    pItem->cn_added--;
  l_end:
    spin_unlock_irqrestore(&hpeutab->lock, flag);
    return peu_ret;
}

// return the distance to ECC lpn.
static int32_t _HPEU_find_ecc(struct lfsm_dev_struct *td,
                              EUProperty_t ** ar_peu, int32_t row,
                              struct rddcy_fail_disk *fail_disk,
                              sector64 * ar_stripe_pbno,
                              int32_t * id_fail_trunk)
{
    struct EUProperty *peu_ecc;
    int32_t len, ecc_disk, max_stripe_len, i;
    sector64 extecc_pbno;
    int32_t *map, disk, old_fail_idx;

    map = rddcy_get_stripe_map(td->stripe_map, row, td->hpeutab.cn_disk);

    len = -EINVAL;
    max_stripe_len = td->hpeutab.cn_disk;

    ecc_disk = map[max_stripe_len - 1];

    if (ecc_disk == fail_disk->idx[0]) {
        LOGERR("ECC disks is failure %d\n", ecc_disk);
        return len;
    }

    if (NULL == (peu_ecc = ar_peu[ecc_disk])) {
        LOGERR("NULL ecc peu in hpeu unexpected cases\n");
        return -EDOM;
    }

    len = xorbmp_get(peu_ecc, row) ? max_stripe_len : 0;

    if (len == 0) {             // Find Ext ecc
        len =
            ExtEcc_get_stripe_size(td, ar_peu[0]->start_pos, row, &extecc_pbno,
                                   fail_disk);
    }

    old_fail_idx = *id_fail_trunk;
    for (i = 0; i < len; i++) {
        if (i == (len - 1) && len != max_stripe_len) {  // Use ext ecc
            ar_stripe_pbno[i] = extecc_pbno;
        } else {
            disk = map[i];
            if (disk == fail_disk->idx[0]) {
                ar_stripe_pbno[i] = PBNO_SPECIAL_BAD_PAGE;
                if (old_fail_idx == disk) {
                    *id_fail_trunk = i;
                }
            } else {
                ar_stripe_pbno[i] =
                    ar_peu[disk]->start_pos +
                    (row << SECTORS_PER_FLASH_PAGE_ORDER);
            }
        }
    }

    return len;
}

int32_t HPEU_get_stripe_pbnos(HPEUTab_t * hpeutab, int32_t id_hpeu,
                              sector64 pbno, sector64 * ar_stripe_pbno,
                              int32_t * id_fail_trunk)
{
    HPEUTabItem_t *pItem;
    struct EUProperty *peu;
    int32_t offset, pg_id;
    struct rddcy_fail_disk fail_disk;
    struct lfsm_dev_struct *td;

    if ((id_hpeu < 0) || (id_hpeu > hpeutab->cn_total)) {
        LOGERR(" invalid hpeuid=%d\n", id_hpeu);
        return -EINVAL;
    }

    if (NULL == (pItem = HPEU_GetTabItem(hpeutab, id_hpeu))) {
        return -EINVAL;
    }

    if (NULL == (peu = eu_find(pbno))) {
        return -EINVAL;
    }

    td = hpeutab->td;
    offset = (int32_t) (pbno - peu->start_pos) >> SECTORS_PER_FLASH_PAGE_ORDER;
    *id_fail_trunk = peu->disk_num % td->hpeutab.cn_disk;
    //idx_eu = peu->disk_num % hpeutab->cn_disk;
    pg_id = HPEU_get_pgroup_id(hpeutab, id_hpeu);
    rddcy_fail_ids_to_disk(hpeutab->cn_disk,
                           rddcy_search_failslot_by_pg_id(&grp_ssds, pg_id),
                           &fail_disk);
    return _HPEU_find_ecc(td, pItem->ar_peu, offset, &fail_disk, ar_stripe_pbno,
                          id_fail_trunk);
}

//td->active_stripe[temp].id_hpeu=eu_new->id_hpeu;
bool HPEU_smartadd_eu(HPEUTab_t * hpeutab, struct EUProperty *eu_new,
                      int32_t * ref_id_hpeu)
{
    int32_t id_hpeu;

    if ((eu_new->disk_num % hpeutab->cn_disk) == 0) {
        if ((id_hpeu = _HPEU_alloc_and_add(hpeutab, eu_new)) == -1) {
            return false;
        }

        *ref_id_hpeu = id_hpeu;
        eu_new->id_hpeu = id_hpeu;
    } else {
        eu_new->id_hpeu = *ref_id_hpeu;
        if (!_HPEU_add_eu(hpeutab, eu_new->id_hpeu, eu_new)) {
            LOGERR(" fail eu_new %p disk id %d\n", eu_new, eu_new->disk_num);
            return false;
        }
    }

    return true;
}

enum ehpeu_state HPEU_get_state(HPEUTab_t * hpeutab, sector64 pbno)
{
    HPEUTabItem_t *pItem;
    struct EUProperty *eu;
    unsigned long flag;
    enum ehpeu_state ret;

    eu = pbno_eu_map[(pbno - eu_log_start) >> EU_BIT_SIZE];
    if (eu->id_hpeu == -1) {
        LOGINFO("InternalError@%s: eu %p pbno=%llu id_hpeu =-1\n", __FUNCTION__,
                eu, pbno);
        dump_stack();
        return damege;
    }

    spin_lock_irqsave(&hpeutab->lock, flag);
    if (NULL == (pItem = HPEU_GetTabItem(hpeutab, eu->id_hpeu))) {
        ret = -1;
    } else {
        ret = pItem->state;
    }
    spin_unlock_irqrestore(&hpeutab->lock, flag);

    if ((ret != good) && (ret != damege)) {
        LOGERR("unexpected state %d pbno %lld eu %p id_hpeu=%d\n", ret, pbno,
               eu, eu->id_hpeu);
    }

    return ret;
}

int32_t HPEU_set_state(HPEUTab_t * hpeutab, int32_t id_hpeu,
                       enum ehpeu_state state)
{
    HPEUTabItem_t *pItem;
    unsigned long flag;

    spin_lock_irqsave(&hpeutab->lock, flag);
    if (NULL == (pItem = HPEU_GetTabItem(hpeutab, id_hpeu))) {
        goto l_end;
    }

    if (pItem->state != state) {
        pItem->state = state;
        if (state == damege) {
            atomic_inc(&hpeutab->cn_damege);
        } else if (state == good) {
            //atomic_set(&pItem->cn_unfinish_fpage, 0);
            atomic_dec(&hpeutab->cn_damege);
        }
    }
    LOGINFO(" set id_hpeu=%d to %d\n", id_hpeu, state);
  l_end:
    spin_unlock_irqrestore(&hpeutab->lock, flag);
    return 0;
}

int32_t HPEU_get_pgroup_id(HPEUTab_t * hpeutab, int32_t id_hpeu)
{
    HPEUTabItem_t *pItem;
    int32_t id_pgroup;

    if ((id_hpeu < 0) || (id_hpeu > hpeutab->cn_total)) {
        LOGERR(" invalid hpeuid=%d\n", id_hpeu);
        return -EINVAL;
    }

    if (NULL == (pItem = HPEU_GetTabItem(hpeutab, id_hpeu))) {
        return -EINVAL;
    }

    if (pItem->cn_added <= 0) {
        LOGERR("cannot get from zeroeus hpeuid=%d %d\n", id_hpeu,
               pItem->cn_added);
        return -EINVAL;
    }

    id_pgroup = PGROUP_ID(pItem->ar_peu[0]->disk_num);
    if ((id_pgroup < 0) || (id_pgroup >= hpeutab->td->cn_pgroup)) {
        LOGERR("got a invlaid pgroup id from eu %p\n", pItem->ar_peu[0]);
        return -ENODEV;
    }

    return id_pgroup;
}

int32_t HPEU_set_allstate(HPEUTab_t * hpeutab, int32_t id_faildisk,
                          enum ehpeu_state state)
{
    HPEUTabItem_t *pItem;
    unsigned long flag;
    int32_t i, id_group, ret;

    ret = -EINVAL;

    spin_lock_irqsave(&hpeutab->lock, flag);
    for (i = 0; i < hpeutab->cn_total; i++) {
        if (NULL == (pItem = HPEU_GetTabItem(hpeutab, i))) {
            goto l_fail;
        }

        if (pItem->cn_added == -1) {
            continue;
        }

        if ((id_group = HPEU_get_pgroup_id(hpeutab, i)) < 0) {
            goto l_fail;
        }

        if (id_group == PGROUP_ID(id_faildisk)) {
            if (pItem->state == state)
                continue;
            pItem->state = state;
            if (state == damege) {
                atomic_inc(&hpeutab->cn_damege);
            } else if (state == good) {
                atomic_dec(&hpeutab->cn_damege);
            }
        }
    }
    ret = 0;

  l_fail:
    spin_unlock_irqrestore(&hpeutab->lock, flag);
    return ret;
}

//return true: insert to freelist, false: not insert to freelist
static bool _HPEU_turnfixed_insert_FreeList(struct HListGC *h,
                                            struct EUProperty *peu,
                                            lfsm_dev_t * td)
{
    int32_t ret;

    if (peu->erase_count == EU_UNUSABLE) {
        return true;
    }
    // normal eu, putback to free_list
    if ((ret = FreeList_insert(h, peu)) == 0) {
        return true;
    }
    // fail path -EINVAL & -EPERM
    return false;
}

bool HPEU_turn_fixed(HPEUTab_t * phpeutab, int32_t id_hpeu, int32_t idx_fail,
                     struct EUProperty *eu_new)
{
    struct HListGC *h;
    struct EUProperty *peu;
    struct lfsm_stripe *psp;
    HPEUTabItem_t *pItem;
    int32_t i, t, ret, id_src_list;

    id_src_list = 0;
    if (NULL == (peu = _HPEU_detach(phpeutab, id_hpeu, idx_fail))) {
        LOGERR(" Got a null eu from hpeu %d idx_fail=%d\n", id_hpeu, idx_fail);
        return false;
    }

    h = gc[peu->disk_num];

    if ((t = eu_is_active(peu)) < 0) {  // is not active
        id_src_list = ret = gc_remove_eu_from_lists(h, peu);

        if (ret < 0) {
            LFSM_FORCE_RUN(goto l_insert);
            return false;
        }
    } else {
        LOGINFO
            ("fixing active eu old: %p %lld cn %d vsec %d new: %p %lld cn %d vsec %d t=%d\n",
             peu, peu->start_pos, peu->log_frontier,
             atomic_read(&peu->eu_valid_num), eu_new, eu_new->start_pos,
             eu_new->log_frontier, atomic_read(&eu_new->eu_valid_num), t);

        mdnode_free(&peu->act_metadata, phpeutab->td->pMdpool);
        if (eu_new->log_frontier > peu->log_frontier) { //ecc will be generate during rebuild if ecc does not be written successfuly before crash
            psp =
                &(phpeutab->td->ar_pgroup[PGROUP_ID(eu_new->disk_num)].
                  stripe[t]);
            mutex_lock(&psp->lock);
            stripe_commit_disk_select(psp);
            mutex_unlock(&psp->lock);
        }
    }

  l_insert:
    LOGINFO("rescue end old eu %p %llu new eu %p %llu\n",
            peu, peu->start_pos, eu_new, eu_new->start_pos);
    LFSM_RMUTEX_LOCK(&h->whole_lock);
    if (!_HPEU_turnfixed_insert_FreeList(h, peu, phpeutab->td)) {
        goto l_unlock_fail;
    }

    if (!_HPEU_add_eu(phpeutab, id_hpeu, eu_new)) {
        goto l_unlock_fail;
    }

    if (t >= 0) {
//        printf("Info@%s: Insert active eu t=%d diskid=%d  frontier=%ld\n",__FUNCTION__,t,eu_new->disk_num,eu_new->log_frontier);
        h->active_eus[t] = eu_new;
    } else {                    // put the new eu to original peu's place
        if (id_src_list == 1) { // in hlist
            HList_insert(h, eu_new);
        } else if (id_src_list == 0) {
            LVP_heap_insert(h, eu_new);
        } else {
            LOGERR("Wrong src_list_id, src_list_id %d\n", id_src_list);
            goto l_unlock_fail;
        }
    }

//    eu_new->id_hpeu = id_hpeu;        // move outside
    LFSM_RMUTEX_UNLOCK(&h->whole_lock);

//    printk("%s: hpeu %d ",__FUNCTION__,id_hpeu);
//    for(i=0;i<DISK_PER_HPEU;i++)
//        printk("%d ",phpeutab->ar_item[id_hpeu].ar_peu[i]->log_frontier);
//    printk("\n");
//    HPEU_set_state(phpeutab, id_hpeu, good);
    if (NULL == (pItem = HPEU_GetTabItem(phpeutab, id_hpeu))) {
        return false;
    }

    for (i = 0; i < phpeutab->cn_disk; i++) {
        xorbmp_free(&pItem->ar_peu[i]->xorbmp);
    }

    return true;
  l_unlock_fail:
    LFSM_RMUTEX_UNLOCK(&h->whole_lock);
    return false;

}

sector64 HPEU_get_birthday(HPEUTab_t * hpeutab, struct EUProperty *peu)
{
    HPEUTabItem_t *pItem;

    if ((peu->id_hpeu < 0) || (peu->id_hpeu >= hpeutab->cn_total)) {
        LOGERR(" peu %lld id_hpeu %d\n", peu->start_pos, peu->id_hpeu);
//        dump_stack();
        return ~0;
    }

    if (NULL == (pItem = HPEU_GetTabItem(hpeutab, peu->id_hpeu))) {
        return ~0;
    }

    return pItem->birthday;
}

// weafon: since we cannot eu within a update log reset
// one-line function is because I don't want to access hpeutab->pssl->next from UpdateLog_hook()

void HPEU_change_next(HPEUTab_t * hpeutab)
{
    SNextPrepare_renew(&hpeutab->pssl.next, EU_HPEU);
}

int32_t HPEU_eunext_init(HPEUTab_t * hpeutab)
{
    return SNextPrepare_alloc(&hpeutab->pssl.next, EU_HPEU);
}

static bool _hpeu_get_lpn(sector64 ** p_metadata, int32_t idx,
                          sector64 * ret_lpn, int32_t cn_ssds_per_hpeu)
{
    if ((idx < 0)
        || (idx >=
            (cn_ssds_per_hpeu *
             (FPAGE_PER_SEU - METADATA_SIZE_IN_FLASH_PAGES)))) {
        LOGERR(" invalid idx=%d (<0 or >%ld)\n", idx,
               (cn_ssds_per_hpeu *
                (FPAGE_PER_SEU - METADATA_SIZE_IN_FLASH_PAGES)));
        return false;
    }

    *ret_lpn = p_metadata[idx % cn_ssds_per_hpeu][idx / cn_ssds_per_hpeu];
    return true;
}

int32_t hpeu_find_ecc_lpn(sector64 ** p_metadata, int32_t idx_curr,
                          int32_t director, int32_t * ret_len,
                          int32_t cn_ssds_per_hpeu)
{
    sector64 lpn;
    int32_t i, idx_next;

    idx_next = -1;
    *ret_len = 0;
    for (i = 1; i < cn_ssds_per_hpeu; i++) {
        idx_next = idx_curr + (i * director);
        if (idx_next == -1) {
            return -1;
        }

        if (!_hpeu_get_lpn(p_metadata, idx_next, &lpn, cn_ssds_per_hpeu)) {
            return -2;
        }

        if (SPAREAREA_IS_EMPTY(lpn)) {
//            printk("%s: supposed not to go here\n",__FUNCTION__);
            if ((i == 1) && (director == 1)) {
                return idx_next;
            }
            LOGINERR("should not get a empty lpn %d %d %d\n", director, i,
                     idx_curr);
            return -2;
        }

        if (RDDCY_IS_ECCLPN(lpn)) {
            *ret_len = RDDCY_GET_STRIPE_SIZE(lpn);
            if ((*ret_len) > cn_ssds_per_hpeu) {
                LOGERR
                    (" invalid stripe size %d dir %d i=%d idx_curr=%d lpn=%llx\n",
                     *ret_len, director, i, idx_curr, lpn);
            }
            return idx_next;
        }
    }

    if (director) {
        *ret_len = cn_ssds_per_hpeu;
    }

    return idx_curr + (i * director);;
}

static int32_t _hpeu_force_get_min_frontier(sector64 ** p_metadata,
                                            int32_t idx_fail,
                                            int32_t cn_ssds_per_hpeu)
{
    int32_t i, j, min_idx, min_frontier, ret;

    min_idx = 0;
    min_frontier = FPAGE_PER_SEU - METADATA_SIZE_IN_FLASH_PAGES;
    ret = min_frontier;

    /// Tifa: this section is looking for min frontier and min idx,
    /// the condition is left-side frontier should be the same or larger than right-side
    ///           _______     _________
    /// like ____|         or
    for (i = cn_ssds_per_hpeu - 1; i >= 0; i--) {   //must backward for loop
        if (i == idx_fail) {
            continue;
        }

        for (j = 0; j < FPAGE_PER_SEU - METADATA_SIZE_IN_FLASH_PAGES; j++) {
            if (SPAREAREA_IS_EMPTY(p_metadata[i][j])) {
                break;
            }
        }

        if (j <= min_frontier) {
            min_frontier = j;
            min_idx = i;
        }
    }

    if (idx_fail > min_idx) {
        ret = min_frontier;
    } else if (idx_fail < min_idx) {
        ret = min_frontier + 1;
    }

    return ret;
}

int32_t hpeu_get_eu_frontier(sector64 ** p_metadata, int32_t idx_fail,
                             int32_t cn_ssds_per_hpeu)
{
    int32_t row_next, row_prev, row_max, id_next, id_prev;

    id_next = (idx_fail + 1) % cn_ssds_per_hpeu;
    id_prev = (idx_fail - 1 + cn_ssds_per_hpeu) % cn_ssds_per_hpeu;

    for (row_next = 0; row_next < FPAGE_PER_SEU - METADATA_SIZE_IN_FLASH_PAGES;
         row_next++) {
        if (SPAREAREA_IS_EMPTY(p_metadata[id_next][row_next])) {
            break;
        }
    }

    for (row_prev = 0; row_prev < FPAGE_PER_SEU - METADATA_SIZE_IN_FLASH_PAGES;
         row_prev++) {
        if (SPAREAREA_IS_EMPTY(p_metadata[id_prev][row_prev])) {
            break;
        }
    }

    row_max = (row_next > row_prev) ? row_next : row_prev;

    switch (row_next - row_prev) {
    case 0:
        row_prev++;
        return min(row_prev,
                   (int32_t) (FPAGE_PER_SEU - METADATA_SIZE_IN_FLASH_PAGES));

    case 1:
    case -1:
        return row_max;
    default:
        LOGERR(" unexpected case: next %d prev %d idx_fail %d\n", row_next,
               row_prev, idx_fail);
        return _hpeu_force_get_min_frontier(p_metadata, idx_fail,
                                            cn_ssds_per_hpeu);
    }
}

bool HPEU_rescue(HPEUTab_t * hpeutab)
{
    return sysEU_rescue(hpeutab->td, &hpeutab->pssl.eu[0],
                        &hpeutab->pssl.eu[1], &hpeutab->pssl.next,
                        &hpeutab->pssl.spss_lock);
}

int32_t HPEU_read_metadata(lfsm_dev_t * td, struct EUProperty **ar_peu,
                           sector64 ** p_metadata,
                           struct rddcy_fail_disk *fail_disk)
{
    devio_fn_t *ar_cb;
    int32_t i, ret, idx_skip, cn_disk;

    ret = 0;
    cn_disk = td->hpeutab.cn_disk;
    if (fail_disk)
        idx_skip = fail_disk->idx[0];
    else
        idx_skip = -1;

    ar_cb = (devio_fn_t *)
        track_kmalloc(cn_disk * sizeof(devio_fn_t), GFP_KERNEL, M_OTHER);
    if (NULL == (ar_cb)) {
        ret = -ENOMEM;
        goto l_end;
    }

    for (i = 0; i < cn_disk; i++) {
        if (i == idx_skip) {
            continue;
        }

        metabioc_asyncload(td, ar_peu[i], &ar_cb[i]);
    }

    for (i = 0; i < cn_disk; i++) {
        if (i == idx_skip) {
            continue;
        }

        if (metabioc_asyncload_bh(td, ar_peu[i], p_metadata[i], &ar_cb[i])) {
            continue;
        }

        if (!EU_spare_read
            (td, (int8_t *) (p_metadata[i]), ar_peu[i]->start_pos,
             FPAGE_PER_SEU)) {
            LOGERR(" fail to read spare for eu %lld \n", ar_peu[i]->start_pos);
            ret = -EIO;
            goto l_end;
        }
    }

  l_end:
    if (ar_cb) {
        track_kfree(ar_cb, cn_disk * sizeof(devio_fn_t), M_OTHER);
    }

    return ret;
}

int32_t hpeu_get_maxcn_data_disk(int32_t cn_ssds_per_hpeu)
{
    return (cn_ssds_per_hpeu - 1);
}

int32_t HPEU_dec_unfinish(HPEUTab_t * hpeutab, sector64 pbno_finished_write)
{
    struct EUProperty *eu;
    HPEUTabItem_t *pItem;

    if (LBNO_REDO_PAGE == pbno_finished_write) {
        LOGINFO("rec REDO_PAGE\n");
        return 0;
    }

    eu = eu_find(pbno_finished_write);
    if (NULL == eu) {
        LOGERR("the pbno %llu is wrong\n", pbno_finished_write);
        return -EINVAL;
    }

    if (eu->id_hpeu < 0)        // ExtEcc eu 
        return 0;
    pItem = HPEU_GetTabItem(hpeutab, eu->id_hpeu);
    if (NULL == pItem) {
        LOGERR("hpeu item is wrong id %d , pItem %p\n", eu->id_hpeu, pItem);
        return -ERANGE;
    }
    atomic_dec(&pItem->cn_unfinish_fpage);

    return 0;
}

int32_t HPEU_is_unfinish(HPEUTab_t * phpeutab, int32_t id_hpeu)
{
    int32_t cn_unwritten;

    cn_unwritten =
        atomic_read(&HPEU_GetTabItem(phpeutab, id_hpeu)->cn_unfinish_fpage);

    if (cn_unwritten < 0) {
        LOGERR("id_hpeu %d unwritten %d < 0 !!\n", id_hpeu, cn_unwritten);
        return -ENOMEM;
    }

    return cn_unwritten;
}

int HPEU_dump(char *buffer, char **buffer_location, off_t offset,
              int buffer_length, int *eof, void *data)
{
    int32_t len, i, sz_record, sz_title, sz_head, idx, buf_used, record_offset;
    struct HPEUTab *phpeutab;
    HPEUTabItem_t *pItem;

    len = 0;
    sz_record = 100;            // 33= size of "%5d %5d %20s\n"
    sz_title = 100;
    sz_head = 100;
    buf_used = 0;
    phpeutab = &lfsmdev.hpeutab;

    if (offset >= ((phpeutab->cn_total * sz_record) + sz_head + sz_title)) {
        *eof = 1;
        return 0;
    }

    if (offset < sz_head + sz_title) {
        memset(buffer, ' ', sz_head + sz_title);

        sprintf(buffer, "Total CN hpeu %d  used %d curr %d\n ID Status added",
                phpeutab->cn_total, phpeutab->cn_used, phpeutab->curri);
        buffer[sz_head + sz_title - 1] = '\n';
        *buffer_location = buffer + offset;
        len = sz_head + sz_title - offset;
        return len;
    }

    memset(buffer, ' ', sz_record);

    record_offset = offset - sz_head - sz_title;
    idx = record_offset / sz_record;
    pItem = HPEU_GetTabItem(phpeutab, idx);

    buf_used =
        sprintf(buffer, "%05d %d %d  ", idx, pItem->state, pItem->cn_added);
    for (i = 0; i < pItem->cn_added; i++) {
        buf_used += sprintf(buffer + buf_used, "%p  ", pItem->ar_peu[i]);
    }

    buffer[sz_record - 1] = '\n';
    *buffer_location = buffer + record_offset % sz_record;
    len = sz_record - record_offset % sz_record;
    *eof = 0;
    return len;
}

static void _HPEU_reset(HPEUTab_t * hpeutab, int32_t cn_total,
                        int32_t cn_ssds_per_hpeu)
{
    HPEUTabItem_t *pItem;
    int32_t i, j;

    hpeutab->td = 0;
    hpeutab->cn_total = cn_total;
    hpeutab->cn_used = 0;
    hpeutab->curri = -1;
    hpeutab->cn_disk = cn_ssds_per_hpeu;
    atomic_set(&hpeutab->cn_damege, 0);
    spin_lock_init(&hpeutab->lock);
    for (i = 0; i < cn_total; i++) {
        pItem = HPEU_GetTabItem(hpeutab, i);
        pItem->cn_added = -1;
        pItem->state = good;
        pItem->birthday = 0;
        atomic_set(&pItem->cn_unfinish_fpage, 0);
        for (j = 0; j < cn_ssds_per_hpeu; j++) {
            pItem->ar_peu[j] = NULL;
        }
    }
}

bool HPEU_reset_for_scale_up(HPEUTab_t * hpeutab, HPEUTabItem_t * ar_item_new,
                             HPEUTabItem_t * ar_item_orig,
                             int32_t cn_total_hpeu, int32_t cn_total_hpeu_orig,
                             int32_t cn_ssds_per_hpeu)
{
    int i, j;
    HPEUTabItem_t *pItem = NULL;

    memcpy(ar_item_new, ar_item_orig,
           _HPEU_GetTabItem_size(hpeutab) * cn_total_hpeu_orig);
/*	
	for (i = 0; i < cn_total_hpeu_orig;i++) {
		pItem = ar_item_new + (i*_HPEU_GetTabItem_size(hpeutab));
		pItem_orig = ar_item_orig + (i*_HPEU_GetTabItem_size(hpeutab));
        pItem->cn_added = pItem_orig->cn_added;
        pItem->state = pItem_orig->state;
        pItem->birthday = pItem_orig->birthday;
		cn_unfinish_fpage = atomic_read(&pItem_orig->cn_unfinish_fpage);
        atomic_set(&pItem->cn_unfinish_fpage, cn_unfinish_fpage);
        for (j = 0; j < cn_ssds_per_hpeu; j++) {
            pItem->ar_peu[j] = pItem_orig->ar_peu[j];
        }
    }
*/
    for (i = cn_total_hpeu_orig; i < cn_total_hpeu; i++) {
        pItem =
            (HPEUTabItem_t *) ((int8_t *) ar_item_new +
                               (i * _HPEU_GetTabItem_size(hpeutab)));
        pItem->cn_added = -1;
        pItem->state = good;
        pItem->birthday = 0;
        atomic_set(&pItem->cn_unfinish_fpage, 0);
        for (j = 0; j < cn_ssds_per_hpeu; j++) {
            pItem->ar_peu[j] = NULL;
        }
    }

    return true;
}

int32_t HPEU_re_init(HPEUTab_t * hpeutab, int32_t cn_total_hpeu, lfsm_dev_t * td, int32_t cn_ssds_per_hpeu) //ding: notice ar_item size!!
{
    int32_t ret = 0;
//  int32_t i,j;
    int32_t sz;
    int32_t cn_total_hpeu_orig;
    HPEUTabItem_t *ar_item_new = NULL, *ar_item_orig;

    cn_total_hpeu_orig = hpeutab->cn_total;
    hpeutab->cn_total = cn_total_hpeu;

    LOGINFO("cn_total_hpeu_orig=%d, cn_total_hpeu=%d\n", cn_total_hpeu_orig,
            cn_total_hpeu);

    if (IS_ERR_OR_NULL
        (ar_item_new =
         (HPEUTabItem_t *) track_kmalloc(_HPEU_GetTabItem_size(hpeutab) *
                                         cn_total_hpeu, GFP_KERNEL, M_OTHER))) {
        LOGERR("memory alloc HPEUTabItem failure\n");
        return -ENOMEM;
    }

    ar_item_orig = (HPEUTabItem_t *) hpeutab->ar_item;
    HPEU_reset_for_scale_up(hpeutab, ar_item_new, ar_item_orig, cn_total_hpeu,
                            cn_total_hpeu_orig, cn_ssds_per_hpeu);

    hpeutab->ar_item = ar_item_new;
    track_kfree(ar_item_orig,
                _HPEU_GetTabItem_size(hpeutab) * cn_total_hpeu_orig, M_OTHER);

    sz = cn_total_hpeu * cn_ssds_per_hpeu * sizeof(int32_t);
    if (PssLog_reinit
        (&hpeutab->pssl, sz, LFSM_CEIL(sz + sizeof(int32_t), HARD_PAGE_SIZE)))
        ret = -EINVAL;
    else
        ret = PssLog_Reclaim(&hpeutab->pssl);

    return ret;

}

/*
 * Description: Initial hpeu table
 * Parameters: cn_total_hpeu: number of EU in a disk * number of protection group
 *             cn_ssds_per_hpeu: How many disk in a protection group
 * return: true: init fail
 *         false: init ok
 */
bool HPEU_init(HPEUTab_t * hpeutab, int32_t cn_total_hpeu, lfsm_dev_t * td, int32_t cn_ssds_per_hpeu)   //ding: notice ar_item size!!
{
    int32_t sz;
    _HPEU_reset(hpeutab, 0, cn_ssds_per_hpeu);

    if (IS_ERR_OR_NULL(hpeutab->ar_item = (HPEUTabItem_t *)
                       track_kmalloc(_HPEU_GetTabItem_size(hpeutab) *
                                     cn_total_hpeu, GFP_KERNEL, M_OTHER))) {
        LOGERR("memory alloc HPEUTabItem failure\n");
        return true;
    }

    _HPEU_reset(hpeutab, cn_total_hpeu, cn_ssds_per_hpeu);
    hpeutab->td = td;
    sz = cn_total_hpeu * cn_ssds_per_hpeu * sizeof(int32_t);
    return PssLog_Init(&hpeutab->pssl, td, sz,
                       LFSM_CEIL(sz + sizeof(int32_t), HARD_PAGE_SIZE),
                       EU_HPEU);
}

bool HPEU_destroy(HPEUTab_t * hpeutab)
{
    if (IS_ERR_OR_NULL(hpeutab)) {
        return false;
    }

    PssLog_Destroy(&hpeutab->pssl);
    if (hpeutab->ar_item) {
        track_kfree(hpeutab->ar_item,
                    _HPEU_GetTabItem_size(hpeutab) *
                    (grp_ssds.cn_sectors_per_dev >> SECTORS_PER_SEU_ORDER) *
                    hpeutab->td->cn_pgroup, M_OTHER);
    }
    _HPEU_reset(hpeutab, 0, hpeutab->cn_disk);
    return true;
}
