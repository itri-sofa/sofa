/*
 * Copyright (c) 2015-2020 Industrial Technology Research Institute.
 *
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 *
 *
 *
 *
 *
 *
 *  
 */
#define CFTEST_H
#include "ssdconf.h"
#ifdef WIN32
#include "vssd.h"
#include <conio.h>
#else
#include "alssd.h"
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <time.h>
#include "bfun.h"


void wrcf(CVSSD&, unsigned char*, int, int, int);
void wwcf(CVSSD&, unsigned char*, int, int, int);
void wccf_sour(CVSSD&, unsigned char*, int, int, int);
void wccf_dest(CVSSD&, unsigned char*, int, int, int);
void wecf(CVSSD&, unsigned char*, int, int, int);
void crcf_sour(CVSSD&, unsigned char*, int, int, int);
void crcf_dest(CVSSD&, unsigned char*, int, int, int);
void cwcf_sour(CVSSD&, unsigned char*, int, int, int);
void cwcf_dest(CVSSD&, unsigned char*, int, int, int);
void cccf_sour(CVSSD&, unsigned char*, int, int, int);
void cccf_dest(CVSSD&, unsigned char*, int, int, int);
void cecf_sour(CVSSD&, unsigned char*, int, int, int);
void cecf_dest(CVSSD&, unsigned char*, int, int, int);
void ercf(CVSSD&, unsigned char*, int, int, int);
void ewcf(CVSSD&, unsigned char*, int, int, int);
void eecf(CVSSD&, unsigned char*, int, int, int);
void eccf_sour(CVSSD&, unsigned char*, int, int, int);
void eccf_dest(CVSSD& , unsigned char* , int, int, int);
void rrcf(CVSSD&, unsigned char*, int, int, int);
void rwcf(CVSSD&, unsigned char*, int, int, int);
void recf(CVSSD&, unsigned char*, int, int, int);
void rccf_sour(CVSSD&, unsigned char*, int, int, int);
void rccf_dest(CVSSD&, unsigned char*, int, int, int);
