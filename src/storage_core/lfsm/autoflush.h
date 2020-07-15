/*
 * SPDX-License-Identifier: GPL-2.0-or-later OR Apache-2.0
 * Copyright (C) 2015-2020 Industrial Technology Research Institute
 *
 *
 *
 *  
 */
#ifndef __HYBRID_H
#define __HYBRID_H

#include "bmt.h"

typedef struct sflushops {
    int8_t *name;
    void *variables;
    bool (*init)(struct lfsm_dev_struct * td, struct sflushops * afops,
                 int32_t id_user);
     int32_t(*estimate) (MD_L2P_item_t * ar_ppq, sector64 lpno_s, int32_t len);
     int32_t(*exec) (struct lfsm_dev_struct * td, struct sflushops * ops,
                     sector64 lpno_s, int32_t len);
    bool (*waitdone)(struct sflushops * ops);
    bool (*destroy)(struct sflushops * ops);
} sflushops_t;

// should be used outside bmt_lookup
#define  PBNO_ON_SSD(pbno)  (pbno != PBNO_SPECIAL_NEVERWRITE)

#endif
