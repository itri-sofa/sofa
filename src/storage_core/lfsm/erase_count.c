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
#include "common/rmutex.h"
#include "io_manager/mqbiobox.h"
#include "io_manager/biocbh.h"
#include "nextprepare.h"
#include "diskgrp.h"
#include "system_metadata/Dedicated_map.h"
#include "lfsm_core.h"
#include "erase_count_private.h"
#include "erase_count.h"
#include "system_metadata/Dedicated_map.h"
#include "io_manager/io_common.h"
#include "EU_access.h"
#include "GC.h"
#include "EU_create.h"

static void dump_eu_erase_count(struct HListGC *h, t_erase_count * buf,
                                int32_t idx_st, int32_t cn, bool isBuftoEU)
{
    int32_t i, k, idx_eu, eu_limit;

    eu_limit = (idx_st + cn < h->total_eus) ? (idx_st + cn) : h->total_eus;
    LFSM_ASSERT(!IS_ERR_OR_NULL(buf));

    for (i = idx_st, k = 0; i < eu_limit; i++, k++) {
        idx_eu = h->total_eus * h->disk_num + i;
        if (isBuftoEU) {
            pbno_eu_map[idx_eu]->erase_count = buf[k];
        } else {
            buf[k] = pbno_eu_map[idx_eu]->erase_count;
        }
    }
}

// Tifa : ECT module is 4K/8K safety code.
int ECT_dump_disk(char *buffer, char **buffer_location, off_t offset,
                  int buffer_length, int *eof, void *data)
{
    int32_t len, sz_record, id_start_disk, idx_eu, type, cn_eu_per_disk;
    struct HListGC *h;

    len = 0;
    sz_record = 33;             // 33= size of "%5d %5d %20s\n"
    h = gc[0];
    cn_eu_per_disk = gc[0]->total_eus;
    id_start_disk = (int32_t) (long)data;

    if (offset / sz_record >= h->total_eus) {
        *eof = 1;
        return 0;
    }

    idx_eu = id_start_disk * cn_eu_per_disk + offset / sz_record;   // 33= size of "%5d %5d %20s\n"
    type = pbno_eu_map[idx_eu]->usage;

    if (pbno_eu_map[idx_eu]->erase_count == EU_UNUSABLE) {
        sprintf(buffer, "%05ld %-20s %5s\n", offset / sz_record, "EU_BAD",
                "-----");
    } else {
        sprintf(buffer, "%05ld %-20s %05u\n", offset / sz_record,
                EU_usage_type[type], pbno_eu_map[idx_eu]->erase_count);
    }

    buffer[sz_record - 1] = '\n';
    *buffer_location = buffer + offset % sz_record;
    len = sz_record - offset % sz_record;
    *eof = 0;
    return len;
}

int32_t ECT_dump_alldisk(int8_t * buffer, int32_t size)
{
    int32_t len, sum, i, j;
    struct HListGC *h;

    len = 0;
    sum = 0;
    h = gc[0];

    for (i = 0; i < lfsmdev.cn_ssds; i++) {
        sum = 0;

        for (j = 0; j < h->total_eus; j++) {
            if (pbno_eu_map[i * h->total_eus + j]->erase_count != EU_UNUSABLE) {
                sum +=
                    (uint32_t) pbno_eu_map[i * h->total_eus + j]->erase_count;
            }
            //printk(" %d %d  %d sum %d\n",i,j ,pbno_eu_map[i*h->total_eus+j]->erase_count,sum);
        }

        len +=
            sprintf(buffer + len,
                    "disk %2d : erase ratio %lld disk_size (%d)\n", i,
                    sum / h->total_eus, sum);

        if ((len + 30) > size) {
            len += sprintf(buffer + len, "UNFINISH_end\n");
            break;
        }
    }

    len += sprintf(buffer + len, "cn_free_total %d\n", gcn_free_total);
    return len;
}

static void ECT_end_bio(struct bio *bio)
{
    struct HListGC *h;

    h = bio->bi_private;

    LFSM_ASSERT(h != NULL);

    if (bio->bi_status) {
        h->ECT.io_queue_fin = blk_status_to_errno(bio->bi_status);
    } else {
        h->ECT.io_queue_fin = 1;
    }

    dprintk("%s io_queue_fin= %d\n", __FUNCTION__, h->ECT.io_queue_fin);
    bio->bi_iter.bi_size = PAGE_SIZE * MAX_MEM_PAGE_NUM;
    wake_up_interruptible(&h->io_queue);

    return;
}

bool ECT_init(struct HListGC *h, sector64 total_eu)
{
    struct SEraseCount *pECT;

    pECT = &h->ECT;
    memset(&h->ECT, 0, sizeof(struct SEraseCount));
    if ((total_eu * sizeof(t_erase_count)) > EU_SIZE) {
        LOGERR("total eu size is out of bound\n");
        return false;
    }

    pECT->num_mempage =
        ((total_eu * sizeof(t_erase_count)) +
         (HARD_PAGE_SIZE - 1)) / HARD_PAGE_SIZE;
    pECT->num_mempage *= MEM_PER_HARD_PAGE;

    LOGINFO("erase_map costs cn_mpage: %d\n", pECT->num_mempage);

    mutex_init(&pECT->lock);
    compose_bio(&pECT->bio, NULL, ECT_end_bio, h, MAX_MEM_PAGE_NUM * PAGE_SIZE,
                MAX_MEM_PAGE_NUM);
    pECT->cn_cycle = 0;
    return true;

}

void ECT_destroy(struct SEraseCount *pECT)
{
    free_composed_bio(pECT->bio, MAX_MEM_PAGE_NUM);
    mutex_destroy(&pECT->lock);
}

void ECT_change_next(struct SEraseCount *pECT)
{
    struct HListGC *h;

    h = gc[pECT->eu_next->disk_num];
    if (atomic_read(&pECT->fg_next_dirty) == 1) {
        LFSM_RMUTEX_LOCK(&h->whole_lock);
        if (FreeList_insert(h, pECT->eu_next) < 0) {
            LFSM_ASSERT(0);
        }

        pECT->eu_next = FreeList_delete_with_log(&lfsmdev, h, EU_ECT, false);
        LFSM_RMUTEX_UNLOCK(&h->whole_lock);

        atomic_set(&pECT->fg_next_dirty, 0);
        LOGINFO("finish with fg= %d\n", atomic_read(&pECT->fg_next_dirty));
    }
}

bool ECT_commit_all(lfsm_dev_t * td, bool isReuse)
{
    struct HListGC *h;
    int32_t i;

    if (lfsm_readonly == 1) {
        return true;
    }

    if (IS_ERR_OR_NULL(gc)) {
        return false;
    }

    for (i = 0; i < td->cn_ssds; i++) {
        if ((h = gc[i]) == NULL) {
            return false;
        }

        if ((h->ECT.eu_curr) == NULL) {
            return false;
        }

        if (isReuse) {
            if (EU_erase(h->ECT.eu_curr) < 0) {
                LOGERR("EU_erase fail\n");
                return false;
            }

            h->ECT.eu_curr->log_frontier = 0;
        }

        if (!ECT_commit(h)) {
            LOGWARN("Write failure erase_count_map of disk %d\n", i);
        }
    }

    if (dedicated_map_logging(td) < 0) {
        LOGERR("dedicated_map_logging fail\n");
        return false;
    }

    return true;
}

// weafon: return 0 or <0
static int32_t ECT_disk_io(struct HListGC *h, sector64 pos, int32_t rw,
                           int32_t cn_mpages)
{
    struct SEraseCount *pECT;
    int32_t cnt, wait_ret;

    pECT = &h->ECT;

    pECT->bio->bi_iter.bi_size = PAGE_SIZE * cn_mpages;
    pECT->bio->bi_next = NULL;
    pECT->bio->bi_iter.bi_idx = 0;
    pECT->bio->bi_opf = rw;
    pECT->bio->bi_status = BLK_STS_OK;
    pECT->bio->bi_flags &= ~(1 << BIO_SEG_VALID);

    pECT->bio->bi_vcnt = cn_mpages;

    pECT->bio->bi_iter.bi_sector = pos;
    pECT->io_queue_fin = 0;
    if (my_make_request(pECT->bio) == 0) {
        //lfsm_wait_event_interruptible(h->io_queue, pECT->io_queue_fin != 0);
        cnt = 0;
        while (true) {
            cnt++;
            wait_ret = wait_event_interruptible_timeout(h->io_queue,
                                                        pECT->io_queue_fin != 0,
                                                        LFSM_WAIT_EVEN_TIMEOUT);
            if (wait_ret <= 0) {
                LOGWARN("ECT disk IO no response after seconds %d\n",
                        LFSM_WAIT_EVEN_TIMEOUT_SEC * cnt);
            } else {
                break;
            }
        }
    } else {
        return -EIO;
    }

    return (pECT->io_queue_fin < 0) ? pECT->io_queue_fin : 0;
}

bool ECT_commit(struct HListGC *h)
{
    struct SEraseCount *pECT;
    int32_t i, off, cn_per_write, ret, cn_element, cn_mempage;
    t_erase_count *addr;
    struct EUProperty *eu;

    if (lfsm_readonly == 1) {
        return true;
    }

    off = 0;
    ret = 0;
    cn_element = 0;
    cn_mempage = 0;

    pECT = &h->ECT;
    LFSM_MUTEX_LOCK(&pECT->lock);

    cn_element = PAGE_SIZE / sizeof(t_erase_count); // 4096/4=1024
    cn_mempage = pECT->num_mempage;
    eu = pECT->eu_curr;

    //if ECT_eu full, change to new one
    if ((eu->log_frontier / SECTORS_PER_HARD_PAGE +
         cn_mempage / MEM_PER_HARD_PAGE)
        >= HPAGE_PER_SEU) {
        pECT->cn_cycle++;

        if (atomic_read(&pECT->fg_next_dirty) != 0) {
            LOGERR("pECT->fg_next_dirty = %d\n",
                   atomic_read(&pECT->fg_next_dirty));
            LFSM_MUTEX_UNLOCK(&pECT->lock);
            return false;
        }

        LOGINFO("Need change ECT from %llu to %llu\n",
                pECT->eu_curr->start_pos, pECT->eu_next->start_pos);
        eu = pECT->eu_next;
    }

    while (cn_mempage > 0) {
        cn_per_write =
            ((cn_mempage < MAX_MEM_PAGE_NUM) ? cn_mempage : MAX_MEM_PAGE_NUM);

        for (i = 0; i < cn_per_write; i++) {
            addr = (t_erase_count *) kmap(pECT->bio->bi_io_vec[i].bv_page);
            dump_eu_erase_count(h, addr, off, cn_element, false);
            kunmap(pECT->bio->bi_io_vec[i].bv_page);
            off += cn_element;
        }

        ret =
            ECT_disk_io(h, eu->start_pos + eu->log_frontier, REQ_OP_WRITE,
                        cn_per_write);
        if (ret < 0) {          //tf todo: erase_map get new EU and commit again
            LOGERR(" erase_map write failure at %llu\n",
                   eu->start_pos + eu->log_frontier);
        }

        cn_mempage -= cn_per_write;
        eu->log_frontier += cn_per_write * SECTORS_PER_PAGE;
    }

    if (eu == pECT->eu_next) {
        // swap eu_curr and eu_next during eu_curr full
        pECT->eu_next = pECT->eu_curr;
        pECT->eu_curr = eu;
        atomic_set(&pECT->fg_next_dirty, 1);    //should set up after swap, because background will check
    }

    LFSM_MUTEX_UNLOCK(&pECT->lock);
    return true;
}

static void ECT_update_minmax(struct HListGC *h, struct EUProperty *victimEU)
{
    if (NULL == h->min_erase_eu) {
        if (victimEU->erase_count < 0XFFFFFFFF) {
            h->min_erase_eu = victimEU;
        }
    } else {
        if (victimEU->erase_count <= h->min_erase_eu->erase_count) {
            h->min_erase_eu = victimEU;
        }
    }

    if (NULL == h->max_erase_eu) {
        if (victimEU->erase_count > 0) {
            h->max_erase_eu = victimEU;
        }
    } else {
        if (victimEU->erase_count >= h->max_erase_eu->erase_count) {
            h->max_erase_eu = victimEU;
        }
    }
}

void ECT_erase_count_inc(struct EUProperty *victimEU)
{
    struct HListGC *h;

    h = gc[victimEU->disk_num];
    // AN 2012/10/4 : this function is only called by EU_earse
    // becasue only one thread to erase a EU, no need to lock ECT lock
    LFSM_ASSERT(((victimEU->start_pos - h->eu_log_start) >> EU_BIT_SIZE)
                < h->total_eus);

    victimEU->erase_count++;
    ECT_update_minmax(h, victimEU);
}

int32_t ECT_load_all(lfsm_dev_t * td)
{
    int32_t i, ret;
    struct HListGC *h;

    ret = 0;
    for (i = 0; i < td->cn_ssds; i++) {
        if (!DEV_SYS_READY(grp_ssds, i)) {
            continue;
        }

        h = gc[i];
        if ((ret = ECT_load(h)) < 0) {
            LOGERR("Can't get erase_count_map %d (ret %d)\n", i, ret);
            return ret;
        }
    }

    return 0;
}

bool ECT_next_eu_init(lfsm_dev_t * td)
{
    int32_t i;

    for (i = 0; i < td->cn_ssds; i++) {
        if (td->prev_state != sys_fresh) {
            if (IS_ERR_OR_NULL
                (gc[i]->ECT.eu_next = FreeList_delete(gc[i], EU_ECT, false))) {
                return false;
            }
            atomic_set(&gc[i]->ECT.fg_next_dirty, 0);
        }
    }
    return true;
}

int32_t ECT_rescue_disk(lfsm_dev_t * td, int32_t id_fail_disk)
{
    int32_t ret;
    struct HListGC *h;

    if (lfsm_readonly == 1) {
        return 0;
    }

    h = gc[id_fail_disk];
    if (!DEV_SYS_READY(grp_ssds, id_fail_disk)) {
        LOGINFO("rescue ECT disk %d\n", id_fail_disk);
        if ((ret = ECT_rescue(h)) < 0) {
            return ret;
        }
    }

    return 0;
}

int32_t ECT_rescue(struct HListGC *h)
{
    struct SEraseCount *pECT;
    int32_t ret;

    ret = -ENOSPC;
    pECT = &h->ECT;

    LFSM_MUTEX_LOCK(&pECT->lock);

    do {
        if (IS_ERR_OR_NULL(h->ECT.eu_curr)) {
            LOGERR("h->ECT.eu_curr is %p, diskid %d\n", h->ECT.eu_curr,
                   h->disk_num);
            break;
        }

        if (IS_ERR_OR_NULL(h->ECT.eu_curr = EU_reset(h->ECT.eu_curr))) {
            LOGERR("h->ECT.eu_curr is %p, diskid %d\n", h->ECT.eu_curr,
                   h->disk_num);
            break;
        }

        if (IS_ERR_OR_NULL(h->ECT.eu_next)) {
            LOGERR("h->ECT.eu_next is %p, diskid %d\n", h->ECT.eu_next,
                   h->disk_num);
            break;
        }

        if (IS_ERR_OR_NULL(h->ECT.eu_next = EU_reset(h->ECT.eu_next))) {
            LOGERR("h->ECT.eu_next is %p, diskid %d\n", h->ECT.eu_next,
                   h->disk_num);
            break;
        }

        atomic_set(&h->ECT.fg_next_dirty, 0);
        ret = 0;
    } while (0);

    LFSM_MUTEX_UNLOCK(&pECT->lock);
    return ret;
}

int32_t ECT_load(struct HListGC *h)
{
    struct SEraseCount *pECT;
    sector64 pos, frontier;
    t_erase_count *addr;
    int32_t cn_mempage, cn_element, i, cn_per_write, off, ret;

    cn_mempage = 0;
    cn_element = 0;             // 4096/4=1024
    frontier = 0;
    off = 0;
    ret = 0;
    pECT = &h->ECT;
    LFSM_MUTEX_LOCK(&pECT->lock);
    cn_mempage = pECT->num_mempage;
    cn_element = PAGE_SIZE / sizeof(t_erase_count); // 4096/4=1024
    frontier = pECT->eu_curr->log_frontier - cn_mempage * SECTORS_PER_PAGE;

    if (pECT->eu_curr->log_frontier == 0) {
        LFSM_MUTEX_UNLOCK(&pECT->lock);
        return 0;
    }

    if (frontier < 0 || frontier >= (1 << (SECTORS_PER_SEU_ORDER))) {
        LFSM_MUTEX_UNLOCK(&pECT->lock);
        LOGERR("out of bound frontier: %lld\n", frontier);
        return -EFAULT;         //out of bound
    }
    pos = pECT->eu_curr->start_pos + frontier;
//    LOGINFO("%s read ECT[%d] pos %llu page %d\n", __FUNCTION__,pECT->eu_curr->disk_num, pos, cn_mempage);

    while (cn_mempage > 0) {
        cn_per_write =
            ((cn_mempage < MAX_MEM_PAGE_NUM) ? cn_mempage : MAX_MEM_PAGE_NUM);
        if ((ret = ECT_disk_io(h, pos, REQ_OP_READ, cn_per_write)) < 0) {   //tf todo: erase_map get new EU and quit the old ECT info
            LOGERR("erase_map read failure (%d) at %llu\n", ret, pos);
            goto l_fail;
        }

        for (i = 0; i < cn_per_write; i++) {
            addr = (t_erase_count *) kmap(pECT->bio->bi_io_vec[i].bv_page);
            dump_eu_erase_count(h, addr, off, cn_element, true);
            kunmap(pECT->bio->bi_io_vec[i].bv_page);
            off += cn_element;
        }

        cn_mempage -= cn_per_write;
        pos += cn_per_write * SECTORS_PER_PAGE;
    }

  l_fail:
    LFSM_MUTEX_UNLOCK(&pECT->lock);
    return ret;
}
