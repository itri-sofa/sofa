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
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
unsigned int get_seed(unsigned int  id)
{
	return (id+13);

}
unsigned int GeneratePattern(unsigned int* pbuf, unsigned int seed, int sz_int, int* ar_magic, int sz_magic)
{
	int i;
	for(i=0;i<sz_magic;i++)
		pbuf[i] = ar_magic[i];
	for(i=sz_magic;i<sz_int;i++)
	{
		pbuf[i] = seed;
		seed *= 567629137;
	}
	return seed;

}

int GenerateFile(char* path, int id_file, int sz_file, unsigned int  seed)
{
	int ret;
	FILE* fp;
	int ar_magic[2];
	int offset=0;
	unsigned char buffer[4096];
	char filepath[512];
	sprintf(filepath,"%s/F%05d.hex",path,id_file);
	fp = fopen(filepath,"wb");
	if (fp==0)
	{
		printf("fail to open file %s (%d)\n",filepath,errno);
		return -1;
	}
	ar_magic[0] = id_file;

	while(sz_file>0)
	{
		ar_magic[1] = offset;	
		seed=GeneratePattern((unsigned int*)buffer,seed,4096/sizeof(int), ar_magic, 2);
		if (sz_file>8)
			ret=fwrite(buffer,4096,1,fp);
		else
			ret=fwrite(buffer,sz_file*512,1,fp);
		if (ret<=0)
		{
			printf("file %d sz=%d write failure\n",id_file,sz_file);
			return -1;
			break;
		}
		sz_file-=8;
		offset +=8;
	}
	fsync(fileno(fp));
	fclose(fp);
	return 1;
}


void MemCompare(unsigned char* pSrc, unsigned char* pDest, int sz_read)
{
	int i,j;
	for(i=0;i<32;i++)
	{
		for(j=0;j<16;j++)
		{
			if (pSrc[i*16+j]==pDest[i*16+j])
				printf("%02X    ",pSrc[i*16+j]);
			else
				printf("%02X_%02X ",pSrc[i*16+j],pDest[i*16+j]);
		}
		printf("\n");
	}	
}

int CheckFile(char* path, int id_file, unsigned int  seed)
{
	FILE* fp;
	int sz_read;
	int offset=0;
	int cn_err=0;
	int ar_magic[2];
	unsigned char buffer[4096];
	unsigned char buffer_sample[4096];
	char filepath[512];
	sprintf(filepath,"%s/F%05d.hex",path,id_file);
	fp = fopen(filepath,"rb");
	if (fp==0)
	{
		printf("fail to open file %s (%d)\n",filepath,errno);
		perror("ERR MSG:");
		return -1;
	}
	ar_magic[0] = id_file;
	do
	{
		ar_magic[1] = offset*8;
		seed=GeneratePattern((unsigned int*)buffer_sample,seed,4096/sizeof(int),ar_magic,2);
		sz_read = fread(buffer,1,4096,fp);
		if (memcmp(buffer,buffer_sample,sz_read)!=0)
		{
			printf("file %s byte_off=%d (sz=%d bytes, first dword=%08x %08x) error\n",
				filepath,offset*4096,sz_read, *(unsigned int*)buffer,*(unsigned int*)(buffer+4));
			MemCompare(buffer_sample,buffer,sz_read);
			cn_err++;
		}
		offset++;
	}while(sz_read==4096);
	fclose(fp);
	return cn_err;
}
// 0 ~ 100
bool ToGenWriteCmd(int prob_write)
{
	if (prob_write==0)
		return false;
	else if (prob_write==100)
		return true;
	else
		return ((rand()%100)<prob_write);
}
int main(int argc, char* argv[])
{
	int i, id_file;
	int num_files;
	int sz_file;
	int sz_max_file;
	int tp_access;
	int cn_create=0;
	int cn_check=0;
	int cn_err=0;
	char path_output[256];
	char* pEnvStr;
	int RandSeed=12345;
	struct timeval start,end;	
	long long diff;
	if (argc!=6)
	{
		printf("argc=%d: ftest output_dir num_files sz_maxfile(in sector) tp_access (0=r,100=w,others=mixed) seed(e.g.12345)\n",argc);
		return 0;
	}
	strcpy(path_output,argv[1]);
	num_files = atoi(argv[2]);
	sz_max_file = atoi(argv[3]);
	tp_access = atoi(argv[4]);
	RandSeed=atoi(argv[5]);
	printf("%d round , max_filesz=%d sector \n",num_files,sz_max_file);
/*	
    pEnvStr=getenv("ftest_seed");
    if ((pEnvStr)&&(RandSeed==0))
    {
        RandSeed=atol(pEnvStr);
        printf("Get ftest_seed=%d\n",RandSeed);
    }
*/    
    srand(RandSeed);
	if (tp_access==0)
	{
		printf("Force to check all files\n");
		for(i=0;i<num_files;i++)
		{
			fprintf(stderr,"read %d seed=%u\r",i,get_seed(i));
			if (CheckFile(path_output,i, get_seed(i))!=0)
				cn_err++;
		}
		printf("Force checking done errfile=%d\n",cn_err);
		goto l_end;
	}	
	gettimeofday(&start,0);	
	for(i=0;i<num_files;i++)
	{
		if (ToGenWriteCmd(tp_access)==1)
		{
			sz_file = (rand()%sz_max_file)+1;
			fprintf(stderr,"[%08d] write %d sz=%d seed=%u\r",i,cn_create,sz_file,get_seed(cn_create));
			if (GenerateFile(path_output,cn_create, sz_file, get_seed(cn_create))<0)
				break;
			cn_create++;
		}
		else
		{
			if (cn_create==0)
			{
				printf("no any files for checking %d\n",i);
				continue;
			}
			id_file = rand()% cn_create;
			fprintf(stderr,"[%08d] read %d seed=%u cn_create=%d\r",i,id_file,get_seed(id_file),cn_create);
			if (CheckFile(path_output,id_file, get_seed(id_file))>0)
				printf("read %d fail when finish %d write\n",id_file,cn_create);
			cn_check++;
		}
	}
	gettimeofday(&end,0);	
	diff=(long long)(end.tv_sec*1000000+end.tv_usec)-(long long)(start.tv_sec*1000000+start.tv_usec);	
	printf("TotalTime %lld us CreateFile=%d ChkFile=%d\n",diff,cn_create,cn_check);
l_end:
	FILE* fp = fopen("/proc/sys/vm/drop_caches","w");
	printf("Drop cache fp=%x\n",fp);	
	if (fp)
		fprintf(fp,"3\n");
	fclose(fp);
	return 0;
}