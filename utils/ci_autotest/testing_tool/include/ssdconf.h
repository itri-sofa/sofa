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
#ifndef SSDCONF_INC_H
#define SSDCONF_INC_H
#define ASSERT(X,Y) if (!(X)) { printf("[%s:%d] ASSERT: %s\n",__FUNCTION__, __LINE__, Y); while(1); }
#ifdef WIN32
typedef __int64 INT64;
typedef unsigned __int8 UINT08;
#else
typedef long long INT64;
typedef unsigned char UINT08;
typedef unsigned long UINT32;
#endif

//#define NUM_SECTOR_IN_PAGE_ORDER 3
#ifndef NUM_SECTOR_IN_PAGE_ORDER
#error "NUM_SECTOR_IN_PAGE_ORDER must be defined"
#endif
extern int gSeed;
#define PAGE_SIZE (1<<(NUM_SECTOR_IN_PAGE_ORDER+9))
#define MAX_PAGE 256
#define MAX_PENDING_REQS 2048
#endif
