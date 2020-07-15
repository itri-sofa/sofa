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
#include "../include/bfun.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <time.h>
#include "../include/ssdconf.h"
#include "../perftest.h"
#define SFTL_USER

#define SFTL_IOCTL_PRINT_DEBUG _IO( 0xac, 0)
#define SFTL_IOCTL_SIMU_IO _IO( 0xac, 1)
#define SFTL_IOCTL_SUBMIT_IO _IO( 0xac, 2)
#define SFTL_IOCTL_INIT_FIN _IO( 0xac, 3)
#define SFTL_IOCTL_SUBMIT_IO_PAYLOAD _IO( 0xac, 4)

#define MAX_PAGE 256

void dumphex(unsigned char* Buffer)
{
	int i,j,k,idx;
	for (k=0;k<(2*(1<<NUM_SECTOR_IN_PAGE_ORDER));k++)
	{
		for(i=0;i<16;i++)
		{
			printf("%04hx :",(UINT08)k*256+i*16);
	 

			for(j=0;j<8;j++)
				printf("%02hX ",(UINT08)Buffer[k*256+i*16+j]);

	 
			printf("- ");

			for(j=0;j<8;j++)
				printf("%02hX ",(UINT08)Buffer[k*256+i*16+j+8]);
			
			for(j=0;j<16;j++)
			{ 
				if (((UINT08)Buffer[k*256+i*16+j]<32)||((UINT08)Buffer[k*256+i*16+j]>=128)) printf(".");
				else printf("%c",Buffer[k*256+i*16+j]);
			}
			printf("\n");
		}
	
	}
	printf("\n");
}
int Check_data_hybrid(unsigned long addr, unsigned char* pBuf, int len)
{

	bool never_write=true;
	int i, offset,ret;

	for(i=0 ; i<len ; i++)
	{
		offset = addr +i - gOffsetLatestValueAR;
		if((gar_LatestValue>0) && (gar_LatestValue[offset]!=0))
			never_write=false;
	}

	if(never_write==true)
		return 2;// nerve write, no need to check 
	
	ret=Check_data(addr,pBuf,len);

	if(ret !=2)
	{
		fprintf(err_fp,"found error at addr %d \n",addr);
		fflush(err_fp);
	}
	return ret;
	
}

int Check_data(unsigned long addr, unsigned char* pBuf, int len)
{
	unsigned char buf_sample[PAGE_SIZE*MAX_PAGE] __attribute__ ((aligned(PAGE_SIZE)));
	unsigned char empty_sample[PAGE_SIZE*MAX_PAGE] __attribute__ ((aligned(PAGE_SIZE)));
	int ret;
	int i;

	memset(empty_sample,0xff,PAGE_SIZE*len);
	PageBaseFill(addr, len, buf_sample);
	
		if (memcmp(pBuf, buf_sample, PAGE_SIZE*len)==0)
			ret = 2;  // sequential and correct address
		else if (memcmp(pBuf, empty_sample, PAGE_SIZE*len)==0)
			ret = 1;  // empty
		else 
		{
		
			if ( memcmp(pBuf, buf_sample,4)==0 )
			{
				ret = 0;  // wrong content		
			} 
			else
			{
//				printf("wrong LPN %d\n",addr);
				ret = 3;	
			}
		}
			

	return ret;
}


void SeqFill(unsigned long MagicWord, unsigned long byte_len, unsigned char* pbuf)
{
	unsigned long i;
	unsigned long* pwbuf = (unsigned long*)pbuf;
	byte_len>>=2;
	 *(unsigned long*)pbuf = MagicWord;
	for(i=1;i<byte_len;i++)
	{
		pwbuf[i]= i;
	}

}
#define PRIME1 567629137 
#define PRIME2 7629137 
#define PRIME3 67629137
/*
void PageBaseFill(unsigned long MagicWord, unsigned long page_len, unsigned char* pbuf)
{
	unsigned long i,j;
	unsigned long* pwbuf = (unsigned long*)pbuf;
	unsigned long long byte_len;
	byte_len=page_len<<10;
	
	for(j=0; j<page_len; j++)
	{
		pwbuf[j*1024] = MagicWord+j;
		if (gar_LatestValue)
			pwbuf[1+j*1024]=gar_LatestValue[MagicWord+j];
		else
			pwbuf[1+j*1024] = (unsigned long)(MagicWord+j)*PRIME1;
		pwbuf[2+j*1024] = (unsigned long)(MagicWord+j)*PRIME2;
		pwbuf[3+j*1024] = (unsigned long)(MagicWord+j)*PRIME3;
		for(i=4; i<byte_len; i++)
		{
			pwbuf[i+j*1024]=i;
		}
	}
}
*/ 
#define PAGE_SIZE_IN_WORD (PAGE_SIZE/sizeof(unsigned long))
void PageBaseFill(unsigned long MagicWord, unsigned long page_len, unsigned char* pbuf)
{
	unsigned long i,j;
	unsigned long* pwbuf = (unsigned long*)pbuf;
	unsigned long off;
#ifdef FIXPAYLOAD 
	memset(pbuf,FIXPAYLOAD,page_len*PAGE_SIZE);
#else
	for(j=0; j<page_len; j++)
	{
	
		pwbuf[j*PAGE_SIZE_IN_WORD] = MagicWord+j;
		off = MagicWord +j - gOffsetLatestValueAR;
//		printf("gar_LatestValue[%d]=%d \n",MagicWord+j, gar_LatestValue[off]);

		if((gar_LatestValue>0) && (gar_LatestValue[off]==0))
		{
//			printf("%d %d \n",pwbuf[j*PAGE_SIZE_IN_WORD],pwbuf[1+j*PAGE_SIZE_IN_WORD]);
			for(i=0; i<PAGE_SIZE_IN_WORD; i++)
				pwbuf[i+j*PAGE_SIZE_IN_WORD]=0;
		}
		else if ((gar_LatestValue>0) && (gar_LatestValue[off]>0))
		{	
			pwbuf[1+j*PAGE_SIZE_IN_WORD]= gar_LatestValue[off];
			pwbuf[2+j*PAGE_SIZE_IN_WORD] = (unsigned long)(MagicWord+j)*PRIME2;
			pwbuf[3+j*PAGE_SIZE_IN_WORD] = (unsigned long)(MagicWord+j)*PRIME3;
			pwbuf[4+j*PAGE_SIZE_IN_WORD]= g_version;
//			printf("%d %d \n",pwbuf[j*PAGE_SIZE_IN_WORD],pwbuf[1+j*PAGE_SIZE_IN_WORD]);
			for(i=5; i<PAGE_SIZE_IN_WORD; i++)
			{
				pwbuf[i+j*PAGE_SIZE_IN_WORD]=i;
			}
		}

		else
		{
			pwbuf[1+j*PAGE_SIZE_IN_WORD] = (unsigned long)(MagicWord+j)*PRIME1;
			pwbuf[2+j*PAGE_SIZE_IN_WORD] = (unsigned long)(MagicWord+j)*PRIME2;
			pwbuf[3+j*PAGE_SIZE_IN_WORD] = (unsigned long)(MagicWord+j)*PRIME3;
//			printf("%d %d \n",pwbuf[j*PAGE_SIZE_IN_WORD],pwbuf[1+j*PAGE_SIZE_IN_WORD]);
			pwbuf[4+j*PAGE_SIZE_IN_WORD]= gSeed;
			for(i=5; i<PAGE_SIZE_IN_WORD; i++)
			{
				pwbuf[i+j*PAGE_SIZE_IN_WORD]= pwbuf[i-1+j*PAGE_SIZE_IN_WORD]*241202641; //rand();
			}			
		}

	}	
#endif
}

void NormalFill(unsigned long MagicWord, unsigned long len, unsigned char* pbuf)
{
	unsigned long i;
	unsigned long* pdwbuf = (unsigned long*)pbuf;
	len>>=2;
	
	for(i=0;i<len;i+=2)
	{
		pdwbuf[i]= MagicWord;
		pdwbuf[i+1]= 0x55AAF00F;
	}
}
void RandomFill(unsigned long MagicWord, unsigned long len, unsigned char* pbuf)
{
	unsigned long i;
	unsigned long* pdwbuf = (unsigned long*)pbuf;
	len>>=2;
	pdwbuf[0]= MagicWord;
	srand(MagicWord+1);
	for(i=1;i<len;i++)
	{
		pdwbuf[i] = rand();
		pdwbuf[i] <<=16;
		pdwbuf[i] |= rand();
	}
}
bool bLFSM_erase(unsigned long addr, unsigned long len)
{
	int ret;
	int fd;
	sftl_ioctl_simu_w_t arg;	
	fd = open ("/dev/lfsm", O_RDWR|O_SYNC);
	if (fd == -1)
	{
		perror("Fail to open /dev/lfsm\n");
		return false;
	}
	arg.m_nType = 17;
	arg.m_nSum = addr;
	arg.m_nSize = len;
	printf("\r\n len=%d\r\n",len);
	ret = ioctl (fd, SFTL_IOCTL_INIT_FIN, &arg);
	if (ret==-1)
		perror("Fail to ioctl /dev/lfsm for erase");
	ASSERT(ret!=-1,"AST: Fail to ioctl /dev/lfsm for erase");
	close(fd);		
	return (ret!=-1);
}

bool bLFSM_copy(unsigned long addr, unsigned long dest, unsigned long len)
{
	int ret;
	int fd;
	sftl_ioctl_simu_w_t arg;
	fd = open ("/dev/lfsm", O_RDWR|O_SYNC);
	if (fd == -1)
	{
		perror("Fail to open /dev/lfsm\n");
		return false;
	}
	arg.m_nType = 20;
	arg.m_nSum = addr;
	arg.m_nBatchSize = dest;
	arg.m_nSize = len;
	ret = ioctl (fd, SFTL_IOCTL_INIT_FIN, &arg);
	if (ret==-1)
		perror("Fail to ioctl /dev/lfsm for copy");
	ASSERT(ret!=-1,"AST: Fail to ioctl /dev/lfsm for copy");		
	close(fd);
	return (ret!=-1);
}
bool bLFSM_markbad(unsigned long addr, unsigned long len)
{
	int ret;
	int fd;
	sftl_ioctl_simu_w_t arg;
	fd = open ("/dev/lfsm", O_RDWR|O_SYNC);
	if (fd == -1)
	{
		perror("Fail to open /dev/lfsm");
		return false;
	}
	arg.m_nType = 19;
	arg.m_nSum = addr;
	arg.m_nSize = len;
	ret = ioctl (fd, SFTL_IOCTL_INIT_FIN, &arg);
	if (ret==-1)
		perror("Fail to ioctl /dev/lfsm for mark bad");
	ASSERT(ret!=-1,"AST:Fail to ioctl /dev/lfsm for mark bad");
	close(fd);
	return (ret!=-1);
}

bool bLFSM_query(unsigned long addr, unsigned long len, bool isBlkLevel)
{
	int ret;
	int fd;
	sftl_ioctl_simu_w_t arg;
	fd = open ("/dev/lfsm", O_RDWR|O_SYNC);
	if (fd == -1)
	{
		perror("Fail to open /dev/lfsm");
		return false;
	}
	if (isBlkLevel)
		arg.m_nType=16;
	else
		arg.m_nType=18;
	arg.m_nSum = addr;
	arg.m_nSize = len;
	ret = ioctl (fd, SFTL_IOCTL_INIT_FIN, &arg);
	if (ret==-1)
		perror("Fail to ioctl /dev/lfsm for query");
	ASSERT(ret!=-1,"AST: Fail to ioctl /dev/lfsm for query");
	close(fd);	
	return (ret!=-1);
}

