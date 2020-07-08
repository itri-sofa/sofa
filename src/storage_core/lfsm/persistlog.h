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
#ifndef PSSLOG_INC_12098301928309123
#define PSSLOG_INC_12098301928309123

struct lfsm_dev_struct;
struct EUProperty;

#define PSSLOG_CN_EU 2
#define PSSLOG_ID_EUMAIN     0
#define PSSLOG_ID_EUBACKUP     1
#define PSSLOG_ID2MASK(X) (0x01 << X)
#define PSSLOG_CHECKSUM 0x5A6211F3

typedef struct SPssLog {
    lfsm_dev_t *td;
    struct EUProperty *eu[2];   /* main and backup EU that storing log entry */
    int32_t sz_real;            /* the size of log entry */
    int32_t sz_alloc;           /* the size of log entry with page alignment */
    int32_t eu_usage;           /* component name which use pss log */
    struct mutex spss_lock;     /* lock to protect the access of log */
    SNextPrepare_t next;        /* pre-alloc EUs that for logging when eu are full */
    int32_t cn_cycle;           /* counting info of number of change EUs due to log full */
} SPssLog_t;

//Public Function
extern bool PssLog_Assign(SPssLog_t * pssl, sector64 pos_main,
                          sector64 pos_backup, int32_t idx_next,
                          int8_t * pret_buf);

extern int32_t PssLog_Getlast(SPssLog_t * pssl, int8_t * buf);
extern int32_t PssLog_Getlast_binarysearch(SPssLog_t * pssl, int8_t * buf);
extern int32_t PssLog_Get_Comp(SPssLog_t * pssl, int8_t * buf, int32_t off_next,
                               bool comp);

extern int32_t PssLog_get_entry_size(SPssLog_t * pssl);
extern int32_t PssLog_Reclaim(SPssLog_t * pssl);

extern bool PssLog_Append(SPssLog_t * pssl, int8_t * buf);
extern bool PssLog_AllocEU(SPssLog_t * pssl, int32_t ar_id_dev[PSSLOG_CN_EU],
                           int8_t * buf, bool NeedLog);

extern bool PssLog_reinit(SPssLog_t * pssl, int32_t sz_real, int32_t sz_alloc);
extern bool PssLog_Init(SPssLog_t * pssl, lfsm_dev_t * td, int32_t sz_real,
                        int32_t sz_alloc, int32_t eu_usage);
extern void PssLog_Destroy(SPssLog_t * pssl);

#endif //PSSLOG_INC_12098301928309123
