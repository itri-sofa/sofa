/*
 * SPDX-License-Identifier: GPL-2.0-or-later OR Apache-2.0
 * Copyright (C) 2015-2020 Industrial Technology Research Institute
 *
 *
 *
 *  
 */
#ifndef _LFSM_H
#define _LFSM_H

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
#include <linux/types.h>
#include "ioctl_A.h"

#define LFSM_WAIT_EVEN_TIMEOUT_SEC  30
#define LFSM_WAIT_EVEN_TIMEOUT  LFSM_WAIT_EVEN_TIMEOUT_SEC*HZ   //for wait_event_interruptible_timeout JIRA FSS-86

#define LFSM_MAX_RETRY 10
#define LFSM_MIN_RETRY 3

typedef enum {
    LFSM_STAT_CLEAN = 0,        /* after lfsm_exit, mean lfsm already cleanup */
    LFSM_STAT_NORMAL = 1,       /* lfsm start to services */
    LFSM_STAT_REJECT = 2,       /* lfsm start to cleanup */
    LFSM_STAT_INIT = 3,         /* lfsm initialize */
    LFSM_STAT_READONLY = 4,     /* lfsm enter read only state */
} lfsm_stat_t;

//#define mr_printk(...)  printk("[mr PID=%d] ",current->pid);printk(__VA_ARGS__)
#define mr_printk(...)

//#define mp_printk(...)  printk("[PID=%d] ",current->pid);printk(__VA_ARGS__)
#define mp_printk(...)

//#define err_handle_printk(...)  {printk("eh[PID=%d]:",current->pid);printk(__VA_ARGS__);};
#define err_handle_printk(...) {};
#define assert_printk(...)  {printk("Assert[%d]:%s:%d",current->pid,__FUNCTION__,__LINE__);printk(__VA_ARGS__);};

//#define mpp_printk(...)  printk("[PID=%d] ",current->pid);printk(__VA_ARGS__)
#define mpp_printk(...)

//#define rddcy_printk(...) {printk("[PID=%d] ",current->pid);printk(__VA_ARGS__);}
#define rddcy_printk(...)

//#define dprintk(fmt, args...) printk(KERN_NOTICE fmt, ##args)
#define dprintk(fmt, args...)

//#define tf_printk(fmt, args...) printk(KERN_NOTICE fmt, ##args)
#define tf_printk(fmt, args...)

// weafon : 2011/1/20
//TODO: related to old version lfsm spare area. remove it
#define LFSM_ALLOC_PAGE_MASK (0xFFFE000000000000ULL)

extern int32_t assert_flag;
extern atomic_t cn_paused_thread;
extern struct diskgrp grp_ssds;
extern struct HListGC **gc;
extern struct EUProperty **pbno_eu_map;
extern sector64 eu_log_start;
extern bool fg_gc_continue;

#define IS_GC_THREAD(cur_pid) (((lfsmdev.partial_io_thread!=NULL) && (cur_pid==lfsmdev.partial_io_thread->pid))    || cur_pid==pid_ioctl)

#if LFSM_REVERSIBLE_ASSERT == 1
#define LFSM_ASSERT(condition) {}
#define LFSM_ASSERT2(condition, fm, x...) {}
#define LFSM_SHOULD_PAUSE()      \
        if (unlikely(assert_flag == 1)) {            \
            printk("[PID: %d] Pause due to an assertion , %d thread paused, %d thread should pause\n", \
                    current->pid,lfsm_atomic_inc_return(&cn_paused_thread), lfsmCfg.cnt_io_threads+1);\
            while (wait_event_interruptible(lfsmdev.wq_assert,assert_flag==0)!=0) schedule(); \
            printk("[PID: %d] Resume from an assertion\n",current->pid); atomic_dec(&cn_paused_thread);\
        }
#else

#define LFSM_ASSERT(condition)                                          \
    if (!(condition)) {                                                \
            assert_flag =1;\
            printk("Assertion failed: line %d, file \"%s\"\n",            \
                            __LINE__, __FILE__);                          \
            BUG_ON(1);\
\
    }
#define LFSM_ASSERT2(condition,fmt,x...) \
        if (!(condition)) {                                             \
                assert_flag=1;\
                printk("Assertion failed: line %d, file \"%s\"\n",              \
                                __LINE__, __FILE__);                          \
                printk("Value: " fmt " \n",x);    \
                BUG_ON(1);\
    \
        }
#endif

extern int32_t lfsm_run_mode;
#define LFSM_FORCE_RUN(action) \
    do{ \
    if (lfsm_run_mode==0) {} \
    else if (lfsm_run_mode==1) {action;} \
    else BUG_ON(1); \
    }while(0)

#define DEV_SYS_READY(grp,id) (((id<0)&&(id>=lfsmdev.cn_ssds))?0:(grp.ar_subdev[id].load_from_prev!=non_ready)) //ding maybe dirty

/* 
** BMT record
*/
// renamed as D_BMT_E

/* 
** Magic Sector 
*/
enum edev_state { non_ready = 0xB3, sys_ready = 0x4A, all_ready = 0xA5, error =
        0xff };

struct lfsm_dev_struct;
struct bio_container;

typedef struct lfsm_dev_struct lfsm_dev_t;
/*
** Subdisk Structure
*/
typedef struct lfsm_subdevice {
    sector64 log_start;         /* starting address for non-super map data,
                                   sector base. initial value size of EU in sector */

    /////tf: super map information/////
    enum edev_state load_from_prev; /* prev shutdown state */
    struct bio *super_bio;      /* pre alloc bio fro R/W/log super map */
    int32_t super_flag;         /* control the thread which want R/W/log super map */
    wait_queue_head_t super_queue;  /* result of R/W/log super map */
    uint64_t super_sequence;    /* entry sequence number (how to handle when overflow?) */
    sector64 super_frontier;    /* Sector position of lastest record */
    struct EUProperty **Super_Map;  /* EU structure of super map */
    int32_t Super_num_EUs;      /* num of EUs for supermap in a disk */
    struct mutex supmap_lock;   /* protect the access of super map */
    int32_t cn_super_cycle;

    struct sbrm *pbrm;          /* for batch read, not use now */
} lfsm_subdev_t;

struct HListGC;

//TODO: purpose unknown, where to put?
#define MAX_COPY_RUNLEN          64
#define BIO_SIZE_ALIGN_TO_FLASH_PAGE(bio)   \
    (LFSM_CEIL((bio->bi_iter.bi_sector << SECTOR_ORDER) + bio->bi_iter.bi_size, FLASH_PAGE_SIZE)- \
        LFSM_FLOOR(bio->bi_iter.bi_sector << SECTOR_ORDER, FLASH_PAGE_SIZE))
//(((bio->bi_iter.bi_size + ((bio->bi_iter.bi_sector - BIO_SECTOR_ALIGN_TO_FLASH_PAGE(bio))<<SECTOR_ORDER) + FLASH_PAGE_SIZE - 1)>>FLASH_PAGE_ORDER)<<FLASH_PAGE_ORDER)

/*
** LFSM device Structure
*/

#include "dbmtapool.h"
#include "lfsm_info.h"
#include "nextprepare.h"
#include "autoflush.h"
#include "sflush.h"
#include "hpeu.h"
#include "ecc.h"
#include "xorbmp.h"
#include "stripe.h"
#include "pgroup.h"
#include "pgcache.h"
#include "erase_manager.h"
#include "hpeu_gc.h"
#include "metalog.h"
#include "mdnode.h"
#include "hotswap.h"
#include "hook.h"
#include "spare_bmt_ul.h"

typedef struct lfsm_thread {
    struct task_struct *pthread;
    wait_queue_head_t wq;
} lfsm_thread_t;

struct lfsm_dev_struct {
    atomic_t dev_stat;          /* lfsm driver state */
    MD_L2P_table_t *ltop_bmt;   /* LBN to PPN Mapping Table */
    SUpdateLog_t UpdateLog;     /* BMT_update_log data structure */
    SDedicatedMap_t DeMap;      /* Dedicated map used to storing metadata */
    struct external_ecc *ExtEcc;    /* External ECC page */
    enum elfsm_state prev_state;    /* indicate whether normal shutdown in previous stop lfsm */

    //spinlock_t       queue_lock;   /* linux gdisk request queue lock */
    struct gendisk *disk;       /* alloc_disk -> add_disk -> put_disk -> del_gendisk */
    int32_t lfsm_major;         /* device ID more /proc/device */
    sector64 logical_capacity;  /* lfsm capacity in pages = sum of each disk - reserved EUs for system and bad block */
    sector64 physical_capacity; //Lego: todo redundant data member 2016/01/22 remove cause perf. down??
    uint32_t super_map_primary_disk;    //primary disk for storing super map, first healthy one in grpssds
    sector64 start_sector;      /* the starting address of 1st disk + size of super map, used to prevent bmt access supermap */

    atomic_t cn_extra_bioc;     /* how many extra bioc we allocated
                                   (not allocated in initialization)
                                   when bioc list is empty (datalog_free_list).
                                   We won't free extra alloc unless
                                   user issue ioctl commands */
    wait_queue_head_t wq_datalog;   /* pause thread when ran of bioc */
    struct list_head datalog_free_list[DATALOG_GROUP_NUM];  /* bioc pool */
    atomic_t len_datalog_free_list; /* # of free bioc in pool */
    struct list_head gc_bioc_list;  /* only insert gc bioc */
    spinlock_t datalog_list_spinlock[DATALOG_GROUP_NUM];    /* pool lock */
    uint64_t freedata_bio_capacity; /* # of free bioc at initialization */
    uint64_t data_bio_max_size; /* MAX number of pages in our bio = 1 (default setting) */
    /* For kernel process */
    struct tasklet_struct io_task;  //Lego: todo redundant data member 2016/01/22 remove cause perf. down??
    atomic_t io_task_busy;      //Lego: todo redundant data member 2016/01/22 remove cause perf. down??

    /* For partial I/O thread */
    struct task_struct *partial_io_thread;
    wait_queue_head_t worker_queue; /*  control lfsm_bg_thread */

    wait_queue_head_t gc_conflict_queue;    /* pause thread when less free eu */
    int32_t per_page_pool_cnt;  /* how many free bucket */
    struct list_head per_page_pool_list;    /* free bucket pool */
    struct mutex per_page_pool_list_lock;   //Lego: todo redundant data member 2016/01/22 remove cause perf. down??
    spinlock_t per_page_pool_lock;  /* pool lock */

    struct kmem_cache *pMdpool; /* linux kernel kmem_cache pool for mdnode */
    dbmtE_bk_pool_t dbmtapool;  /* linux kernel kmem_cache pool for dbmtE_bucket_t */

    bioboxpool_t bioboxpool;    /* biobox pool for all user IOs. shared by all threads */
    biocbh_t biocbh;            /* used for lfsm_bh when user IO done */
    atomic_t cn_conflict_pages_r;   /* how many bioc of read IO hit conflict, but never decrease */
    atomic_t cn_conflict_pages_w;   /* how many bioc of write IO hit conflict, but never decrease */
    SProcLFSM_t procfs;         /* proc directory */
    sflushops_t crosscopy;      /* background copy validate page bmt when GC */
    sflushops_t gcwrite;        /* background copy payload when GC? */
    struct HPEUTab hpeutab;     /* data structure for hpeu table */
    spgroup_t *ar_pgroup;       //[LFSM_PGROUP_NUM]; /* data structure for protect group*/
    atomic_t cn_alloc_bioext;   /* counting info of bio_c->par_bioext when alloc, TODO: no decrease */
    spgcache_t pgcache;         /* data structure for page cache */
    struct mutex special_query_lock;    //Lego: todo redundant data member 2016/01/22 remove cause perf. down??
    atomic64_t tm_sum_bioc_life;    //Lego: todo redundant data member 2016/01/22 remove cause perf. down??
    atomic64_t cn_sum_bioc_life;    //Lego: todo redundant data member 2016/01/22 remove cause perf. down??
    ersmgr_t ersmgr;            /* data structure for erase manager */
    smetalogger_t mlogger;      /* data structure for meta log */

    lfsm_thread_t monitor;      /* background thread for idle monitor */
#if LFSM_REVERSIBLE_ASSERT == 1
    wait_queue_head_t wq_assert;    /* Use this wait_queue to pause lfsm when
                                       something wrong.
                                       Developer can wakeup through ioctl enable_disk_io: 999 */
#endif

    struct shotswap hotswap;    /* data structure for hotswap */

    int32_t cn_ssds;            /* total number of working ssds (not including spare ssds) */
    int32_t cn_pgroup;          /* # of protection group (# of raid group) */

    atomic_t cn_flush_bio;      /* counting how many flush IOs received */
    atomic_t cn_trim_bio;       /* counting how many trim IOs received */

    struct stripe_disk_map *stripe_map;

    shook_t hook;               // for debug now
};

struct bio_container;

typedef struct per_page_active_list_item {
    sector64 lbno;
    int32_t len;
    int32_t offset;
    struct page *ppage;
    int32_t ready_bit;
/*
** For partial write:
** 0 means that its net payload needs to come from a preceding conflicting write
** 1 means that the write is ready to go
** 2 means that its net payload needs to be read from the disk
** For normal write:
** 0 means the write conflicts with a preceding write
** 1 means that the write is ready to go
** For GC write:
** 0 means that the write conflicts with a preceding write
** 1 means that thte write is ready to go
** 2 means that the write payload should be read from the disk
*/
    struct list_head page_list;
} ppage_activelist_item_t;

typedef struct bioext_item {
    struct bio *bio;
    struct lfsm_stripe_w_ctrl *pstripe_wctrl;
    sector64 pbn_dest;
    sector64 pbn_source;
} bioext_item_t;

typedef struct bio_extend {
    struct bio *bio_orig;
    bioext_item_t items[50];
} bio_extend_t;

/*
 * NOTE: 20160908-Lego
 * Currently, the max size of a bio is 64K which corresponding to 16 4K
 * So a bio_container might belong to multiple ppq bucket and max size is 2
 * In current conflict detection mechanism, if a bioc contains a conflict lpn with
 * previous lpn, it will add arhook_conflict to sconflict of ppq bucket.
 * And also set the corresponding bit in conflict_pages. (We use it as a bit vector)
 * bit 0 ~ bit 15: bucket 0
 * bit 16 ~ bit 32: bucket 1
 * if conflict_pages = 0, means no conflict now
 */
typedef enum {
    FG_IO_NO_PEND = 0,
    FG_IO_WAIT_FREESEU,
} io_pending_t;

#define  MAX_CONFLICT_HOOK    2
typedef struct bio_container {
    struct list_head list;
    struct list_head arhook_conflict[MAX_CONFLICT_HOOK];
    io_pending_t fg_pending;
    struct bio *bio;
    bio_extend_t *par_bioext;

    uint64_t bytes_submitted;
    struct bio *org_bio;
    wait_queue_head_t io_queue;
    int32_t io_queue_fin;       /* bio io result 1 io ok, other io fail */
    int32_t write_type;         // 0 for normal write. 1 for partial write. 2 for GC write
    int32_t io_rw;
    struct list_head wait_list;
    sector64 start_lbno, end_lbno;
    struct per_page_active_list_item *per_page_list[MAX_FLASH_PAGE_NUM + 1];
    struct per_page_active_list_item ppl_local;
    atomic_t conflict_pages;    //bit 0 ~ bit 15 for counting the conflict pages of ppq[bucket index % 2]
    //bit 16 ~ bit 31 for counting the conflict pages of ppq[bucket index % 2]
    //example: if lpn 1022 and lpn 1023 is conflict, then value of conflict = 0x20000
    //         if lpn 1024 and lpn 1025 is conflict, then value of conflict = 2
    lfsm_dev_t *lfsm_dev;
    struct timeval tm_start;
    sector64 source_pbn;
    sector64 dest_pbn;
    blk_status_t status;
    atomic_t active_flag;       //This field indicates status of the IO.
#if LFSM_PAGE_SWAP == 1
    struct page *ar_ppage_swap[MEM_PER_FLASH_PAGE];
#endif
    // struct mutex mutex_clean_wait_list;  // parameter for locking the status of item->active_flag
    spinlock_t clean_waitlist_spinlock;
    struct list_head submitted_list;
    atomic_t cn_wait;           // read 4kb done
    struct lfsm_stripe_w_ctrl *pstripe_wctrl;
#if LFSM_REDUCE_BIOC_PAGE == 1
    int32_t cn_alloc_pages;
#endif
    bool is_user_bioc;
    //int32_t bioc_req_id;
    int32_t selected_diskID;    //which disk id to be selected to submit IO. FIXME: fail handle mulitple pages
    int64_t ts_submit_ssd;      //start jiffies to submit IO. FIXME: fail handle mulitple pages
    int32_t idx_group;
} bioconter_t;

//TODO: bioc state, we should create a bioc component to handle
#define ACF_IDLE                0
#define ACF_ONGOING                1
#define ACF_UNFINISHED            2 //Resolve the confliction between GC write and Main thread write when there is no free space
#define ACF_PAGE_READ_DONE        30

//TODO: EU purpose, move to EU component
#define EU_FREE            0
#define EU_DATA            1
#define EU_UPDATELOG    2
#define EU_DEDICATEMAP    3
#define EU_ECT            4
#define EU_HPEU            5
#define EU_BMT            6
#define EU_SUPERMAP        7
#define EU_UNKNOWN      8
#define EU_GC            9
#define EU_MLOG            10
#define EU_EXT_ECC      11
#define EU_END_TAG        12

//#define EU_BITMAP_REFACTOR_PERF

//TODO: EU purpose, move to EU component
typedef struct EUProperty {
    struct list_head list;      /* link to connect entries of Freelist/Activelist/HList/Update log */
    int32_t eu_size;            /* erasure unit size in sectors */
    int32_t temper;             /* 0 for cold. 1 for warm. 2 for hot */
    sector64 start_pos;         /* Starting sector of the EU, sector base */
    int32_t log_frontier;       /* Current position of the write head inside EU */
    atomic_t eu_valid_num;      /* Valid # of sectors in the EU */
    int32_t index_in_updatelog; /* The virtual in-place index of the EU in the update log */
    int32_t disk_num;           /* to which disk this EU belongs to */
    t_erase_count erase_count;  /* Erase count of the EU */
/* Mlu : Bitmap in the unit of 4 KB page, note that the first FOUR sectors are used for dummy write and the last FOUR sectors are used for metadata.
** G    : Each bit represents 1 page. Hence, each byte holds bit map of 8 consecutive pages.
** Used __set_bit() and __clear_bit() to update the bitmap instead of the commonly used set_bit() and clear_bit() because the latter two lock 
** the memory region disabling interrupts as well, which is not necessary here since we have our locks protecting the bitmap.
*/
    int8_t *bit_map;            /* Bitmap to represent valid/invalid pages of the EU */
    int32_t index_in_LVP_Heap;  /* If this EU is in heap, array index. Else, -1 */
    atomic_t state_list;        /* 2 if in falselist. 1 if in hlist. 0 if in heap */
    smd_t act_metadata;

#ifndef EU_BITMAP_REFACTOR_PERF
    spinlock_t lock_bitmap;
#endif

    bool fg_metadata_ondisk;
    atomic_t cn_pinned_pages;   // e.g. the source address of a copy command
    int32_t usage;
    int32_t id_hpeu;
    sxorbmp_t xorbmp;
    atomic_t erase_state;
} EUProperty_t;

#include "GC.h"
//TODO: move to erase count
typedef struct SEraseCount {
    struct bio *bio;
//    unsigned long      bio_org_flag;
    int32_t io_queue_fin;
    struct mutex lock;
    struct EUProperty *eu_curr;
    struct EUProperty *eu_next;
    atomic_t fg_next_dirty;
//    t_erase_count      *ar_eu; // the array keeps the ect counter for each eu
    int32_t num_mempage;        // number of memory pages to store the erase counter table on disk
    int32_t cn_cycle;
} SEraseCount_t;

//TODO: move to GC
struct HListGC {
    int32_t eu_size;            /* erasure unit size in sectors */
    sector64 eu_log_start;      /* Starting sector for user data in a disk */
    sector64 free_list_number;  /* # of free EUs in the Freelist */
    atomic_t cn_clean_eu;
    sector64 total_eus;         /* # of total EUs in a disk */
    struct list_head free_list; /* List head of the Freelist */
    struct mutex lock_metadata_io_bio;
    struct bio *metadata_io_bio;
    struct bio *read_io_bio[FPAGE_PER_SEU / READ_EU_ONE_SHOT_SIZE]; /* BIOs to read EU during GC */
    wait_queue_head_t io_queue; /* The wait queue used to block any thread when necessary */
    atomic_t io_queue_fin;      /* Flag used for the above */
    struct EUProperty **pbno_eu_map;    /* Map from PBN to the corresponding EUProperty */
    struct SEraseCount ECT;

    int32_t NumberOfRegions;    /* # of temperatures configured */
    struct EUProperty **active_eus; /* array of active EUs size: temp layer */
    int32_t *par_cn_log;
    struct list_head hlist;     /* List head of the HList */
    struct EUProperty **LVP_Heap;   /* Array for the LVP Heap */
    struct EUProperty **LVP_Heap_backup;    /* Array for the LVP Heap for gc pick up */
    int32_t hlist_limit;        /* Max. size of the HList. Currently, 100 */
    int32_t hlist_number;       /* Current size of the HList */
    int32_t LVP_Heap_number;    /* Current size of the LVP Heap */
    int32_t LVP_Heap_backup_number;
    int32_t disk_num;           /* indicates which disk it belongs */

    rmutex_t whole_lock;        /* Mutex to protect HList, Freelist and Heap */
    struct list_head false_list;    /* List head of the FalseList */
    atomic_t false_list_number; /* Current size of the FalseList */
    struct EUProperty *min_erase_eu;
    struct EUProperty *max_erase_eu;

#ifdef  HLIST_GC_WHOLE_LOCK_DEBUG
    int8_t lock_fn_name[128];
    int32_t access_cnt;
#endif
};

extern lfsm_dev_t lfsmdev;
//extern atomic_t ioreq_counter;
extern int32_t cn_erase_front;
extern int32_t cn_erase_back;

extern int32_t pid_ioctl;
extern int32_t lfsm_resetData;
extern int32_t ulfb_offset;
extern int32_t lfsm_readonly;
extern atomic_t wio_cn[TEMP_LAYER_NUM];

extern int32_t my_make_request(struct bio *bio);
extern int32_t lfsm_write_pause(void);

extern uint32_t get_next_disk(void);

extern int32_t BiSectorAdjust_bdev2lfsm(struct bio *bio);
extern bool pbno_lfsm2bdev(sector64 pbno, sector64 * pbno_af_adj,
                           int32_t * dev_id);
extern bool pbno_lfsm2bdev_A(sector64 pbno, sector64 * pbno_af_adj,
                             int32_t * dev_id, struct diskgrp *pgrp);
extern sector64 calculate_capacity(lfsm_dev_t * td, sector64 org_size_in_page);

extern int32_t reinit_lfsm_subisks(lfsm_dev_t * td, MD_L2P_table_t * bmt_orig,
                                   sector64 total_SEU_orig,
                                   int32_t cn_ssd_orig);
extern int32_t reinit_external_ecc(lfsm_dev_t * mydev, int32_t cn_org_ssd,
                                   int32_t cn_new_ssd);
extern bool lfsm_idle(lfsm_dev_t * td);
extern void lfsm_cleanup(lfsm_cfg_t * myCfg, bool isNormalExit);
extern uint64_t get_lfsm_capacity(void);
extern int32_t lfsm_init(lfsm_cfg_t * myCfg, int32_t ul_off, int32_t ronly,
                         int64_t lfsm_auth_key);

extern void lfsm_resp_user_bio_free_bioc(lfsm_dev_t * td,
                                         struct bio_container *bio_c,
                                         int32_t err, int32_t id_bh_thread);

#endif
