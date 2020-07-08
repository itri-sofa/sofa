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
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include "../include/ssdconf.h"
#include "../include/lssd.h"


CVSSD::CVSSD(void)
{
}


CVSSD::~CVSSD(void)
{
}

bool CVSSD::InitParameter(int m_l2no_sector_in_page, int m_l2no_page_in_block)
{
	l2no_sector_in_page = m_l2no_sector_in_page;
	l2no_page_in_block = m_l2no_page_in_block;
	l2no_byte_in_sector = 9;
	return true;
}


bool CVSSD::Open(int DriverID)
{
	unsigned int AccessMode;
	if (DriverID==99)
		sprintf(DriverDesc,"/dev/lfsm");
	else
		sprintf(DriverDesc,"/dev/sd%c",DriverID+'a');
	printf("Open disk %s\n",DriverDesc);
	hDevice = open(DriverDesc,O_RDWR|O_DIRECT);
	
	if (hDevice==INVALID_HANDLE_VALUE)
		return false;
	cn_sector = GetSizeA();
	return true;
}

bool CVSSD::Close()
{
	close(hDevice);             
	hDevice=INVALID_HANDLE_VALUE;
	return true;
}

bool CVSSD::ReadByLPN(INT64 lpn, INT64 len, unsigned char* pBuf)
{
	INT64 ret;
	unsigned long m_nBytes=(UINT32)(len<<(l2no_sector_in_page + l2no_byte_in_sector));
	INT64 Offset = lpn << (l2no_sector_in_page + l2no_byte_in_sector);
	long hOff=(long)(Offset>>32);
	long lOff=(long)((Offset)&0x00000000ffffffff);
	if (lseek64(hDevice,Offset,SEEK_SET)==Offset)
	{
		if (read(hDevice,pBuf,m_nBytes)>=0)
			return 1;
		else
			perror("Read File error!");
	}
	else
		perror("Set File error! ");
	return 0;
}

bool CVSSD::ReadByPPN(unsigned long lbn, unsigned long id_page, unsigned long len, unsigned char* pBuf)
{
	INT64 Offset = lbn;
	ASSERT(id_page<128, "wrong page id");
	Offset <<= (l2no_page_in_block + l2no_sector_in_page);
	Offset += id_page;
	return ReadByLPN(Offset,len,pBuf);
}

bool CVSSD::WriteByLPN(INT64 lpn, INT64 len, unsigned char* pBuf)
{
	INT64 ret;
	unsigned long m_nBytes=(UINT32)(len<<(l2no_sector_in_page + l2no_byte_in_sector));
	INT64 Offset = lpn << (l2no_sector_in_page + l2no_byte_in_sector);
	long hOff=(long)(Offset>>32);
	long lOff=(long)((Offset)&0x00000000ffffffff);
	if (lseek64(hDevice,Offset,SEEK_SET)==Offset)	
	{
		if (write(hDevice,pBuf,m_nBytes)>=0)
			return 1;
		else
			perror("Write File error!\n");
	}
	else
		perror("Set File error!\n");
	return 0;	    
}


INT64 CVSSD::GetSizeA(void)
{
	INT64 inputsect=0;
	int bResult;                 // results flag
	bResult=stat(DriverDesc,&dg);
	if (bResult==-1)  return (0);
	inputsect=dg.st_size >> 9;	
	return inputsect;
}
