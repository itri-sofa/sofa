/*
 * Copyright (c) 2015-2025 Industrial Technology Research Institute.
 *
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
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
