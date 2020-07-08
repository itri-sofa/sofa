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
#ifndef CVSSD_INC_H
#define CVSSD_INC_H
#define INVALID_HANDLE_VALUE ((UINT32)-1)

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <libaio.h>
#include "ssdconf.h"

class CVSSD
{
public:
	CVSSD(void);
	~CVSSD(void);
	bool Open(int DriverID);
	bool Close(void);
	bool ReadByLPN(INT64 lpn, INT64 len, unsigned char* pBuf);
	bool WriteByLPN(INT64 lpn, INT64 len, unsigned char* pBuf);
	bool ReadByPPN(unsigned long lbn, unsigned long id_page, unsigned long len, unsigned char* pBuf);
	bool InitParameter(int m_l2no_sector_in_page, int m_l2no_page_in_block);
	INT64 GetSize(void)
	{
		return cn_sector;
	}
protected:
	INT64 GetSizeA(void);
	int hDevice;
	char DriverDesc[64];
	int l2no_sector_in_page;
	int l2no_page_in_block;
	int l2no_byte_in_sector;
	INT64 cn_sector;
	struct stat dg;
	
protected:
	io_context_t ctx;
	unsigned int nr_events;
	
};


#endif