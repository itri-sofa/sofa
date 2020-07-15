/*
 * SPDX-License-Identifier: GPL-2.0-or-later OR Apache-2.0
 * Copyright (C) 2015-2020 Industrial Technology Research Institute
 *
 *
 *
 *  
 */
///////////////////////////////////////////////////////////////////////////////
// Overview: 
//    Provide SOFA a double-logs component. For example, when SOFA plan to log something to
//    flash memory, SOFA simply calls PssLog_Append() to pass the information to log.
//     Then, PssLog_Append() will write to TWO on-flash logs, which is pre-allocated by PssLog_Init()
//    Surely, SOFA should give the size of log entry whenever call PssLog_Init.
//    PssLog can accept a log entry which size is larger one flash page
// 
//    Then, whenever the log is full, PssLog will call PssLog_Reclaim() to get a new SEU
//  and register the location of the new SEU to dedicated log by dedicated_map_logging()
//  By the registration, after rebooting, PssLog can find out the latest used SEUs.
//
//    After rebooting, PssLog_Getlast() help SOFA to get the latest entry logged on the double-log.
//  By PssLog_Getlast, SOFA don't need to worry where is the latest entry. Unfortunately, the searching
//     costs too-many IOs so SOFA does not use the function actually.
//
//    For now, the componenet PssLog is used to help SOFA to log the HPEU table. Whevever SOFA plans
//    to save the whole HPEU table, SOFA calls HPEU_save() which calls PssLog_append() to save the whole table
//  Next time, SOFA call HPEU_load() to load the latest HPEU table, where HPEU_load() locates the
//    latest SEU used by PssLog and the index of the logging entry by reading dedicated map. Then,
//    HPEU_load() call PssLog_Assign() to init PssLog and get the latest HPEU table logged in the last entry
////////////////////////////////////////////////////////////////////////////////
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
#include "persistlog.h"
#include "EU_access.h"
#include "devio.h"
#include "system_metadata/Dedicated_map.h"
#include "spare_bmt_ul.h"
#include "persistlog_private.h"

int32_t PssLog_get_entry_size(SPssLog_t * pssl)
{
    return pssl->sz_alloc;
}

/*
 * Description: check whether have enough space for write record
 * return: true : space is enough
 *         false : space not enough
 */
static bool PssLog_HasSpace(SPssLog_t * pssl)
{
    if (NULL == pssl->eu[0]) {
        return false;
    }

    if ((pssl->eu[0]->log_frontier +
         LFSM_CEIL_DIV(pssl->sz_alloc,
                       HARD_PAGE_SIZE) * SECTORS_PER_HARD_PAGE) >
        pssl->eu[0]->eu_size) {
        return false;
    } else {
        return true;
    }
}

/*
 * Description: reclaim old EU and alloc new EU for persist log
 * return: 0 : reclaim ok
 *         non 0 : reclaim fail
 */
int32_t PssLog_Reclaim(SPssLog_t * pssl)
{
#if PSSLOG_CN_EU != 2
#error "Sorry, SNextPrepare does not support the case of eu > 2"
#endif
    struct EUProperty *ar_eu_temp[PSSLOG_CN_EU];
    int32_t i;

    LOGINFO("Persist log eu reclaim called\n");

    for (i = 0; i < PSSLOG_CN_EU; i++) {
        ar_eu_temp[i] = pssl->eu[i];
    }

    SNextPrepare_get_new(&pssl->next, &pssl->eu[0], &pssl->eu[1],
                         pssl->eu_usage);

    if (dedicated_map_logging(pssl->td)) {
        LOGERR("pss log change EU fail write to ddmap\n");
        goto l_end;
    }

    SNextPrepare_put_old(&pssl->next, ar_eu_temp[0], ar_eu_temp[1]);

    return 0;

  l_end:
    SNextPrepare_rollback(&pssl->next, pssl->eu[0], pssl->eu[1]);
    for (i = 0; i < PSSLOG_CN_EU; i++) {
        pssl->eu[i] = ar_eu_temp[i];
    }

    return -EIO;
}

static void PssLog_SetCheckSum(SPssLog_t * pssl, int8_t * buf)
{
    int32_t i;
    uint32_t checksum;

    checksum = 0;
    for (i = 0; i < pssl->sz_real / sizeof(int32_t); i++) {
        checksum ^= ((uint32_t *) buf)[i];
    }
    checksum ^= PSSLOG_CHECKSUM;
    *(uint32_t *) (buf + pssl->sz_alloc - sizeof(int32_t)) = checksum;
}

/*
 * Description: append log to persist log
 * Parameters: buf : log entry
 *             target: indicate which EU to save
 *
 *             target & PSSLOG_EUMAIN_MASK : eu_main
 *             target & PSSLOG_EUBACKUP_MASK : eu_backup
 *
 * return: true : append log fail
 *         fail : append log ok
 */
static bool PssLog_AppendA(SPssLog_t * pssl, int8_t * buf, int32_t target)
{
    struct EUProperty *eu;
    int32_t cn_sectors, cn_hpage;
    bool ret, ret_main, ret_backup;

    cn_sectors = LFSM_CEIL_DIV(pssl->sz_alloc, SECTOR_SIZE);
    cn_hpage = LFSM_CEIL_DIV(pssl->sz_alloc, HARD_PAGE_SIZE);
    PssLog_SetCheckSum(pssl, buf);

    ret = false;
    ret_main = false;
    ret_backup = false;

    if (target & PSSLOG_ID2MASK(PSSLOG_ID_EUMAIN)) {
        eu = pssl->eu[PSSLOG_ID_EUMAIN];
        if (devio_write_pages_safe
            (eu->log_frontier + eu->start_pos, buf, NULL, cn_hpage)) {
            LOGERR("append log to persist log main eu fail\n");
            ret_main = true;
        }
        eu->log_frontier += cn_sectors;
    } else {
        ret_main = true;
    }

    if (target & PSSLOG_ID2MASK(PSSLOG_ID_EUBACKUP)) {
        eu = pssl->eu[PSSLOG_ID_EUBACKUP];
        if (devio_write_pages_safe
            (eu->log_frontier + eu->start_pos, buf, NULL, cn_hpage)) {
            LOGERR("append log to persist log main eu fail\n");
            ret_backup = true;
        }

        eu->log_frontier += cn_sectors;
    } else {
        ret_backup = true;
    }

    if (ret_main && ret_backup) {
        ret = true;
    }

    return ret;
}

/*
 * Description: API for log information to persist log
 * Parameter:   buf: information to log
 * return   :   true : fail
 *          :   false: ok
  */
bool PssLog_Append(SPssLog_t * pssl, int8_t * buf)
{
    bool ret;

    mutex_lock(&pssl->spss_lock);
    if (!PssLog_HasSpace(pssl)) {
        // under the new version of using SNextPrepare, weafon cannot do "even after reclaim, table must have been written down"
        pssl->cn_cycle++;

        if (PssLog_Reclaim(pssl)) {
            ret = true;
            LOGERR("reclaim and assign new EU fail when append pss log\n");
            goto l_end;
        }
    }

    ret = PssLog_AppendA(pssl, buf,
                         PSSLOG_ID2MASK(PSSLOG_ID_EUMAIN) |
                         PSSLOG_ID2MASK(PSSLOG_ID_EUBACKUP));

  l_end:
    mutex_unlock(&pssl->spss_lock);
    return ret;
}

static bool PssLog_VerifyChecksum(SPssLog_t * pssl, int8_t * buf)
{
    int32_t i;
    uint32_t checksum;

    checksum = 0;
    for (i = 0; i < pssl->sz_real / sizeof(int32_t); i++) {
        checksum ^= ((uint32_t *) buf)[i];
    }

    checksum ^= PSSLOG_CHECKSUM;
    if (*(uint32_t *) (buf + pssl->sz_alloc - sizeof(int32_t)) != checksum) {
        LOGWARN("ineq read %x chksum=%x sz_real=%d\n",
                *(uint32_t *) (buf + pssl->sz_alloc - sizeof(int32_t)),
                checksum, pssl->sz_real);
        return false;
//        hexdump(buf, pssl->sz_real);
    }

    return true;
}

static bool PssLog_Empty(SPssLog_t * pssl, int8_t * buf)
{
    int32_t i;
    for (i = 0; i < pssl->sz_real / sizeof(int32_t); i++) {
        if (((uint32_t *) buf)[i] != TAG_UNWRITE_HPAGE) {
            LOGWARN("idx %d value=%x\n", i, ((uint32_t *) buf)[i]);
            return false;
        }
    }

    if (*(uint32_t *) (buf + pssl->sz_alloc - sizeof(int32_t)) !=
        TAG_UNWRITE_HPAGE) {
        LOGWARN("idx %d value=%x\n", i,
                *(uint32_t *) (buf + pssl->sz_alloc - sizeof(int32_t)));
        return false;
    }
    return true;
}

static int32_t PssLog_read(SPssLog_t * pssl, int8_t * buf, sector64 addr,
                           int32_t cn_fpage)
{
    int32_t ret;

    if (devio_read_hpages_batch(addr, buf, cn_fpage)) {
        if (PssLog_VerifyChecksum(pssl, buf)) {
            ret = 0;
        } else {
            LOGWARN("pssl Log  read chkecsum failure sector=%lld\n", addr);
            ret = -EDOM;
        }
    } else {
        LOGWARN("pssl Log  read failure sector=%lld\n", addr);
        ret = -EIO;
    }

    return ret;
}

static int32_t PssLog_compare_main_backup_fn(int32_t ret_inside[2],
                                             int8_t * buf_inside[2],
                                             int8_t * buf, int32_t cn_byte)
{
    if (ret_inside[0] == 0 && ret_inside[1] == 0
        && memcmp(buf_inside[0], buf_inside[1], cn_byte) == 0) {
        memcpy(buf, buf_inside[0], cn_byte);
        return 0;
    } else {
        return -EIO;
    }
}

static int32_t PssLog_choose_main_backup_fn(int32_t ret_inside[2],
                                            int8_t * buf_inside[2],
                                            int8_t * buf, int32_t cn_byte)
{
    int32_t i;
    for (i = 0; i < 2; i++) {
        if (ret_inside[i] == 0) {
            memcpy(buf, buf_inside[i], cn_byte);
            return 0;
        }
    }
    return -EIO;
}

int32_t PssLog_Get_Comp(SPssLog_t * pssl, int8_t * buf, int32_t idx_next,
                        bool compare)
{
    int8_t *buf_read[2];
    sector64 idx_curr;
    int32_t ret_read[2], i, ret, cn_sectors, cn_fpage, cn_byte;

    cn_sectors = LFSM_CEIL_DIV(pssl->sz_alloc, SECTOR_SIZE);
    cn_fpage = LFSM_CEIL_DIV(pssl->sz_alloc, HARD_PAGE_SIZE);
    cn_byte = LFSM_CEIL(pssl->sz_alloc, HARD_PAGE_SIZE);
    ret = -EIO;

    if (idx_next == 0) {
        memset(buf, 0xFF, pssl->sz_alloc);
        return 0;
    }
    idx_curr = idx_next - cn_sectors;

    for (i = 0; i < 2; i++) {
        if (NULL == (buf_read[i] = track_kmalloc(cn_byte, GFP_KERNEL, M_OTHER))) {
            LOGERR("Cannot alloc memory\n");
            return -ENOMEM;
        }
    }

    mutex_lock(&pssl->spss_lock);

    for (i = 0; i < 2; i++) {
        ret_read[i] =
            PssLog_read(pssl, buf_read[i], pssl->eu[i]->start_pos + idx_curr,
                        cn_fpage);
    }

////// compare case
    if (compare) {
        ret = PssLog_compare_main_backup_fn(ret_read, buf_read, buf, cn_byte);
    } else {
        ret = PssLog_choose_main_backup_fn(ret_read, buf_read, buf, cn_byte);
    }

    mutex_unlock(&pssl->spss_lock);

    for (i = 0; i < 2; i++) {
        track_kfree(buf_read[i], cn_byte, M_OTHER);
    }

    if (ret < 0) {
        LOGERR(" failure ret=%d\n", ret);
    }
    return ret;
}

/*
 * Description: get persist log with offset idx_next
 * Parameter: idx_next  : log frontier of hpeu table (unit: sector)
 * return : 0: get ok
 *          non 0: get fail
 */
static int32_t PssLog_Get(SPssLog_t * pssl, int8_t * buf, int32_t idx_next)
{
    sector64 idx_curr;
    int32_t i, ret, cn_sectors, cn_fpage;

    cn_sectors = LFSM_CEIL_DIV(pssl->sz_alloc, SECTOR_SIZE);
    cn_fpage = LFSM_CEIL_DIV(pssl->sz_alloc, HARD_PAGE_SIZE);
    ret = -EIO;

    if (idx_next == 0) {        //TODO: How we handle system crash during change EU, already forget original EU
        memset(buf, 0xFF, pssl->sz_alloc);
        return 0;
    }
    idx_curr = idx_next - cn_sectors;

    mutex_lock(&pssl->spss_lock);

    for (i = 0; i < 2; i++) {
        if (PssLog_read(pssl, buf, pssl->eu[i]->start_pos + idx_curr, cn_fpage)
            == 0) {
            ret = 0;
            break;
        }
    }

    mutex_unlock(&pssl->spss_lock);

    if (ret < 0) {
        LOGERR("persist log get fail ret=%d\n", ret);
    }
    return ret;
}

/*
 * Description: setup EU in persist log by given position (main and backup)
 *              read persist log and return to caller by pret_buf
 * Parameter: pos_main  : start position of main EU
 *            pos_backup: start position of backup EU
 *            idx_next  : log frontier of hpeu table (unit: sector)
 * retuen : true: assign fail
 *          false: assign ok
 */
bool PssLog_Assign(SPssLog_t * pssl, sector64 pos_main, sector64 pos_backup,
                   int32_t idx_next, int8_t * pret_buf)
{
    bool ret;

    ret = true;
    if (NULL == (pssl->eu[PSSLOG_ID_EUMAIN] =
                 FreeList_delete_by_pos(pssl->td, pos_main, pssl->eu_usage))) {
        LOGERR("fail to reget a eu: %lld\n", pos_main);
        goto l_fail;
    }

    if (NULL == (pssl->eu[PSSLOG_ID_EUBACKUP] =
                 FreeList_delete_by_pos(pssl->td, pos_backup,
                                        pssl->eu_usage))) {
        LOGERR("fail to reget a eu: %lld\n", pos_backup);
        goto l_fail;

    }

    LOGINFO("pass-in next space=%d\n", idx_next);

    // memset(pret_buf, 0xFF, pssl->sz_alloc);    AN:move into PssLog_Get

    if (PssLog_Get(pssl, pret_buf, idx_next) < 0) {
        LOGERR("fail get persist log when assign eu: idx_next %u\n", idx_next);
        goto l_fail;
    }

    pssl->eu[PSSLOG_ID_EUMAIN]->log_frontier = idx_next;
    pssl->eu[PSSLOG_ID_EUBACKUP]->log_frontier = idx_next;

    ret = false;

  l_fail:
    if (ret) {
        LOGERR("assign EU to persist log fail\n");
    }
    return ret;
}

static bool binappro_init(struct sbinappro *sba, int32_t range)
{
    sba->start = 0;
    sba->end = range;
    sba->curr = (sba->start + sba->end) / 2;
    return true;
}

static bool binappro_nexti(struct sbinappro *sba, bool result)
{
    if ((result == false) && (sba->curr == sba->start)) {
        return true;
    }

    if (result == true) {
        sba->start = (sba->start + sba->end) / 2;
    } else {
        sba->end = (sba->start + sba->end) / 2;
    }

    sba->curr = (sba->start + sba->end) / 2;
    if ((result == true) && (sba->curr == sba->start)) {
        sba->curr++;
        return true;
    }
    return false;
}

/*
 * Description: return the offset position of next available space through binary search
 */
int32_t PssLog_Getlast_binarysearch(SPssLog_t * pssl, int8_t * buf)
{
    struct sbinappro sba;
    int32_t i, cn_sectors;
    bool result, ret;

    result = false;
    ret = false;
    cn_sectors = LFSM_CEIL_DIV(pssl->sz_alloc, SECTOR_SIZE);
    binappro_init(&sba, pssl->eu[PSSLOG_ID_EUMAIN]->eu_size / cn_sectors);

    mutex_lock(&pssl->spss_lock);
    do {
        i = sba.curr;
        if (devio_read_hpages
            (pssl->eu[PSSLOG_ID_EUMAIN]->start_pos + i, buf, 1)) {
            if (PssLog_Empty(pssl, buf)) {
                result = false;
                continue;
            } else if (PssLog_VerifyChecksum(pssl, buf)) {
                result = true;
                continue;
            } else {
                LOGERR("Log Main read chkecsum failure\n");
            }
        }

        if (devio_read_hpages
            (pssl->eu[PSSLOG_ID_EUBACKUP]->start_pos + i, buf, 1)) {
            if (PssLog_VerifyChecksum(pssl, buf)) {
                result = true;
                continue;
            } else {
                LOGERR("Log backup read chkecsum failure\n");
                goto l_end;
            }
        }
    } while (!binappro_nexti(&sba, result));
    ret = true;

  l_end:
    mutex_unlock(&pssl->spss_lock);
    if (!ret) {
        LOGERR("failure sba->curr=%d\n", sba.curr);
    }
    return sba.curr * cn_sectors;
}

/*
 * Description: return the offset position of next available space through linear search
 *              read from bottom to upper
 * return : 0 : ok
 *          non 0 : fail
 */
int32_t PssLog_Getlast(SPssLog_t * pssl, int8_t * buf)
{
    int32_t i, ret, cn_sectors, cn_fpage;

    ret = -1;
    cn_sectors = LFSM_CEIL_DIV(pssl->sz_alloc, SECTOR_SIZE);
    cn_fpage = LFSM_CEIL_DIV(pssl->sz_alloc, HARD_PAGE_SIZE);

    mutex_lock(&pssl->spss_lock);
    for (i = pssl->eu[PSSLOG_ID_EUMAIN]->eu_size - cn_sectors; i >= 0;
         i -= cn_sectors) {
        if (devio_read_hpages_batch
            (pssl->eu[PSSLOG_ID_EUMAIN]->start_pos + i, buf, cn_fpage)) {
            if (PssLog_Empty(pssl, buf)) {
                continue;
            }

            if (PssLog_VerifyChecksum(pssl, buf)) {
                ret = i + cn_sectors;
                goto l_end;
            } else {
                LOGERR("Log Main read chkecsum failure sector=%lld\n",
                       pssl->eu[PSSLOG_ID_EUMAIN]->start_pos + i);
            }
        }

        if (devio_read_hpages_batch
            (pssl->eu[PSSLOG_ID_EUBACKUP]->start_pos + i, buf, cn_fpage)) {
            if (PssLog_VerifyChecksum(pssl, buf)) {
                ret = i + cn_sectors;
                goto l_end;
            } else {
                LOGERR("Log backup read chkecsum failure sector=%lld\n",
                       pssl->eu[PSSLOG_ID_EUBACKUP]->start_pos + i);
                goto l_end;
            }
        }
    }
    ret = 0;                    // has not been write.

  l_end:
    mutex_unlock(&pssl->spss_lock);
    if (ret < 0) {
        LOGERR("failure ret=%d\n", ret);
    }
    return ret;
}

/*
 * Description: Alloc EU for pss log
 * Parameters: pssl: log data structure to use
 *             ar_id_dev
 *             buf
 *             NeedLog
 * return    : true : init fail
 *             false: init ok
 * NOTE: not a complete function when NeedLog = true
 */
bool PssLog_AllocEU(SPssLog_t * pssl, int32_t ar_id_dev[PSSLOG_CN_EU],
                    int8_t * buf, bool NeedLog)
{
    struct HListGC *h;
    int32_t i, cn_retry;

    cn_retry = 0;

    for (i = 0; i < PSSLOG_CN_EU; i++) {
        h = gc[ar_id_dev[i]];
        LFSM_RMUTEX_LOCK(&h->whole_lock);
        while (cn_retry < LFSM_MIN_RETRY) {
            if (NeedLog) {
                pssl->eu[i] =
                    FreeList_delete_with_log(pssl->td, h, pssl->eu_usage,
                                             false);
            } else {
                pssl->eu[i] = FreeList_delete(h, pssl->eu_usage, false);
            }

            if (NULL == buf) {
                break;
            }

            if (PssLog_AppendA(pssl, buf, PSSLOG_ID2MASK(i))) {
                if (FreeList_insert(h, pssl->eu[i]) < 0) {
                    goto l_fail_unlock;
                }
                pssl->eu[i] = 0;
                cn_retry++;
            } else {
                break;
            }
        }
        LFSM_RMUTEX_UNLOCK(&h->whole_lock);
        if (pssl->eu[i] == NULL) {
            return true;
        }
    }

    SNextPrepare_alloc(&pssl->next, pssl->eu_usage);

    return false;

  l_fail_unlock:
    LFSM_RMUTEX_UNLOCK(&h->whole_lock);

    return true;
}

/*
 * Description: initial persist log data structure
 * Parameters: pssl: log to use
 *             sz_real
 *             sz_alloc
 *             eu_usage
 * return    : true : init fail
 *             false: init ok
 */

bool PssLog_reinit(SPssLog_t * pssl, int32_t sz_real, int32_t sz_alloc)
{
    pssl->sz_real = sz_real;
    pssl->sz_alloc = sz_alloc;

    //pssl->cn_cycle = 0;

    if (pssl->sz_real > (pssl->sz_alloc - sizeof(int32_t))) {
        LOGERR("pssl: sz_real %d, sz_alloc-sizeof(int) %ld \n",
               pssl->sz_real, pssl->sz_alloc - sizeof(int32_t));
        return true;
    }

    if (pssl->sz_real % sizeof(int32_t) != 0) {
        return true;
    }

    return false;
}

bool PssLog_Init(SPssLog_t * pssl, lfsm_dev_t * td, int32_t sz_real,
                 int32_t sz_alloc, int32_t eu_usage)
{
    pssl->td = td;
    pssl->sz_real = sz_real;
    pssl->sz_alloc = sz_alloc;
    pssl->eu_usage = eu_usage;
    mutex_init(&pssl->spss_lock);
    pssl->cn_cycle = 0;

    if (pssl->sz_real > (pssl->sz_alloc - sizeof(int32_t))) {
        LOGERR("pssl: sz_real %d, sz_alloc-sizeof(int) %ld \n",
               pssl->sz_real, pssl->sz_alloc - sizeof(int32_t));
        return true;
    }

    if (pssl->sz_real % sizeof(int32_t) != 0) {
        return true;
    }

    return false;
}

/*
 * Description: free persist log data structure
 * Parameters: pssl: log data structure to free
 * return    : true : init fail
 *             false: init ok
 */
void PssLog_Destroy(SPssLog_t * pssl)
{
    if (&pssl->spss_lock) {
        mutex_destroy(&pssl->spss_lock);
    }
}
