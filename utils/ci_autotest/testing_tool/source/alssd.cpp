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
#include <stdlib.h>
#include <libaio.h>
#include <stdlib.h>
#include "../include/ssdconf.h"
#include "../include/alssd.h"


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
	unsigned int flags=O_RDWR|O_DIRECT;
	nr_events = 10;
    memset(&ctx, 0, sizeof(ctx));  // It's necessary
    int errcode = io_setup(nr_events, &ctx);
    if (errcode == 0)
         perror("io_setup");
	
	if (DriverID==99)
		sprintf(DriverDesc,"/dev/lfsm");
	else
		sprintf(DriverDesc,"/dev/sd%c",DriverID+'a');
	printf("Open disk %s\n",DriverDesc);
	hDevice = open(DriverDesc,flags);
	
	if (hDevice==INVALID_HANDLE_VALUE)
		return false;
	
	return true;
}

bool CVSSD::Close()
{
	close(hDevice);    
	io_destroy(ctx);	
	hDevice=INVALID_HANDLE_VALUE;
	return true;
}

bool CVSSD::ReadByLPN(INT64 lpn, INT64 len, unsigned char* pBuf)
{
	INT64 ret;
	unsigned long m_nBytes=(UINT32)(len<<(l2no_sector_in_page + l2no_byte_in_sector));
	INT64 Offset = lpn << (l2no_sector_in_page + l2no_byte_in_sector);
    struct io_event events[10];
    struct timespec timeout = {1, 100};
    struct iocb *iocbpp = (struct iocb *)malloc(sizeof(struct iocb));
    memset(iocbpp, 0, sizeof(struct iocb));

    iocbpp[0].data           = pBuf;
    iocbpp[0].aio_lio_opcode = IO_CMD_PREAD;
    iocbpp[0].aio_reqprio    = 0;
    iocbpp[0].aio_fildes     = hDevice;

    iocbpp[0].u.c.buf    = pBuf;
    iocbpp[0].u.c.nbytes = m_nBytes;//strlen(buf);
    iocbpp[0].u.c.offset = Offset;


    int n = io_submit(ctx, 1, &iocbpp);

    n = io_getevents(ctx, 1, 10, events, &timeout);
    if (n>0)
        printf("io[0] return:%d\n",events[0].res);
    else
    {
        printf("get no event\n");
        return 0;
    }
    return 1;
}

bool CVSSD::ReadByPPN(unsigned long lbn, unsigned long id_page, unsigned long len, unsigned char* pBuf)
{
	printf("ReadByPPN Unsupport\n");
	return false;
}

bool CVSSD::WriteByLPN(INT64 lpn, INT64 len, unsigned char* pBuf)
{
	INT64 ret;
	unsigned long m_nBytes=(UINT32)(len<<(l2no_sector_in_page + l2no_byte_in_sector));
	INT64 Offset = lpn << (l2no_sector_in_page + l2no_byte_in_sector);
    struct io_event events[10];
    struct timespec timeout = {1, 100};
	struct iocb *iocbpp = (struct iocb *)malloc(sizeof(struct iocb));
    memset(iocbpp, 0, sizeof(struct iocb));

    iocbpp[0].data           = pBuf;
    iocbpp[0].aio_lio_opcode = IO_CMD_PWRITE;
    iocbpp[0].aio_reqprio    = 0;
    iocbpp[0].aio_fildes     = hDevice;

    iocbpp[0].u.c.buf    = pBuf;
    iocbpp[0].u.c.nbytes = m_nBytes;//strlen(buf);
    iocbpp[0].u.c.offset = Offset;


    int n = io_submit(ctx, 1, &iocbpp);

    n = io_getevents(ctx, 1, 10, events, &timeout);
    if (n>0)
        printf("io[0] return:%d\n",events[0].res);
    else
    {
        printf("get no event\n");    
        return 0;
    }
	return 1;	    
}


INT64 CVSSD::GetSizeA(void)
{
	return 0;
}
