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
#ifndef CPageBuffer_INC_H
#define CPageBuffer_INC_H

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "ssdconf.h"
#define MAX_PAGE_BUFFER (81920L)

class CPageBuffer 
{
protected:  
	unsigned char *buffer; //[MAX_PAGE_BUFFER*PAGE_SIZE] __attribute__ ((aligned(PAGE_SIZE)));
	int bitmap[MAX_PAGE_BUFFER];
	int cn_used;
	unsigned char *pOrgBuf;		
public:
	CPageBuffer(void);
	~CPageBuffer(void);
	unsigned char* Allocate(int len);
	bool FreeBuf(unsigned char* ptr, int len);
		
};
#endif