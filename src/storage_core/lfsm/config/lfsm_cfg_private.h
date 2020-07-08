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

#ifndef _LFSM_CFG_PRIVATE_H_
#define _LFSM_CFG_PRIVATE_H_

#define DEF_REINSTALL       0
#define DEF_CNSSDS          3
#define DEF_SSD_HPEU        3
#define DEF_CNT_PGROUP      1
#define DEF_IO_THREADS      7
#define DEF_BH_THREADS      3
#define DEF_GC_RTHREADS     1
#define DEF_GC_WTHREADS     1
#define DEF_GC_SEU_RESERV   250 //25 mean 25%
#define DEF_GC_CHK_INTVL    1000    //1 second
#define DEF_GC_UPPER_RATIO   250    //50% when free EU >= 250/1000 * total EU, stop GC
#define DEF_GC_LOWER_RATIO   125    //25% when free EU <= 125/1000 * total EU, start normal GC
#define DEF_DISK_PARTITION  256
#define DEF_MEM_BIOSET_SIZE 2

//NOTE: size of devlist should large than MAX_SUPPORT_SSD
int8_t *devlist[] = {
    "a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "l", "m",
    "n", "o", "p", "q", "r", "s", "t", "u", "v", "w", "x", "y", "z",

    "aa", "ab", "ac", "ad", "ae", "af", "ag", "ah", "ai", "aj", "ak", "al",
        "am",
    "an", "ao", "ap", "aq", "ar", "as", "at", "au", "av", "aw", "ax", "ay",
        "az",

    "ba", "bb", "bc", "bd", "be", "bf", "bg", "bh", "bi", "bj", "bk", "bl",
        "bm",
    "bn", "bo", "bp", "bq", "br", "bs", "bt", "bu", "bv", "bw", "bx", "by",
        "bz",
};

lfsm_cfg_t lfsmCfg;

#endif /* _LFSM_CFG_PRIVATE_H_ */
