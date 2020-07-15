/*
 * SPDX-License-Identifier: GPL-2.0-or-later OR Apache-2.0
 * Copyright (C) 2015-2020 Industrial Technology Research Institute
 *
 *
 *
 *  
 */
/*
** This is the header file for Bmt_commit.h file
** Only the function prototypes declared below are exported to be used by other files
*/

#ifndef BMT_COMMIT_H
#define BMT_COMMIT_H

extern int32_t BMT_commit_manager(lfsm_dev_t *, MD_L2P_table_t *);
extern void flush_clear_bmt_cache(lfsm_dev_t * td, MD_L2P_table_t * bmt,
                                  bool isNormalExit);

extern int32_t update_ondisk_BMT(lfsm_dev_t *, MD_L2P_table_t *,
                                 bool reuseable);
//NOTE: related to release process, don't remove
//void change_signature_during_unload (MD_L2P_table_t *, int32_t);
extern int32_t BMT_cache_commit(lfsm_dev_t * td, MD_L2P_table_t * bmt);

#endif
