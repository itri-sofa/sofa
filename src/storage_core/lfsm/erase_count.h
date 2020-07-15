/*
 * SPDX-License-Identifier: GPL-2.0-or-later OR Apache-2.0
 * Copyright (C) 2015-2020 Industrial Technology Research Institute
 *
 *
 *
 *  
 */

#ifndef NEW_WEAR_LEVEL_H
#define NEW_WEAR_LEVEL_H

//----------public-----------//
extern bool ECT_init(struct HListGC *h, sector64 total_eu);
extern void ECT_destroy(struct SEraseCount *pECT);
extern void ECT_change_next(struct SEraseCount *pECT);
extern bool ECT_commit_all(lfsm_dev_t * td, bool isReuse);
extern void ECT_erase_count_inc(struct EUProperty *victimEU);
extern int32_t ECT_load_all(lfsm_dev_t * td);
extern int32_t ECT_rescue_disk(lfsm_dev_t * td, int32_t id_fail_disk);
extern bool ECT_next_eu_init(lfsm_dev_t * td);
extern int32_t ECT_rescue(struct HListGC *h);
//----------private----------//
extern bool ECT_commit(struct HListGC *h);
extern int32_t ECT_load(struct HListGC *h);

extern int ECT_dump_disk(char *buffer, char **buffer_location, off_t offset,
                         int buffer_length, int *eof, void *data);
extern int32_t ECT_dump_alldisk(int8_t * buffer, int32_t size);
#endif
