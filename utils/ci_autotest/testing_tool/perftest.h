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
#ifndef INC_PERFTEST_0918230981209312
#define INC_PERFTEST_0918230981209312

#ifdef WIN32
#include "include/vssd.h"
#include <conio.h>
#else
#include "include/alssd2.h"
#endif
int pf_var_write(CVSSD& ssd, unsigned long addr, int len);
int pf_4K_write(CVSSD& ssd, unsigned long addr, unsigned long round, bool HasPattern, int len, int skip);
int pf_4K_batchwrite(CVSSD& ssd, unsigned long addr, unsigned long round, bool HasPattern, bool isRand, unsigned long range, bool StepDisplay);
int pf_4K_batchread(CVSSD& ssd, unsigned long addr, unsigned long round, bool HasPattern, bool isRand, unsigned long range, bool StepDisplay);
int pf_4K_batch(CVSSD& ssd,int op, unsigned long addr, unsigned long round, bool HasPattern, bool isRand, unsigned long range, bool StepDisplay);
int pf_4K_read(CVSSD& ssd, unsigned long addr, unsigned long range, bool HasPattern);
int pf_4K_jump_read(CVSSD& ssd, unsigned long addr, unsigned long round, bool HasPattern, int len, int skip);
int pf_4K_randwrite(CVSSD& ssd, unsigned long addr, unsigned long range, unsigned long round, bool HasPattern, int len);
int pf_4K_randwriteA(CVSSD& ssd, unsigned long addr, unsigned long range, unsigned long round, bool HasPattern, int len);
int pf_4K_randread(CVSSD& ssd, unsigned long addr, unsigned long range, unsigned long round, bool HasPattern, int len);
int pf_4K_write_with_randread(CVSSD& ssd, unsigned long addr, unsigned long round, bool HasPattern, int len);
int randsw_seqsr(CVSSD& ssd, unsigned long addr, unsigned long range, unsigned long round,int pSeed, int len, bool isSim);
int pf_4K_randread_A(CVSSD& ssd, unsigned long addr, unsigned long range, unsigned long round, bool HasPattern, int len);
int pf_4K_randread_hybrid(CVSSD& ssd, unsigned long addr, unsigned long range, unsigned long round, bool HasPattern, int len);
int pf_4K_read_A(CVSSD& ssd, unsigned long addr, unsigned long round, bool HasPattern);
int pf_4K_jump_read_A(CVSSD& ssd, unsigned long addr, unsigned long round, bool HasPattern, int len, int skip);
int pf_4K_read_hybrid(CVSSD& ssd, unsigned long addr, unsigned long round, bool HasPattern);

int log_replay(CVSSD& ssd, char* filename, unsigned long offset, unsigned long range, int fg_delay);
int log_replay_chk(CVSSD& ssd, char* filename, unsigned long offset, unsigned long range);
int hybrid_randswsr(CVSSD& ssd, unsigned long addr,unsigned long range, unsigned long round, int Seed, int len, bool isSim,bool is_write);

extern bool gSlience;
extern int* gar_LatestValue;
extern unsigned long gOffsetLatestValueAR;
extern int g_version;
extern FILE* err_fp;

#endif

