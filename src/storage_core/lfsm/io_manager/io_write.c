/*
 * SPDX-License-Identifier: GPL-2.0-or-later OR Apache-2.0
 * Copyright (C) 2015-2020 Industrial Technology Research Institute
 *
 *
 *
 *  
 */
/*
** This file constitues of all the functions related to WRITE LOGGING
** It deals with :
** 1. Assigning PBN for a WRITE (main thread)
** 2. Handling WRITE to the metadata section of the eu
** 3. Handling WRITE requests spanning more than one eu
*/

#include <linux/types.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/bio.h>
#include "../config/lfsm_setting.h"
#include "../config/lfsm_feature_setting.h"
#include "../config/lfsm_cfg.h"
#include "../common/common.h"
#include "../common/mem_manager.h"
#include "../common/rmutex.h"
#include "mqbiobox.h"
#include "biocbh.h"
#include "../nextprepare.h"
#include "../diskgrp.h"
#include "../system_metadata/Dedicated_map.h"
#include "../lfsm_core.h"
#include "../bmt.h"
#include "../bmt_commit.h"
#include "../bmt_ppdm.h"
#include "../bmt_ppq.h"
#include "io_common.h"
#include "io_write.h"
#include "../GC.h"
#include "../EU_access.h"
#include "../spare_bmt_ul.h"
#include "../special_cmd.h"
#include "../err_manager.h"
#include "../EU_create.h"
#include "../autoflush.h"
#include "../devio.h"
#include "../io_manager/io_read.h"
#include "../metabioc.h"
#include "../mdnode.h"
#include "../conflict.h"
#include "../metalog.h"
#include "../xorcalc.h"
#include "../sysapi_linux_kernel.h"

#include "../hook.h"

int32_t update_bio_spare_area(struct bio_container *bio_ct, sector64 * LBN,
                              int32_t num_lbns);

// true means latest, false means not latest
bool bioc_chk_is_latest(struct bio_container *pbioc)
{
    //if(pbioc->par_bioext==NULL) ///AAA todo
    //    return true;
    ///// with par_bioext
    if (atomic_dec_and_test(&pbioc->cn_wait)) {
        return true;
    }

    return false;
}

/* end_bio_write HWint32_t 1. set active_reading -- 2. end_bio(ori_bio) 3. recycle bio contaoner 4. dev_busy = 0
 * @[in]bio: bio 
 * @[in]err: 
 * @[return] void
 */
static struct bio_container *end_bio_write_generic(struct bio *bio)
{
    struct bio_container *entry = bio->bi_private;
    blk_status_t err = bio->bi_status;

    entry = bio->bi_private;
    if (NULL == entry) {
        LOGINERR("bio %p entry=0...\n", bio);
        return NULL;
    }

    if (entry->bio->bi_vcnt == 0) {
        assert_printk("warning bioc(%p) lpn (%llu,%llu) bio %p bi_sector=%lld "
                      "(0x%llx) bi_opf=%u bi_vcnt zero\n",
                      entry, entry->start_lbno, entry->end_lbno, bio,
                      (sector64) bio->bi_iter.bi_sector,
                      (sector64) bio->bi_iter.bi_sector, bio->bi_opf);
    }

    if (err) {
        LOGERR("bio %p err=%d\n", bio, err);
        entry->status = err;
    }

    if (rddcy_end_bio_write_helper(entry, bio) < 0) {
        goto l_fail;
    }

    return entry;
    goto l_fail;
  l_fail:
    entry->status = BLK_STS_RESOURCE;
    return entry;
}

static void bio_end_pipeline_write(struct bio *bio)
{
    struct bio_container *entry;
    entry = end_bio_write_generic(bio);
    if (bioc_chk_is_latest(entry)) {
        biocbh_io_done(&lfsmdev.biocbh, entry, true);
    }

    return;
}

void end_bio_write(struct bio *bio)
{
    struct bio_container *entry;

    entry = end_bio_write_generic(bio);
    rddcy_printk("getbio_nonpipecall_back bioc %p\n", entry);
    LFSM_ASSERT2(entry->bio->bi_vcnt != 0,
                 "bioc(%p) lpn %llu, %llu bi_vcnt zero\n", entry,
                 entry->start_lbno, entry->end_lbno);
    entry->io_queue_fin = 1;    // set io_queue_fin =1 to wakeup fronted process
    wake_up_interruptible(&entry->io_queue);
    return;
}

/* @bio: the original bio
 * @lbno[O]: the lbno after the alignment in the unit of 4KB page
 * @vcnt[O]: the size in the unit of 4KB page 
 * */
void get_local_lbno(struct bio *org_bio, sector64 * lbno,
                    int32_t * flash_page_num)
{
    *lbno = (org_bio->bi_iter.bi_sector >> SECTORS_PER_FLASH_PAGE_ORDER);
    *flash_page_num = BIO_SIZE_ALIGN_TO_FLASH_PAGE(org_bio) >> FLASH_PAGE_ORDER;
}

int32_t get_disk_num(sector64 pbno)
{
    sector64 pbno_af_adj;
    int32_t dev_id;

    if (!pbno_lfsm2bdev_A(pbno, &pbno_af_adj, &dev_id, &grp_ssds)) {
        return -ENODEV;
    }
    return dev_id;
}

// Hold the foreground process until a free EU is released by the background GC process
// Notably, since h->active_eus[x] may be changed by the background GC process,
// one may need to reexecute "active_eu = h->active_eus[cur_temperature];" 
// after wait_free_eu is called if the pointer active_eu is supposed to point32_t to the active eu.
static bool wait_free_eu(lfsm_dev_t * td, struct bio_container *bio_c,
                         struct HListGC *h)
{
    int32_t count;

    conflict_bioc_mark_waitSEU(bio_c, td);

    count = 0;
    //mxx_printk("f %llu t %d %llu\n",h->free_list_number,bio_c->write_type,bio_c->start_lbno);
    while ((h->free_list_number <= MIN_RESERVE_EU) &&
           (td->ar_pgroup[PGROUP_ID(h->disk_num)].state == epg_good)) {
        count++;
        //mxx_printk("binc lpn %llu disk_num %d count %d\n",bio_c->start_lbno,h->disk_num,count);
        wake_up_interruptible(&td->worker_queue);
        lfsm_wait_event_interruptible(td->gc_conflict_queue,
                                      h->free_list_number > MIN_RESERVE_EU ||
                                      td->ar_pgroup[PGROUP_ID(h->disk_num)].
                                      state != epg_good);
    }

    conflict_bioc_mark_nopending(bio_c, td);
    return true;
}

struct EUProperty *change_active_eu_A(lfsm_dev_t * td, struct HListGC *h,
                                      int32_t temper, bool withOld)
{
    struct EUProperty *eu_new;
    int32_t *pid_hpeu;

    eu_new = NULL;
    if (NULL == (eu_new = FreeList_delete_A(td, h, EU_DATA, true, true))) {
        LOGERR("Fail to get new eu_new (%ld)\n", PTR_ERR(eu_new));
        return eu_new;
    }

    eu_new->temper = temper;
    if (NULL ==
        (pid_hpeu = lfsm_pgroup_retref_hpeuid(td, eu_new->disk_num, temper))) {
        goto l_fail;
    }

    if (!HPEU_smartadd_eu(&td->hpeutab, eu_new, pid_hpeu)) {
        LOGERR("smartadd fail eu %llu\n", eu_new->start_pos);
        goto l_fail;
    }
    // Tifa, Weafon 2012/9/4: we move LVP_heap_insert from metadata_write to here
    // Events "change active eu" and "lvp_heap_insert" should be atomic.
    // if one eu is inserted in lvp_heap, then the eu must not be in active
    // the error is detected by HPEU_turn_fixed
    if (withOld) {
        HOOK_REPLACE(LVP_heap_insert(h, h->active_eus[temper]),
                     fn_change_active_eu, h, temper);
    }

    h->par_cn_log[temper] = 0;
    h->active_eus[temper] = eu_new;
    h->active_eus[temper]->temper = temper;
    if (mdnode_alloc(&eu_new->act_metadata, td->pMdpool) == -ENOENT) {
        LOGWARN("eu %llu active eu have been allocated before\n",
                eu_new->start_pos);
    }

    return h->active_eus[temper];

  l_fail:
    FreeList_insert_nodirty(h, eu_new);
    return NULL;
}

static struct EUProperty *change_active_eu(lfsm_dev_t * td, struct HListGC *h,
                                           int32_t cur_temperature)
{
    struct EUProperty *eu_new;
    int32_t ret;

    // if we check the availibility of free eu until now and goto begin_get_dest_pbno,
    // then the matadata of the current active EU will not be written,
    // since the log_frontier of currect active EU has been increased by update_new_PBN_bitmap().

    if (h->active_eus[cur_temperature]->log_frontier
        != h->active_eus[cur_temperature]->eu_size -
        (METADATA_SIZE_IN_SECTOR - DUMMY_SIZE_IN_SECTOR)) {
        LOGINERR("eu %p\n", h->active_eus[cur_temperature]);
        return NULL;
    }

    if (NULL == (eu_new = change_active_eu_A(td, h, cur_temperature, true))) {
        return eu_new;
    }

    ret =
        UpdateLog_logging(td, h->active_eus[cur_temperature], cur_temperature,
                          UL_NORMAL_MARK);

    //active_eu = h->active_eus[cur_temperature];
    return h->active_eus[cur_temperature];
}

/*
** To assing PBN to a LBN for a main thread WRITE request
**
** BMT lookup happens for the corresponding LBNs to decide the temperature
** Tries to assign PBNs for all the pages in the bio in the active eu of corresponding temperature
** If not possible, allocates new active eu from the free pool and continues allocating PBNs from the new active eu
** If no free eus are available then the main thread yields to the back ground thread for GC to happen
** In that case, the allocation of PBNs re-starts from the first page so as to avoid a possibility of a deadlock
**
** @td : lfsm_dev_t object
** @h : Garbage Collection structure
** @bio_c : The bio container structure of the bio
** @lbno : The LBN of the starting page in the bio
** @size : Number of pages in the bio
** @pbno : The PBN array to be filled by this function
** @old_pbno : Array to store the old PBN values 
** @pending_metadata_pbno : 0 if this write doesn't trigger the metadata section write. Appropriate sector number, otherwise
**
** Returns the PBN of the starting page of the bio
*/
static int32_t get_old_temper(lfsm_dev_t * td, sector64 * old_pbno, int32_t size, sector64 lbno)    //here support the lbno should be continue;
{
    struct D_BMT_E dbmta;
    struct EUProperty *eu_element;
    int32_t i, ret, old_temper;

    ret = -1;
    old_temper = -1;
    for (i = 0; i < size; i++) {
        LFSM_ASSERT((lbno + i) <= (td->logical_capacity));
        ret = bmt_lookup(td, td->ltop_bmt, (lbno + i), true, 1, &dbmta);
        if (ret < 0 && ret != -EACCES) {
            return -2;
        }

        old_pbno[i] = dbmta.pbno;

        if (PBNO_ON_SSD(old_pbno[i])) {
            eu_element =
                pbno_eu_map[(((old_pbno[i] - eu_log_start) >> EU_BIT_SIZE))];
            if (eu_element->temper > old_temper) {
                old_temper = eu_element->temper;
            }
        }
    }

    return old_temper;
}

static int32_t get_temperature(lfsm_dev_t * td, sector64 * old_pbno,
                               int32_t size, sector64 lbno, int32_t delta)
{
    int32_t temperature;

    if ((temperature = get_old_temper(td, old_pbno, size, lbno)) == -2) {
        return -ESPIPE;
    }

    if ((delta == -1) && (temperature == -1)) {
        return -EPERM;
    }

    temperature += delta;
    if (temperature < 0) {
        temperature = 0;
    }

    if (temperature > (TEMP_LAYER_NUM - 1)) {
        temperature = TEMP_LAYER_NUM - 1;
    }

    return temperature;
}

/*
** To know how many PBNs actually belong to one EU
** This function is useful to know whether a particular bio spans more than one bio
**
** @pbno : The PBN array
** @size : Size of the PBN array
**
** Returns number of continuos PBNs in the PBN array
*/
int32_t get_contiguous_PBN(sector64 * pbno, int32_t size)
{
    int32_t i;

    LFSM_ASSERT(size <= MAX_FLASH_PAGE_NUM);

    for (i = 0; i < size - 1; i++) {
        if (pbno[i + 1] != pbno[i] + SECTORS_PER_FLASH_PAGE) {
            return (i + 1);
        }
    }

    LFSM_ASSERT((i + 1) == size);
    return size;
}

int32_t process_copy_write_fail(struct bad_block_info *bad_blk_info)
{
    int32_t items;
    int32_t failure_code;

    failure_code = LFSM_NO_FAIL;
    for (items = 0; items < bad_blk_info->num_bad_items; items++) {
        if (bad_blk_info->errors[items].failure_code == LFSM_ERR_WR_RETRY) {
            if (failure_code == LFSM_ERR_WR_RETRY
                || failure_code == LFSM_NO_FAIL) {
                failure_code = LFSM_ERR_WR_RETRY;
            } else {
                LFSM_ASSERT(0);
            }
        } else {                // == LFSM_ERR_WR_ERROR
            LFSM_ASSERT(failure_code != LFSM_ERR_WR_RETRY)
                failure_code = LFSM_ERR_WR_ERROR;
        }
    }

    return failure_code;
}

int32_t retry_special_read_enquiry(lfsm_dev_t * td, sector64 bi_sector,
                                   int32_t page_num, int32_t org_cmd_type,
                                   int32_t BlkUnit,
                                   struct bad_block_info *bad_blk_info)
{
    int32_t cp_err, cn_retry;

    cp_err = 0;
    cn_retry = 0;
    do {
        cn_retry++;
        cp_err = special_read_enquiry(bi_sector, page_num, 0, 0, bad_blk_info);
    } while (cp_err < 0 && cn_retry < 4);
    LFSM_ASSERT(cp_err >= 0);
    return cp_err;
}

static int32_t bioc_write_userdata(struct bio_container *pbioc,
                                   bio_end_io_t * fn_endbio, int32_t isWait)
{
    int32_t cnt, wait_ret;

    pbioc->io_queue_fin = 0;
    pbioc->bytes_submitted = pbioc->bio->bi_iter.bi_size;
    pbioc->bio->bi_end_io = fn_endbio;
    pbioc->ts_submit_ssd = jiffies_64;

    if (my_make_request(pbioc->bio) < 0) {
        // call end_io to ask bh handle (i.e. reget_dest) the bio, so don't return now
        pbioc->bio->bi_status = BLK_STS_IOERR;
        pbioc->bio->bi_end_io(pbioc->bio);
    }

    if (isWait) {
        //lfsm_wait_event_interruptible(pbioc->io_queue, pbioc->io_queue_fin != 0);
        cnt = 0;
        while (true) {
            cnt++;
            wait_ret = wait_event_interruptible_timeout(pbioc->io_queue,
                                                        pbioc->io_queue_fin !=
                                                        0,
                                                        LFSM_WAIT_EVEN_TIMEOUT);
            if (wait_ret <= 0) {
                LOGWARN("metabioc IO no response %lld after seconds %d\n",
                        (sector64) pbioc->bio->bi_iter.bi_sector,
                        LFSM_WAIT_EVEN_TIMEOUT_SEC * cnt);
            } else {
                break;
            }
        }
    }
    return 0;
}

/*
** To do the DATA logging and metadata page write if necessary
**
** @data_bio_c : The bio container for the data
** @metadata_bio_c : The bio container for the metadata
**
** Returns error or zero
*/
/* possible return value RET_BIOBH, REGET_DESTPBNO, -EIO, 0*/

static int32_t write_data_and_metadata(lfsm_dev_t * td,
                                       struct bio_container *data_bio_c,
                                       struct bio_container *metadata_bio_c,
                                       int32_t isWait, int32_t id_disk)
{
    int32_t ret;

    ret = 0;

    data_bio_c->selected_diskID = id_disk;
    if ((ret =
         bioc_write_userdata(data_bio_c, bio_end_pipeline_write, false)) < 0) {
        LOGERR("Unhandled write failure blocking ret = %d\n", ret);
    }
    ret = RET_BIOCBH;

    if (metadata_bio_c) {
        metabioc_save_and_free(td, metadata_bio_c, false);  // metadata_bio is returned in metadata_write, always zero
    }

    return ret;
}

/*
** To update the details of the EU which contains the old PBN of the LBN which we have now over-written
** Update the old bitmap
** Move the old EU to the HList. Just make sure that the EU is not an active EU
**
** @h : The GC object
** @size : Number of pages in the bio that are to be updated now
** @old_pbno : The old PBN array
**
** Returns 0
*/
int32_t update_old_eu(struct HListGC *h, int32_t size, sector64 * old_pbno)
{
    struct EUProperty *entry;
    struct HListGC *local_h;
    int32_t ret, i, org_temperature;

    local_h = NULL;
    ret = 0;
    for (i = 0; i < size; i++) {
        if (old_pbno[i] != 0) {
            entry = pbno_eu_map[(old_pbno[i] - eu_log_start) >> EU_BIT_SIZE];

            //Lego 20160120: Original is compile flag: WF_ENABLE_BOTTLENECKS why?
            ret = clear_PBN_bitmap_validcount(old_pbno[i]);
            org_temperature = entry->temper;
            local_h = gc[entry->disk_num];

            // if(eu_is_active(entry)<0)
            // weafon: an active eu may not be in heap or hlist since we postpone the heap insertion of a eu till the complete of its metadata write,
            LFSM_RMUTEX_LOCK(&local_h->whole_lock);
            if (atomic_read(&entry->state_list) != -1) {
                move_old_eu_to_HList(local_h, entry);
            }
            LFSM_RMUTEX_UNLOCK(&local_h->whole_lock);
        }
    }

    return ret;
}

static int32_t bioc_write_submit_data(lfsm_dev_t * td,
                                      struct swrite_param *pwparam)
{
    struct bio_container *pbioc_meta;
    int32_t ret;

    ret = -EINVAL;
    pbioc_meta = NULL;
    if (pwparam->bio_c->bio->bi_vcnt != MEM_PER_FLASH_PAGE) {
        assert_printk("pid =%d, %d\n", current->pid,
                      pwparam->bio_c->bio->bi_vcnt);
        goto l_end;
    }
    //LOGINFO("lbno %lld -> %llu [birthday %lld ft %lld pg %d]\n",pwparam->lbno, pwparam->pbno_new,
    //            HPEU_get_birthday(&td->hpeutab, eu_find(pwparam->pbno_new)), HPEU_get_frontier(pwparam->pbno_new),
    //            PGROUP_ID(eu_find(pwparam->pbno_new)->disk_num));
    pwparam->bio_c->bytes_submitted = pwparam->bio_c->bio->bi_iter.bi_size;
    if (pwparam->pbno_meta) {
        if (NULL == (pbioc_meta = metabioc_build(td, NULL, pwparam->pbno_meta))) {
            ret = PTR_ERR(pbioc_meta);
            goto l_end;
        }
    }
    // FIXME : should find out a right place, Weafon
    pwparam->bio_c->bio->bi_iter.bi_size = FLASH_PAGE_SIZE;

    ret =
        write_data_and_metadata(td, pwparam->bio_c, pbioc_meta, false,
                                pwparam->id_disk);
    // return value    RET_BIOBH, REGET_DESTPBNO, -EIO, 0
    goto l_end;

    free_bio_container(pbioc_meta, METADATA_SIZE_IN_PAGES);

  l_end:
    // wake up conflicted bio for -EIO and 0 (--> non-pipeline normal finish)
    // AN 20121114:move to topest level
    //if((ret<=0) && (pwparam->bio_c->write_type!=TP_GC_IO))
    //    post_rw_check_waitlist(td, pwparam->bio_c,ret<0);

    //move put_bio_container(td, bio_c) to get_dest_pbno_and_finish_write under LDDCY=1
    return ret;
}

static int32_t bioc_write_submit_A(lfsm_dev_t * td,
                                   struct swrite_param *pwparam, int32_t isWait)
{
    //struct bio_container *update_log_bio_c = NULL;
    struct bio_container *pbioc_meta;

    pbioc_meta = NULL;
    rddcy_printk("\nEntered %s ", __FUNCTION__);

    if ((pwparam->bio_c->bio->bi_vcnt % MEM_PER_FLASH_PAGE) != 0) {
        assert_printk("pid =%d, %d", current->pid,
                      pwparam->bio_c->bio->bi_vcnt);
        return -EIO;
    }

    pwparam->bio_c->bytes_submitted = pwparam->bio_c->bio->bi_iter.bi_size;

    if (pwparam->pbno_meta) {
        if (NULL == (pbioc_meta = metabioc_build(td, NULL, pwparam->pbno_meta))) {
            return PTR_ERR(pbioc_meta);
        }
    }

    if (update_bio_spare_area(pwparam->bio_c, &(pwparam->lbno), 1) < 0) {
        goto l_fail;
    }

    return write_data_and_metadata(td, pwparam->bio_c, pbioc_meta, isWait,
                                   pwparam->id_disk);
    // return value    RET_BIOBH, REGET_DESTPBNO, -EIO, 0
  l_fail:
    free_bio_container(pbioc_meta, METADATA_SIZE_IN_PAGES);

    pwparam->bio_c->status = BLK_STS_IOERR;
    return -EIO;
}

static int32_t bioc_write_submit_prepad(lfsm_dev_t * td,
                                        struct swrite_param *pwparam)
{
//    printk("%s: padding bio for lbno %llx to disk %d bi_sector = %lld \n",__FUNCTION__,
//            pwparam->lbno,pwparam->id_disk,(sector64)pwparam->bio_c->bio->bi_iter.bi_sector);
    bioc_write_submit_A(td, pwparam, true); // when WF_WRITE_ALL_BH_HANDLED ==1 , bioc_write_submit_A return value is 0,
    // when WF_WRITE_ALL_BH_HANDLED ==0,  bioc_write_submit_A return value is RET_BIOCBH

    return REGET_DESTPBNO;      // no matter prepad bio is bh handled and shall reget_dest_pbno
}

static int32_t bioc_write_submit_ecc(lfsm_dev_t * td,
                                     struct swrite_param *pwparam)
{
#if LFSM_XOR_METHOD == 2 || LFSM_XOR_METHOD == 3
    int32_t retValue;

    retValue = xorcalc_outside_stripe_lock(pwparam->bio_c);
    if (retValue < 0) {
        return retValue;
    }
#endif
    return bioc_write_submit_A(td, pwparam, false);
}

void end_bio_partial_write_read(struct bio *bio)
{
    LFSM_ASSERT2(0, "%s: partial write should not use brm: bio=%p err=%d\n",
                 __FUNCTION__, bio, bio->bi_status);
}

static int32_t bioc_get_partial_state(sector64 addr, sector64 size,
                                      sector64 lpn_start, sector64 lpn_end)
{
    int32_t ret = 0;

    if (((addr % SECTORS_PER_FLASH_PAGE) > 0) ||
        (((addr % SECTORS_PER_FLASH_PAGE) == 0)
         && (size < SECTORS_PER_FLASH_PAGE))) {
        ret |= HEAD_PAGE_UNALIGN_MASK;
    }

    if ((lpn_end > lpn_start) && ((addr + size) % SECTORS_PER_FLASH_PAGE) > 0) {
        ret |= TAIL_PAGE_UNALIGN_MASK;
    }

    return ret;
}

static int32_t bioc_data_preload(lfsm_dev_t * td, struct bio_container *bioc,
                                 int32_t lpn_start, int32_t idx_fpage,
                                 bool dummy)
{
    if (pgcache_load(&td->pgcache, lpn_start,
                     &(bioc->bio->bi_io_vec[idx_fpage * MEM_PER_FLASH_PAGE])) ==
        0) {
        return 0;
    }

    if (iread_syncread(td, lpn_start, bioc, idx_fpage)) {
        return 0;
    }

    return -EIO;
}

static int32_t bioc_handle_partial(lfsm_dev_t * td, struct bio_container *bioc,
                                   func_sync_t pfunc_sync, int32_t * pret_state)
{
    sector64 addr, size;
    int32_t ret1, ret2, lpn_start, lpn_end, state;

    ret1 = 0;
    ret2 = 0;
    addr = bioc->org_bio->bi_iter.bi_sector;    // in sector
    size = bioc->org_bio->bi_iter.bi_size >> SECTOR_ORDER;  // in sector

    if (bioc->write_type != TP_PARTIAL_IO) {
        return 0;
    }

    lpn_start = LFSM_FLOOR_DIV(addr, SECTORS_PER_FLASH_PAGE);
    lpn_end = LFSM_FLOOR_DIV(addr + size, SECTORS_PER_FLASH_PAGE);

    state = bioc_get_partial_state(addr, size, lpn_start, lpn_end);

    if (HEAD_PAGE_UNALIGN(state)) {
        ret1 = pfunc_sync(td, bioc, lpn_start, 0, true);
    }

    if (TAIL_PAGE_UNALIGN(state)) {
        ret2 = pfunc_sync(td, bioc, lpn_end, lpn_end - lpn_start, true);
    }

    if (pret_state) {
        *pret_state = state;
    }

    if ((ret1 < 0) || (ret2 < 0)) {
        LOGERR("fail head/tail ret=%d/%d pfunc_sync= %p\n", ret1, ret2,
               pfunc_sync);
        return -EIO;
    }

    return 0;
}

static int32_t bioc_data_cache_update_A(lfsm_dev_t * td,
                                        struct bio_container *bioc,
                                        int32_t lpn_start, int32_t idx_fpage,
                                        bool isForceUpdate)
{
    if (bioc->par_bioext) {
        return pgcache_update(&td->pgcache, lpn_start,
                              &(bioc->par_bioext->
                                bio_orig->bi_io_vec[idx_fpage *
                                                    MEM_PER_FLASH_PAGE]),
                              isForceUpdate);
    }

    return pgcache_update(&td->pgcache, lpn_start,
                          &(bioc->bio->
                            bi_io_vec[idx_fpage * MEM_PER_FLASH_PAGE]),
                          isForceUpdate);
}

int32_t bioc_data_cache_update(lfsm_dev_t * td, struct bio_container *bioc)
{
    int32_t i, state, start, len;

    state = 0;
    start = 0;
    len = bioc_cn_fpage(bioc);

    // force update the new data of bioc's partial pages to pagecache
    if (bioc_handle_partial(td, bioc, bioc_data_cache_update_A, &state) < 0) {
        LOGERR("sync failure bioc %p\n", bioc);
    }
    // update the new data of bioc's other pages to pagecache, IF these page has been cached.
    if (HEAD_PAGE_UNALIGN(state)) {
        start = 1;
    }

    if (TAIL_PAGE_UNALIGN(state)) {
        len--;
    }

    for (i = start; i < len; i++) {
        bioc_data_cache_update_A(td, bioc, bioc->start_lbno + i, i, false);
    }

    return 0;
}

#if LFSM_RELEASE_VER == 3
/*
 * Lego-20161005: The reason we change the name of function is try to
 *                prevent user from modify binary code.
 *                So we design a confusing name.
 *                When number of write data >= threshold, we will modify user data
 */
int8_t str_license_base[32] = { 0x00, 0x69, 0x63, 0x65, 0x6e, 0x00, 0x00, 0x00,
    0x70, 0x60, 0x00, 0x00, 0x09, 0x03, 0x03, 0x00,
    0x60, 0x60, 0x00, 0x70, 0x09, 0x60, 0x05, 0x60,
    0x70, 0x70, 0x00, 0x00, 0x15, 0x00, 0x00, 0x00
};

int32_t gcc = 0;
#endif
#if LFSM_RELEASE_VER == 0
static int32_t iwrite_user_data_setup(lfsm_dev_t * td,
                                      struct bio_container *pbioc,
                                      struct bio *pbio_user)
#else
static int32_t read_metadata_all(lfsm_dev_t * td, struct bio_container *pbioc,
                                 struct bio *pbio_user)
#endif
{
    struct bio_vec bvec;
    struct bvec_iter iter;
    void *iovec_mem;
    uint64_t offset;
    int32_t i, ivcnt;

#if LFSM_RELEASE_VER == 3
    int8_t str_license[32] = { 0x6c, 0x00, 0x00, 0x00, 0x00, 0x73, 0x65, 0x20,
        0x00, 0x05, 0x72, 0x6d, 0x60, 0x70, 0x70, 0x69,
        0x0f, 0x0e, 0x20, 0x04, 0x60, 0x0d, 0x60, 0x0f,
        0x05, 0x04, 0x00, 0x00, 0x15, 0x00, 0x00, 0x00
    };
    if (get_total_write_io(&lfsmdev.bioboxpool) >
        ((lfsmdev.logical_capacity << 3) - lfsmdev.logical_capacity)) {
        gcc = 1;
        for (i = 0; i < 32; i++) {
            str_license[i] += str_license_base[i];
        }
    }
#endif
    offset = pbio_user->bi_iter.bi_sector << SECTOR_ORDER;  //in bytes
    ivcnt = 0;

#if LFSM_PAGE_SWAP == 1
    if ((pbio_user->bi_vcnt == MEM_PER_FLASH_PAGE)
        && ((pbio_user->bi_iter.bi_sector % SECTORS_PER_FLASH_PAGE) == 0)   //tifa
        && (pbio_user->bi_iter.bi_size % PAGE_SIZE == 0)
        && (pbio_user->bi_io_vec[0].bv_offset == 0)
        && (pbio_user->bi_io_vec[0].bv_len == PAGE_SIZE)) {
        for (i = 0; i < MEM_PER_FLASH_PAGE; i++) {
            pbioc->ar_ppage_swap[i] = pbioc->bio->bi_io_vec[i].bv_page;
            pbioc->bio->bi_io_vec[i].bv_page = pbio_user->bi_io_vec[i].bv_page;
        }
#if LFSM_RELEASE_VER == 3
        if (gcc) {
            strcpy(page_address(pbioc->bio->bi_io_vec[0].bv_page) + PAGE_SIZE -
                   32, str_license);
        }
#endif
    } else
#endif
    {
        bio_for_each_segment(bvec, pbio_user, iter) {
            i = iter.bi_idx;
            if (NULL == (pbio_user->bi_io_vec[i].bv_page)) {
                LOGERR(" bio->bi_io_vec[%d].bv_page=NULL bio=%p\n", i,
                       pbio_user);
                goto l_fail;
            }

            iovec_mem = kmap(bvec.bv_page) + bvec.bv_offset;
            if (NULL == (iovec_mem)) {
                LOGERR("kmap iovec_mem fail\n");
                goto l_fail;
            }

            if (ibvpage_RW(offset, iovec_mem, bvec.bv_len, ivcnt, pbioc, WRITE)
                != 0) {
                LOGERR("IS_ERR_VALUE(ibvpage_RW()) for WRITE\n");
                kunmap(bvec.bv_page);
                goto l_fail;
            }

            kunmap(bvec.bv_page);
            ivcnt++;
            offset += bvec.bv_len;
        }
    }
#if LFSM_RELEASE_VER == 3
    if (gcc) {
        memcpy(page_address(pbioc->bio->bi_io_vec[0].bv_page) + PAGE_SIZE - 32,
               str_license, 32);
    }
#endif
    return 0;

  l_fail:
    return -EIO;
}

int32_t iwrite_bio(lfsm_dev_t * td, struct bio *pbio_user)
{
    struct bio_container *pbioc;
    int32_t ret;

    pbioc = NULL;
    //LOGINFO("User bio bi_sector: %lu (pg %lu) sz %d vcnt %d\n",
    //    (unsigned long)pbio_user->bi_iter.bi_sector,(unsigned long)pbio_user->bi_iter.bi_sector>>SECTORS_PER_FLASH_PAGE_ORDER,
    //    (int)pbio_user->bi_iter.bi_size,(int)pbio_user->bi_vcnt);

    if (NULL == (pbioc = bioc_get_and_wait(td, pbio_user, REQ_OP_WRITE))) {
        return -ENOMEM;
    }

    if (bioc_handle_partial(td, pbioc, bioc_data_preload, NULL) < 0) {
        goto l_fail;
    }
#if LFSM_RELEASE_VER == 0
    if (iwrite_user_data_setup(td, pbioc, pbio_user) < 0)
#else
    if (read_metadata_all(td, pbioc, pbio_user) < 0)
#endif
    {
        goto l_fail;
    }

    ret = iwrite_bio_bh(td, pbioc, false);

    return ret;

  l_fail:
    lfsm_resp_user_bio_free_bioc(td, pbioc, -EIO, -1);
    return 0;
}

int32_t iwrite_bio_bh_ssd(lfsm_dev_t * td, struct bio_container *bio_c,
                          bool isRetry)
{
    int32_t ret;

    ret = bioc_submit_write(td, bio_c, isRetry);

    if (likely(ret == RET_BIOCBH)) {
        return 0;
    }
    //[CY] LFSM_ASSERT2(org_bio==bio_c->org_bio,"TF: org_bio %p, bio_c(%p) org %p\n",org_bio, bio_c,bio_c->org_bio);
    //[AN] 2012/9/19 the bio_c is returned in the get_dest_pbno_and_finish_write, so the bio_c is invalid here
    //if(org_bio!=bio_c->org_bio)
    //printk("Error: org_bio %p, bio_c(%p) org %p\n",org_bio, bio_c,bio_c->org_bio);
    goto l_fail;

  l_fail:
    if (ret != RET_BIOCBH && bio_c->write_type != TP_GC_IO) {

        //printk("response use bio %p \n",bio_c);
        if (!rddcy_end_stripe(td, bio_c)) {
            LOGERR("Fail to end stripe for bioc %p\n", bio_c);
        }

        if (ret == 0) {
            if (bioc_data_cache_update(td, bio_c) < 0) {
                LOGWARN("fail to data sync.  bioc %p", bio_c);
            }
        }
        lfsm_resp_user_bio_free_bioc(td, bio_c, ret, -1);
    }
    //bio_c->org_bio=0;
    return 0;
}

int32_t iwrite_bio_bh(lfsm_dev_t * td, struct bio_container *pbioc,
                      bool isRetry)
{
    return iwrite_bio_bh_ssd(td, pbioc, isRetry);
}

// 0 -> false, 1-> true <0-> error
static int32_t eu_has_to_pad(lfsm_dev_t * td, struct swrite_param *pwp,
                             struct HListGC *h, int32_t cur_temperature,
                             bool isDataPage)
{
    struct EUProperty *eu_orig;
    sector64 age_orig, age_curr;
    int32_t MustPad;

    MustPad = 0;

    if (td->cn_pgroup == 1) {
        return MustPad;
    }

    if (isDataPage && cur_temperature == (TEMP_LAYER_NUM - 1)) {
        if (NULL == (eu_orig = eu_find(pwp->pbno_orig))) {
            return PTR_ERR(eu_orig);
        }
        age_orig = HPEU_get_birthday(&td->hpeutab, eu_orig);
        age_curr =
            HPEU_get_birthday(&td->hpeutab, h->active_eus[cur_temperature]);

        MustPad =
            (PGROUP_ID(eu_orig->disk_num) != PGROUP_ID(pwp->id_disk)) &&
            (eu_orig->temper == TEMP_LAYER_NUM - 1) &&
            ((age_orig > age_curr) || ((age_orig == age_curr) &&
                                       HPEU_is_older_pbno(pwp->pbno_orig,
                                                          pwp->pbno_new)));
    }

    return MustPad;
}

static int32_t eu_alloc_page(lfsm_dev_t * td, struct swrite_param *pwp,
                             struct HListGC *h, int32_t cur_temperature,
                             bool isDataPage)
{
    struct EUProperty *active_eu;
    int32_t ret;

    ret = 0;

    if (NULL == (h->active_eus[cur_temperature]->act_metadata.mdnode)) {
        LOGERR("eu(%p) %llu active metadata is empty\n",
               h->active_eus[cur_temperature],
               h->active_eus[cur_temperature]->start_pos);
        return -EINVAL;
    }
    //LFSM_RMUTEX_LOCK(&h->whole_lock);
    active_eu = h->active_eus[cur_temperature];
    // check weather free EU is available BEFORE assigning any pbn to a bio
    // which consists of more than 1 pages and some of pages may be written to new EU
    if ((active_eu->log_frontier + SECTORS_PER_FLASH_PAGE) >
        (active_eu->eu_size - METADATA_SIZE_IN_SECTOR)) {
        //LFSM_RMUTEX_UNLOCK(&h->whole_lock);
        ret = -EBUSY;
    }
//    printk("%s: alloc lbno 0x%llx (%d,%d) active_eu (temp=%d dk=%d) ->log_frontier = %d\n",
//        __FUNCTION__, pwp->lbno,isDataPage,MustYoungPgroup,
//        active_eu->temper,active_eu->disk_num,active_eu->log_frontier);
    pwp->pbno_new = active_eu->log_frontier + active_eu->start_pos;
//    printk("%s: parity %d lpn= %llu pbn %llu\n",__FUNCTION__, RDDCY_IS_ECCLPN(pwp->lbno),pwp->lbno,pwp->pbno_new);

    if (active_eu->log_frontier + SECTORS_PER_FLASH_PAGE ==
        active_eu->eu_size - METADATA_SIZE_IN_SECTOR) {
        pwp->pbno_meta = h->active_eus[cur_temperature]->start_pos +
            h->active_eus[cur_temperature]->eu_size - METADATA_SIZE_IN_SECTOR;
        mpp_printk("metadata trigger pending_pbno %llu", pwp->pbno_meta);
    }

    switch (ret = eu_has_to_pad(td, pwp, h, cur_temperature, isDataPage)) {
    case 1:                    // must pad
//            printk("%s:  should pad write for lbno 0x%llx eu disk %d frontier %d\n",
//                __FUNCTION__,pwp->lbno,active_eu->disk_num,active_eu->log_frontier);
        ret = -EXDEV;
        break;
    case 0:                    // any one is ok
        mdnode_set_lpn(active_eu->act_metadata.mdnode,
                       pwp->lbno,
                       active_eu->log_frontier >> SECTORS_PER_FLASH_PAGE_ORDER);
//      if (isDataPage)               // move to bocbh
//          set_PBN_bitmap_validcount(active_eu->start_pos + active_eu->log_frontier);

        active_eu->log_frontier += SECTORS_PER_FLASH_PAGE;
        break;
    default:                   //< 0
        break;
    }

    //LFSM_RMUTEX_UNLOCK(&h->whole_lock);
    return ret;
}

int32_t get_dest_pbno(lfsm_dev_t * td, struct lfsm_stripe *pstripe,
                      bool isValidData, struct swrite_param *pwp)
{
    struct HListGC *h;
    int32_t cur_temperature, ret, *map;

    LFSM_ASSERT(pwp != NULL);
    pwp->pbno_new = 0;
    pwp->pbno_meta = 0;
    if ((cur_temperature = lfsm_pgroup_get_temperature(td, pstripe)) < 0) {
        return cur_temperature; // fail value
    }

    map =
        rddcy_get_stripe_map(td->stripe_map, atomic_read(&pstripe->idx),
                             td->hpeutab.cn_disk);
    pwp->id_disk = map[atomic_read(&pstripe->wio_cn)] + (pstripe->id_pgroup * lfsmdev.hpeutab.cn_disk); // atomic_read(&wio_cn[cur_temperature])%LFSM_DISK_NUM;

    //if(diskgrp_isfaildrive(&grp_ssds,pwp->id_disk)==true)
    //    return -ENODEV;    handled by make_request=-1

    h = gc[pwp->id_disk];
    if ((ret = eu_alloc_page(td, pwp, h, cur_temperature, isValidData)) < 0) {
        return ret;
    }

    if (atomic_read(&pstripe->wio_cn) == 0) {
        if (pwp->id_disk % td->hpeutab.cn_disk != 0) {
            h = gc[HPEU_ID_FIRST_DISK(pwp->id_disk)];
            pstripe->start_pbno = h->active_eus[cur_temperature]->log_frontier +
                h->active_eus[cur_temperature]->start_pos;
        } else
            pstripe->start_pbno = pwp->pbno_new;
    }

    stripe_commit_disk_select(pstripe);

    if (atomic_read(&pstripe->wio_cn) == 0) {
        if (atomic_inc_return(&pstripe->idx) >=
            FPAGE_PER_SEU - METADATA_SIZE_IN_FLASH_PAGES)
            atomic_set(&pstripe->idx, 0);
    }
    return ret;
}

static int32_t chk_and_alloc_eus(lfsm_dev_t * td, int32_t id_start_disk,
                                 int32_t cur_temperature, bool skip_chk,
                                 int32_t * ret_id_wait_disk)
{
    struct HListGC *h;
    struct EUProperty *active_eu;
    int32_t i;

    *ret_id_wait_disk = -1;
    if ((id_start_disk % td->hpeutab.cn_disk) != 0) {
        return 0;
    }

    ersmgr_wakeup_worker(&td->ersmgr);  //ding
    for (i = 0; i < td->hpeutab.cn_disk; i++) {
        h = gc[id_start_disk + i];
        *ret_id_wait_disk = id_start_disk + i;

        LFSM_RMUTEX_LOCK(&h->whole_lock);

        active_eu = h->active_eus[cur_temperature];
        // check weather free EU is available BEFORE assigning any pbn to a bio
        if (active_eu->log_frontier + SECTORS_PER_FLASH_PAGE >
            active_eu->eu_size - METADATA_SIZE_IN_SECTOR) {
            //if (i!=starti) // debug code
            //    printk("Info@%s: Cont to alloc eu from idx %d\n",__FUNCTION__, i);
            if ((skip_chk == false) && (h->free_list_number <= MIN_RESERVE_EU)) {
                goto l_fail_ret;
            }

            if (NULL == (change_active_eu(td, h, cur_temperature))) {
                goto l_fail_ret;
            }
            //starti=i+1; // debug code

        }
        LFSM_RMUTEX_UNLOCK(&h->whole_lock);
    }
    return 0;

  l_fail_ret:
    LFSM_RMUTEX_UNLOCK(&h->whole_lock);
    td->ar_pgroup[PGROUP_ID(id_start_disk)].min_id_disk = *ret_id_wait_disk;
    return -EAGAIN;
}

int32_t bioext_locate(bio_extend_t * pbioext, struct bio *pbio, int32_t len)
{
    int32_t i;
    for (i = 0; i < len; i++) {
        if (pbioext->items[i].bio == pbio) {
            return i;
        }
    }
    return -ESRCH;
}

void bioext_free(bio_extend_t * pbioext, int32_t len)
{
    int32_t i, j;

    for (i = 0; i < len; i++) {
        // weafon: free the addition page for iosch spare page...
        if (NULL !=
            (pbioext->items[i].bio->bi_io_vec[MEM_PER_FLASH_PAGE].bv_page)) {
            track_free_page(pbioext->items[i].bio->
                            bi_io_vec[MEM_PER_FLASH_PAGE].bv_page);
        }

        for (j = 0; j < MEM_PER_FLASH_PAGE + 1; j++) {
//            printk("%s: set bv_page %p to null\n",__FUNCTION__,&pbioext->item[i].bio->bi_io_vec[j].bv_page);
            pbioext->items[i].bio->bi_io_vec[j].bv_page = NULL;
        }
        track_bio_put(pbioext->items[i].bio);
    }
    track_kfree(pbioext, sizeof(bio_extend_t) + sizeof(bioext_item_t) * len,
                M_OTHER);
}

/*
 * TODO: why the alloc size is weird?
 */
static int32_t bioext_alloc(lfsm_dev_t * td, struct bio_container *bio_c,
                            int32_t cn_fpage)
{
    bioext_item_t *pextitem;
    int32_t i, j, cn_retry;

    cn_retry = 0;
    bio_c->par_bioext = (bio_extend_t *)
        track_kmalloc(sizeof(bio_extend_t) + sizeof(bioext_item_t) * cn_fpage,
                      __GFP_ZERO, M_OTHER);
    if (NULL == (bio_c->par_bioext)) {
        return -ENOMEM;
    }

    atomic_inc(&td->cn_alloc_bioext);
    bio_c->par_bioext->bio_orig = bio_c->bio;
    for (i = 0; i < cn_fpage; i++) {
        pextitem = &bio_c->par_bioext->items[i];
        compose_bio_no_page(&pextitem->bio, NULL, NULL, bio_c,
                            (MEM_PER_FLASH_PAGE + 1) * PAGE_SIZE,
                            (MEM_PER_FLASH_PAGE + 1));
        if (NULL == (pextitem->bio)) {
            return -ENOMEM;
        }

        bio_lfsm_init(pextitem->bio, bio_c, NULL, NULL, REQ_OP_WRITE, 0,
                      MEM_PER_FLASH_PAGE);
        for (j = 0; j < MEM_PER_FLASH_PAGE; j++) {
            pextitem->bio->bi_io_vec[j] =
                bio_c->bio->bi_io_vec[i * MEM_PER_FLASH_PAGE + j];
        }
        pextitem->bio->bi_io_vec[MEM_PER_FLASH_PAGE].bv_len = 4096;
        pextitem->bio->bi_io_vec[MEM_PER_FLASH_PAGE].bv_offset = 0;
        cn_retry = 0;
        do {
            pextitem->bio->bi_io_vec[MEM_PER_FLASH_PAGE].bv_page =
                track_alloc_page(__GFP_ZERO);
            if (cn_retry > 0) {
                LOGWARN("cn_retry = %d\n", cn_retry);
            }

            if (cn_retry++ > LFSM_MIN_RETRY) {
                return -ENOMEM;
            }
        } while (NULL == pextitem->bio->bi_io_vec[MEM_PER_FLASH_PAGE].bv_page);

        bio_c->bio->bi_vcnt -= MEM_PER_FLASH_PAGE;  // weafon: since page has been move to bioext, so bi_cnt -- in loop
        pextitem->pbn_dest = pextitem->pbn_source = -1;
    }

    return 0;
}

static int32_t get_dest_pbno_and_finish_write(lfsm_dev_t * td,
                                              struct bio_container *bio_c,
                                              bool isRetry, int32_t idx_fpage,
                                              bool isDataPage)
{
    struct swrite_param wparam[WPARAM_CN_IDS];
    struct swrite_param *pm_act;
    struct lfsm_stripe *cur_stripe;
    int32_t id_pgroup, delta, ret, cur_temperature, id_wait_disk;

    delta = 1;
    ret = 0;
    memset(wparam, 0, sizeof(struct swrite_param) * WPARAM_CN_IDS);

  begin_get_dest_pbno:
    //LFSM_ASSERT2(bio_c->bio->bi_vcnt == MEM_PER_FLASH_PAGE,"FAIL: bioc(%p) lpn %lld; org_bio(%p) org sector: %llu size: %d\n",
    //    bio_c, bio_c->start_lbno, bio_c->org_bio, (sector64)bio_c->org_bio->bi_iter.bi_sector, bio_c->org_bio->bi_iter.bi_size);
    pm_act = &wparam[WPARAM_ID_DATA];
    pm_act->lbno = bio_c->start_lbno + idx_fpage;
    if (bio_c->write_type == TP_GC_IO) {
        delta = -1;
    } else if (isRetry) {
        delta = 0;
    }

    if ((cur_temperature =
         get_temperature(td, &(pm_act->pbno_orig), 1, pm_act->lbno,
                         delta)) < 0) {
        LOGERR("cur_temperature %d \n", cur_temperature);
        goto l_fail;
    }

    if ((id_pgroup = lfsm_pgroup_selector(td->ar_pgroup, cur_temperature,
                                          pm_act->pbno_orig, bio_c,
                                          isRetry)) < 0) {
        LOGERR(" pg damage, id_pgroup= %d \n", id_pgroup);
        goto l_fail;
    }
//    if (isRetry)
//        printk("%s: retry bio_c->start_lbno %lld prev_pbno %lld temp %d    pgroup %d\n",
//            __FUNCTION__,pm_act->lbno,pm_act->pbno_orig,cur_temperature,id_pgroup);

    if (NULL ==
        (cur_stripe =
         lfsm_pgroup_stripe_lock(td, id_pgroup, cur_temperature))) {
        LOGERR(" stript lock fail id_group %d \n", id_pgroup);
        goto l_fail;
    }

    if (unlikely((ret =
                  chk_and_alloc_eus(td, stripe_current_disk(cur_stripe),
                                    cur_temperature,
                                    bio_c->write_type == TP_GC_IO,
                                    &id_wait_disk)) < 0)) {
        LFSM_MUTEX_UNLOCK(&cur_stripe->lock);   // locked by lfsm_pgroup_stripe_lock
        //  isDataPage==false: the WRITE of padding page from "4k->16k" can be give up when free eu is run out.
        if (bio_c->write_type == TP_GC_IO || isDataPage == false) {
            goto l_fail;
        }

        if (wait_free_eu(td, bio_c, gc[id_wait_disk]) == false) {
            goto l_fail;
        }

        if ((bio_c->par_bioext) && (idx_fpage != 0)) {
            bio_c->par_bioext->items[idx_fpage].pbn_dest = LBNO_REDO_PAGE;
            bio_c->bio->bi_status = BLK_STS_AGAIN;
            bio_end_pipeline_write(bio_c->bio);
            return RET_BIOCBH;
        } else {
            goto begin_get_dest_pbno;
        }
    }

    if (IS_ERR(pm_act = rddcy_wparam_setup(td, cur_stripe, pm_act,
                                           &wparam[WPARAM_ID_PREPAD], isRetry,
                                           bio_c, cur_temperature, idx_fpage,
                                           isDataPage))) {
        if (PTR_ERR(pm_act) == -EAGAIN) {
            LFSM_MUTEX_UNLOCK(&cur_stripe->lock);   // locked by lfsm_pgroup_stripe_lock
            goto begin_get_dest_pbno;
        } else {
            goto l_fail_unlock;
        }
    }

    if ((ret = rddcy_wparam_setup_helper(td, cur_stripe, pm_act,
                                         &wparam[WPARAM_ID_ECC],
                                         idx_fpage)) < 0) {
        goto l_fail_unlock;
    }

    LFSM_MUTEX_UNLOCK(&cur_stripe->lock);   // locked by lfsm_pgroup_stripe_lock

    return rddcy_write_submit(td, wparam);  // no matter pass or fail,
    //put_bio_container and post_rw_check_wait_list is executed inside rddcy_write_submit

  l_fail_unlock:
    LFSM_MUTEX_UNLOCK(&cur_stripe->lock);
  l_fail:
    //move to toplest level
    //if(bio_c->write_type!=TP_GC_IO)
    //{
    //    post_rw_check_waitlist(td, bio_c,1);
    //    put_bio_container(td, bio_c);
    //}
    return -EIO;
}

int32_t bioc_submit_write(lfsm_dev_t * td, struct bio_container *bio_c,
                          bool isRetry)
{
//    bool isRetry;
    int32_t ret, final_ret, i, cn;
    bool is_last_one;

    cn = bioc_cn_fpage(bio_c);

    //LOGINFO("bioc %p ####cn %d### %lld ~ %lld bi_vcnt=%d\n",bio_c,cn,bio_c->start_lbno,
    //            bio_c->end_lbno, bio_c->bio->bi_vcnt);
    ret = 0;
    final_ret = 0;
    is_last_one = false;

    if (cn > 1) {
        ret = bioext_alloc(td, bio_c, cn);
    }
    if (ret < 0) {
        goto l_fail;
    }

    for (i = 0; i < cn; i++) {
        //isRetry=false;
        if (unlikely(bio_c->par_bioext)) {
            bio_c->bio = bio_c->par_bioext->items[i].bio;   // FIXME: par_bioext[0] is the second bio
        }

        do {
            ret = get_dest_pbno_and_finish_write(td, bio_c, isRetry, i, true);  //possible value REGET_DESTPBNO , RET_BIOBH, -EIO, 0
            isRetry = true;
        } while (ret == REGET_DESTPBNO);

        // we cannot do this bio, end all
        //(1) set this bioc status = -EIO
        //(2) minus cn_wait all remaining count
        if (unlikely(ret < 0)) {
            bio_c->bio->bi_status = BLK_STS_IOERR;
            end_bio_write(bio_c->bio);
            if (atomic_sub_and_test(cn - i, &bio_c->cn_wait)) {
                final_ret = -EIO;
                is_last_one = true;
            }
            break;
        }
        // handling by front end(0)
        if (unlikely(ret == 0)) {
            if (atomic_dec_and_test(&bio_c->cn_wait)) {
                final_ret = RET_BIOCBH;
                is_last_one = true;
                biocbh_io_done(&td->biocbh, bio_c, true);   // let bh to handle
            }
        }
    }

  l_fail:
    if (is_last_one) {
        return final_ret;       //  -EIO or biocbh
    } else {
        return RET_BIOCBH;
    }
}

static void bio_dump(struct bio *pbio, int8_t * nm_bio)
{
    LOGINFO("nm %s bio %p bi_sector %lld bi_size %d vcnt %d\n",
            nm_bio, pbio, (sector64) pbio->bi_iter.bi_sector,
            pbio->bi_iter.bi_size, pbio->bi_vcnt);
    return;
}

void bioc_dump(struct bio_container *pbioc)
{
    LOGINFO("pid %d pbioc %p lpn %lld %lld \n", current->pid,
            pbioc, pbioc->start_lbno, pbioc->end_lbno);
    bio_dump(pbioc->bio, "out bio");
    bio_dump(pbioc->bio, "orig bio");
    return;
}

struct swrite_param *rddcy_wparam_setup(lfsm_dev_t * td, struct lfsm_stripe *cur_stripe, struct swrite_param *pm_data, struct swrite_param *pm_prepad, bool isRetry, struct bio_container *bio_c, int32_t temper, int32_t idx_fpage, bool isDataPage)   //tifa
{
    int32_t ret;

    switch (ret = get_dest_pbno(td, cur_stripe, isDataPage, pm_data)) {
    case 0:
        pm_data->bio_c = bio_c;
        pm_data->bio_c->dest_pbn = pm_data->bio_c->bio->bi_iter.bi_sector =
            pm_data->pbno_new;
        pm_data->bio_c->source_pbn = pm_data->pbno_orig;    // lfsm test
        if (bio_c->par_bioext) {
            bio_c->par_bioext->items[idx_fpage].pbn_source = pm_data->pbno_orig;
            bio_c->par_bioext->items[idx_fpage].pbn_dest = pm_data->pbno_new;
        }
        return pm_data;
    case -EBUSY:
        LOGERR("fail to get pbno %d \n", ret);
        return ERR_PTR(-EBUSY);

    case -EXDEV:
//        if (!isRetry)
//        {
//            LOGINFO("Reget pbno to prevent padding\n");
//            return ERR_PTR(-EAGAIN);
//        }
        atomic_inc(&lfsm_pgroup_cn_padding_page);
        //atomic_add(LFSM_PGROUP_NUM-1, &lfsm_arpgroup_wcn[temper]); //Retry io no need reset back to it's group
        if (!rddcy_gen_bioc(td, cur_stripe, pm_prepad, false)) {    //add prepad bioc
            return ERR_PTR(-EBUSY);
        }

        pm_prepad->bio_c->dest_pbn = pm_prepad->bio_c->bio->bi_iter.bi_sector =
            pm_prepad->pbno_new;
        pm_prepad->bio_c->source_pbn = -1;  // lfsm test
        return pm_prepad;

    default:
        LOGERR(" invalid ret %d \n", ret);
        return ERR_PTR(ret);
    }
}

int32_t rddcy_wparam_setup_helper(lfsm_dev_t * td,
                                  struct lfsm_stripe *cur_stripe,
                                  struct swrite_param *pm_act,
                                  struct swrite_param *pm_ecc,
                                  int32_t idx_fpage)
{
    int32_t ret, maxcn_data_disk;

    ret = -EPERM;
    maxcn_data_disk = hpeu_get_maxcn_data_disk(td->hpeutab.cn_disk);

    if (!rddcy_xor_flash_buff_op
        (cur_stripe, pm_act->bio_c->bio->bi_io_vec, pm_act->lbno)) {
        ret = -ENOMEM;
        goto l_fail;
    }
    if (pm_act->bio_c->par_bioext) {
        pm_act->bio_c->par_bioext->items[idx_fpage].pstripe_wctrl =
            cur_stripe->wctrl;
    }

    pm_act->bio_c->pstripe_wctrl = cur_stripe->wctrl;

    rddcy_printk("set_bioc %p control %p\n", pm_act->bio_c,
                 pm_act->bio_c->pstripe_wctrl);

    atomic_inc(&cur_stripe->wctrl->cn_allocated);
    if (atomic_read(&cur_stripe->wio_cn) == maxcn_data_disk)    // Full stripe 
    {
        if (rddcy_gen_bioc(td, cur_stripe, pm_ecc, true) == false)
            goto l_fail;

        return 1;               // is ECC
    }

    return 0;
  l_fail:
    LOGERR("err=%d\n", ret);
    return ret;
}

int32_t rddcy_write_submit(lfsm_dev_t * td, struct swrite_param *pwp)
{
    int32_t ret[WPARAM_CN_IDS], i;

    int32_t(*ar_func_submit[WPARAM_CN_IDS]) (lfsm_dev_t * td,
                                             struct swrite_param * pwp)
        = { bioc_write_submit_prepad, bioc_write_submit_data,
        bioc_write_submit_ecc
    };                          // HOTWL: add new prepad's write_submit

    for (i = 0; i < WPARAM_CN_IDS; i++) {
        if (NULL == pwp[i].bio_c) {
            continue;
        }
// HOTWL: return REGET_DESTPBNO if it is a PREPAD case.
        ret[i] = ar_func_submit[i] (td, &pwp[i]);
    }

// handling return value

    for (i = 0; i < WPARAM_CN_IDS; i++) {
        if (NULL == pwp[i].bio_c) {
            continue;
        }
        return ret[i];
    }
    LOGERR("all bioc == null \n");
    return -EIO;
}

/**
 ** For a write request, allocate an extra page
 ** and fill the LPNs in the last page. 
 **
 ***/
// weafon : 2011/1/20
// return value -EIO,0
int32_t update_bio_spare_area(struct bio_container *bio_ct, sector64 * ar_lbn,
                              int32_t num_lbns)
{
    int32_t i;

    struct bio_vec bvec;
    struct bvec_iter iter;

    struct bio *bio = bio_ct->bio;
    bio_for_each_segment(bvec, bio, iter) {
        i = iter.bi_idx;
        if (unlikely((i / MEM_PER_FLASH_PAGE) >= num_lbns)) {
            LOGWARN("(i/MEM_PER_FLASH_PAGE)>=num_lbns (%d)\n", num_lbns);
            return 0;
        }

        if (unlikely((((unsigned long long)
                       ar_lbn[i /
                              MEM_PER_FLASH_PAGE]) & LFSM_ALLOC_PAGE_MASK) !=
                     0)) {
            LOGERR("ar_lbn[%d]&LFSM_ALLOC_PAGE_MASK!=0 ar_lbn=%llu\n",
                   i / MEM_PER_FLASH_PAGE, ar_lbn[i / MEM_PER_FLASH_PAGE]);
            return -EIO;
        }
    }
    return 0;
}
