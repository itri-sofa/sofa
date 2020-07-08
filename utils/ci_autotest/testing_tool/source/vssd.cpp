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

#include <windows.h>
#include <winioctl.h>
#include <stdio.h>
#include "..\include\ssdconf.h"
#include "..\include\vssd.h"


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
	AccessMode=GENERIC_READ|GENERIC_WRITE;

	sprintf(DriverDesc,"\\\\.\\PHYSICALDRIVE%d",DriverID);
	hDevice = CreateFile(DriverDesc, // drive to open
                       AccessMode,       // ReadOnly Open
                       FILE_SHARE_READ | FILE_SHARE_WRITE,  // share mode
                       NULL,    // default security attributes
                       OPEN_EXISTING,  // disposition
                       0,       // file attributes
                       NULL);   // don't copy any file's attributes
	if (hDevice==INVALID_HANDLE_VALUE)
		return false;
	cn_sector = GetSizeA();
	return true;
}

bool CVSSD::Close()
{
	CloseHandle(hDevice);             
	hDevice=INVALID_HANDLE_VALUE;
	return true;
}

bool CVSSD::ReadByLPN(INT64 lpn, INT64 len, unsigned char* pBuf)
{
	unsigned long m_nBytes=(UINT32)(len<<(l2no_sector_in_page + l2no_byte_in_sector));
	INT64 Offset = lpn << (l2no_sector_in_page + l2no_byte_in_sector);
	long hOff=(long)(Offset>>32);
	long lOff=(long)((Offset)&0x00000000ffffffff);
	if (SetFilePointer(hDevice,lOff,&hOff,FILE_BEGIN)==(unsigned long)lOff)
	{
		if (ReadFile(hDevice,pBuf,m_nBytes,&m_nBytes,NULL)>0)
			return 1;
		else
			printf("Read File error! (%d)\n", GetLastError());
	}
	else
		printf("Set File error! (%d)\n", GetLastError());
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
	unsigned long m_nBytes=(UINT32)(len<<(l2no_sector_in_page + l2no_byte_in_sector));
	INT64 Offset = lpn << (l2no_sector_in_page + l2no_byte_in_sector);
	long hOff=(long)(Offset>>32);
	long lOff=(long)((Offset)&0x00000000ffffffff);
	if (SetFilePointer(hDevice,lOff,&hOff,FILE_BEGIN)==(unsigned long)lOff)
	{
		if (WriteFile(hDevice,pBuf,m_nBytes,&m_nBytes,NULL)>0)
			return 1;
		else
			printf("WriteFile failure (%d)\n",GetLastError());
	}
	else
	{
		printf("SetFilePointer failure %lu %lu\n",lOff,hOff);
		return 0;
	}
	return 0;
}


INT64 CVSSD::GetSizeA(void)
{
	INT64 specC=0;
	INT64 inputsect=0;
 	int bResult;                 // results flag
	DWORD junk;                   // discard results
	bResult = DeviceIoControl(hDevice,  // the device we are querying
                            IOCTL_DISK_GET_DRIVE_GEOMETRY,  // operation to perform
                            NULL, 0, // no input buffer, so we pass zero
                            &dg, sizeof(dg),  // the output buffer
                            &junk, // discard the count of bytes returned
                            (LPOVERLAPPED) NULL);  // synchronous I/O
	
	if (!bResult)  return (FALSE);
	specC=dg.Cylinders.QuadPart;
	inputsect=(dg.Cylinders.QuadPart * dg.TracksPerCylinder * dg.SectorsPerTrack);
	return inputsect;
}
