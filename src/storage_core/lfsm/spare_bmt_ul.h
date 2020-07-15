/*
 * SPDX-License-Identifier: GPL-2.0-or-later OR Apache-2.0
 * Copyright (C) 2015-2020 Industrial Technology Research Institute
 *
 *
 *
 *  
 */
#ifndef SPARE_BMT_UL_H
#define SPARE_BMT_UL_H

#include "lfsm_core.h"
#include "EU_access.h"
#include "bmt_commit.h"
#include "special_cmd.h"
#include "bmt_ppdm.h"

#define CN_PAGES_BEFORE_UL_EU_END 30

#define SPAREBMT_DEBUG 1

#define MASK_SPAREAREA                0x0000ffffffffffffULL // wf: only 6-bytes of spare area in firmware is usable
#define TAG_SPAREAREA_BADPAGE         0x0000fffffffffffeULL //tf: only 6-byte for spare area bad bmt page
#define TAG_SPAREAREA_EMPTYPAGE     MASK_SPAREAREA
#define SPAREAREA_IS_EMPTY(X) (((X) & MASK_SPAREAREA) == TAG_SPAREAREA_EMPTYPAGE)
#define SPAREAREA_IS_BAD(X) (((X) & MASK_SPAREAREA) == TAG_SPAREAREA_BADPAGE)

//#define TAG_UNWRITE_HPAGE        ((TF_FW_FREE)?0:(~0))
#define TAG_UNWRITE_HPAGE        0
#define TAG_METADATA_EMPTYBYTE  0xFF
#define TAG_METADATA_EMPTY       ((sector64)0xFFFFFFFFFFFFFFFFULL)
#define TAG_METADATA_WRITTEN   ((sector64)0x5A5A5A5A5A5A5A5AULL)

#define EU_ACTIVE 1
#define EU_INACTIVE 0

#define COLD 0
#define WARM 1
#define HOT  2

#define SZ_SPAREAREA       8
#define SZ_SPAREAREA_ORDER 3
#define MAX_LPN_ONE_SHOT ((PAGE_SIZE-LPN_QUERY_HEADER)>>SZ_SPAREAREA_ORDER)
#define UL_THRE_INIT      98
#define UL_THRE_INC        2

//enum ul_marker {eu_used, eu_erase, gc, hpeu_erase};
#define UL_NORMAL_MARK        0x34
#define UL_GC_MARK            0x36
#define UL_ERASE_MARK         0x38
#define UL_HPEU_ERASE_MARK    0x40
//#define UL_EMPTY_MARK ((TF_FW_FREE)?0:(0xFFFF))
#define UL_EMPTY_MARK         0
#define UL_OPT_RD_HPAGE       16

typedef struct seuinfo {
    sector64 eu_start;
    int32_t frontier;           //in sector
    int32_t id_hpeu;
    sector64 birthday;
} seuinfo_t;

typedef struct normal_UL_record {
    sector64 eu_start;          /*start pos of EU that change attributes (for example: from free list to active EU) */
    int32_t disk_num;           /* which disk this EU belong to */
    int32_t temperature;        /* the temperature of hpeu that EU join */
    int32_t id_hpeu;            /* the id of hpeu that EU join */
    sector64 birthday;          // 8
} UL_record_t;

typedef struct GC_UL_record {
    int32_t total_disks;
    seuinfo_t ar_active;
} GC_UL_record_t;

typedef struct BMT_update_record {
    uint16_t signature;         /* identify which type of log: GC, HPEU erase, Normal, Erase */
    union {
        UL_record_t n_record;
        GC_UL_record_t gc_record;
        int32_t id_hpeu;
    };
} BMT_update_record_t;

/* This will hold all the active EUs that have been finished */
typedef struct active_eus_list {
    struct list_head cold_eus;
    struct list_head warm_eus;
    struct list_head hot_eus;
} active_eus_list_t;

typedef struct srec_param {
    active_eus_list_t active_list;
//    struct seuinfo* par_active; //[LFSM_DISK_NUM][TEMP_LAYER_NUM];
//    int* par_eu_occupied;//[LFSM_DISK_NUM][TEMP_LAYER_NUM];
} srec_param_t;

typedef struct SUpdateLog {
    sector64 size;              //BMT_update_log_size;
    struct mutex lock;          /* protect the concurrent access of update log */
    struct EUProperty *eu_main; /* main EU of update log */
    struct EUProperty *eu_backup;   /* backup EU of update log */
    int32_t threshold;          /* threshold to commit the update log */
    SNextPrepare_t next;        /* pre-alloc extra EU when eu_main and eu_backup full */
    __kernel_time_t last_trimbio_arrive;

// for update_log reset
    int32_t ul_frontier;        /* next location in EU for logging */
    BMT_update_record_t *ul_rec;
    int32_t cn_cycle;           /* counting info of change to new EU */
} SUpdateLog_t;

extern seuinfo_t *gculrec_get_euinfo(GC_UL_record_t * prec, int32_t i,
                                     int32_t j);
extern seuinfo_t *recparam_get_euinfo(srec_param_t * prec, int32_t id_disk,
                                      int32_t id_temp);
extern void recparam_set_eu_occupied(srec_param_t * prec, int32_t id_disk,
                                     int32_t id_temp, int32_t value);
extern int32_t recparam_get_eu_occupied(srec_param_t * prec, int32_t id_disk,
                                        int32_t id_temp);

//-----------public--------------//
//int UpdateLog_logging(lfsm_dev_t *td, sector64 eu_start, int temperature, int log_marker, int disk_num);
extern int32_t UpdateLog_logging(lfsm_dev_t * td, struct EUProperty *log_eu,
                                 int32_t temper, int32_t log_marker);
extern bool UpdateLog_hook(void *p);

//------------private--------------//
extern int32_t UpdateLog_logging_if_undone(lfsm_dev_t * td,
                                           struct EUProperty *eu_ret);

extern int32_t UpdateLog_logging_async(lfsm_dev_t * td,
                                       struct EUProperty *log_eu,
                                       int32_t temperature, int32_t log_marker,
                                       struct bio_container **ar_pbioc);
extern int32_t UpdateLog_reset_withlock(lfsm_dev_t * td);
extern int32_t UpdateLog_try_reset(SUpdateLog_t * pUL, lfsm_dev_t * td,
                                   bool force_reset);

//------------ Initialization --------------//
extern int32_t UpdateLog_eu_set(SUpdateLog_t * pUL, lfsm_dev_t * td,
                                sddmap_disk_hder_t * pheader);
extern int32_t UpdateLog_eu_init(SUpdateLog_t * pUL, int32_t ulfrontier,
                                 bool fg_insert_old);
extern int32_t UpdateLog_eunext_init(SUpdateLog_t * pUL);
extern bool UpdateLog_init(SUpdateLog_t * pUL);
extern bool UpdateLog_rescue(lfsm_dev_t * td, SUpdateLog_t * pUL);

//------------ shutdown --------------//
extern void UpdateLog_destroy(SUpdateLog_t * pUL);

//------------ MISC --------------//
extern int32_t UpdateLog_check_threshold(SUpdateLog_t * pUL);
extern bool UpdateLog_dump(lfsm_dev_t * td, int8_t * data_main,
                           int8_t * data_backup);
#endif
