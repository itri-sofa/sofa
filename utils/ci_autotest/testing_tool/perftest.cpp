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
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <time.h>


#ifdef WIN32
#include "include/vssd.h"
#include <conio.h>
#else
#include "include/alssd2.h"
#endif
#include "mem512.h"
#include "bfun.h"
#include "ssdconf.h"
#include "cftest.h"
#include "addrman.h"
#include "perftest.h"
bool gSlience=false;

int pf_var_write(CVSSD& ssd, unsigned long addr, int len)
{
	long long diff;
	struct timeval start,end;
	unsigned char* pBuf;
	gettimeofday(&start,0);
	pBuf=ssd.Cbuf.Allocate(len);
	if (ssd.WriteByLPN(addr,len,pBuf)==false)
		return false;
	ssd.WaitAllEventDone(1);
	gettimeofday(&end,0);
	diff=(long long)(end.tv_sec*1000000+end.tv_usec)-(long long)(start.tv_sec*1000000+start.tv_usec);
	printf("write %d pages : %lld usec. each %lf usec iops=%lf\n",
		len, diff,((double)(diff)),1000000/(((double)(diff))));
	return 1;

}
int pf_4K_write(CVSSD& ssd, unsigned long addr, unsigned long round, bool HasPattern, int len, int skip)
{
	unsigned long i;
	long long diff;
	struct timeval start,end;
	unsigned char* pBuf;
	unsigned long offset;
//	rand_len=rand()%32+1;
	if (HasPattern==false)
		gSlience=true;
	gettimeofday(&start,0);

	for(i=0;i<round;i++)
	{
		pBuf = ssd.Cbuf.Allocate(len);
		offset=addr+(i*len*skip);
//		offset = ((offset/128)*128) + (127-(offset%128));
		if (HasPattern)
			PageBaseFill(offset , len, pBuf);
		if (ssd.WriteByLPN(offset, len, pBuf)==false)
		{
			printf("WriteByLPN failure %lu\n",offset);
			return false;
		}

	}
	ssd.WaitAllEventDone(1);
	gettimeofday(&end,0);
	diff=(long long)(end.tv_sec*1000000+end.tv_usec)-(long long)(start.tv_sec*1000000+start.tv_usec);
	round-=ssd.cn_outcommand;
	printf("%dk write for %lld round: %lld usec. each %.3lf usec iops=%.3lf 4k-iops=%.3lf\n",
		4*len, round, diff,((double)(diff))/round,
		1000000/(((double)(diff))/round),
		(1000000/(((double)(diff))/round))*len);

}


int pf_4K_write_with_randread(CVSSD& ssd, unsigned long addr, unsigned long round, bool HasPattern, int len)
{
	unsigned long i;
	int rand_len;
	long long diff;
	struct timeval start,end;
	unsigned char* pBuf;
//	rand_len=rand()%32+1;
	rand_len=len;
	if (HasPattern==false)
		gSlience=true;
	gettimeofday(&start,0);
	for(i=0;i<round;i++)
	{
		pBuf = ssd.Cbuf.Allocate(rand_len);
		if (HasPattern)
			PageBaseFill(addr+(i*rand_len) , rand_len, pBuf);
		if (ssd.WriteByLPN(addr+i*rand_len, rand_len, pBuf)==false)
		{
			printf("WriteByLPN failure %lu\n",addr);
			return false;
		}
//		if ((rand()%10)>8)
//		{
//			if (ssd.ReadByLPN(addr+(lrand48() % (round - rand_len)), rand_len, pBuf)==false)
			if (ssd.ReadByLPN(179880/8, rand_len, pBuf)==false)
			{
				printf("ReadByLPN failure %lu\n",addr);
				return false;
			}
//		}

	}
	ssd.WaitAllEventDone(1);
	gettimeofday(&end,0);
	diff=(long long)(end.tv_sec*1000000+end.tv_usec)-(long long)(start.tv_sec*1000000+start.tv_usec);
	round-=ssd.cn_outcommand;
	printf("%dk write for %d round: %lld usec. each %.3lf usec iops=%.3lf 4k-iops=%.3lf\n",
		4*rand_len, round, diff,((double)(diff))/round,
		1000000/(((double)(diff))/round),
		(1000000/(((double)(diff))/round))*rand_len);

}


int pf_4K_batchwrite(CVSSD& ssd,unsigned long addr, unsigned long round, bool HasPattern, bool isRand, unsigned long range, bool StepDisplay)
{
	return pf_4K_batch(ssd,1,addr,round, HasPattern,isRand,range,StepDisplay);
}
int pf_4K_batchread(CVSSD& ssd,unsigned long addr, unsigned long round, bool HasPattern, bool isRand, unsigned long range, bool StepDisplay)
{
	return pf_4K_batch(ssd,0,addr,round, HasPattern,isRand,range,StepDisplay);
}

int pf_4K_batch(CVSSD& ssd,int op, unsigned long addr, unsigned long round, bool HasPattern, bool isRand, unsigned long range, bool StepDisplay)
{
	unsigned long i;
	int rand_len, j;
	long long diff;
	struct timeval start,end;
	unsigned char* pBuf;
	long long ar_addr[BATCH_WRITE_DEPTH];
//	rand_len=rand()%32+1;
	rand_len=1;
	round>>=BATCH_WRITE_DEPTH_ORDER;
	if ((round<200)&&(StepDisplay))
	{
		printf("round should be larger than %d for cmd perfrbsw_step\n",BATCH_WRITE_DEPTH*200);
		return 0;
	}
	if (HasPattern==false)
		gSlience=true;
	gettimeofday(&start,0);
	for(i=0;i<round;i++)
	{
		pBuf = ssd.Cbuf.Allocate(BATCH_WRITE_DEPTH*rand_len);
		for(j=0;j<BATCH_WRITE_DEPTH;j++)
		{
			if (isRand)
				ar_addr[j] = addr + (lrand48() % range);
			else
				ar_addr[j] = addr + ((i*BATCH_WRITE_DEPTH + j) * rand_len * 1 ) % range;
			if (HasPattern)
			{
				if (op==0)
					ssd.SetFunc(Check_data);
				else if (op==1)
					PageBaseFill(ar_addr[j], rand_len, pBuf+(PAGE_SIZE*j*rand_len));
			}
		}
		if (op==0)
		{
			if (ssd.BatchReadByLPN(ar_addr, rand_len, pBuf)==false)
			{
				printf("ReadByLPN failure %lu\n",addr);
				return false;
			}
		}
		else if (op==1)
		{
			if (ssd.BatchWriteByLPN(ar_addr, rand_len, pBuf)==false)
			{
				printf("WriteByLPN failure %lu\n",addr);
				return false;
			}
		}
		if ((StepDisplay)&&(i%400==399))
		{
			gettimeofday(&end,0);
			diff=(long long)(end.tv_sec*1000000+end.tv_usec)-(long long)(start.tv_sec*1000000+start.tv_usec);
			printf("%d %.3lf\n", getpid(),
				1000000/( ((double)diff) / (400*BATCH_WRITE_DEPTH) ) );
			fflush(stdout);
			gettimeofday(&start,0);
		}

	}
	ssd.WaitAllEventDone(1);
	if (StepDisplay==false)
	{
		gettimeofday(&end,0);
		round<<=BATCH_WRITE_DEPTH_ORDER;
		round-=ssd.cn_outcommand;
		diff=(long long)(end.tv_sec*1000000+end.tv_usec)-(long long)(start.tv_sec*1000000+start.tv_usec);
		printf("%dk batch %s %s(%d) for %d round: %lld usec. each %.3lf usec iops=%.3lf\n",
			4,(isRand)?"random":"seq",(op==0)?"read":"write", rand_len, round, diff,((double)(diff))/round,
			1000000/(((double)(diff))/round));
	}
}

int pf_4K_jump_read(CVSSD& ssd, unsigned long addr, unsigned long round, bool HasPattern, int len, int skip)
{
	if (HasPattern)
		ssd.SetFunc(Check_data);
	else
		gSlience=true;

	return pf_4K_jump_read_A(ssd, addr,round,HasPattern,len,skip);
}

int pf_4K_read(CVSSD& ssd, unsigned long addr, unsigned long round, bool HasPattern)
{
	if (HasPattern)
		ssd.SetFunc(Check_data);
	else
		gSlience=true;

	return pf_4K_read_A(ssd, addr,round,HasPattern);
}


int pf_4K_read_hybrid(CVSSD& ssd, unsigned long addr, unsigned long round, bool HasPattern)
{
	if (HasPattern)
		ssd.SetFunc(Check_data_hybrid);
	else
		gSlience=true;

	return pf_4K_read_A(ssd, addr,round,HasPattern);
}



int pf_4K_jump_read_A(CVSSD& ssd, unsigned long addr, unsigned long round, bool HasPattern, int len, int skip)
{
	unsigned long i=0;
	unsigned long count=0;	
	long long diff;
	struct timeval start,end;
	unsigned char* pBuf;
	unsigned long offset;
//	rand_len=rand()%32+1;

	gettimeofday(&start,0);

//	for(i=0;i<round;i++)
	while(i<round)
	{
		pBuf = ssd.Cbuf.Allocate(len);
		offset = addr+(i*len*skip);
		if (ssd.ReadByLPN(offset, len, pBuf)==false)
		{
			printf("ReadByLPN failure %lu\n",addr);
			return false;
		}
		i++;
//		if ((i%16)==8)
//			i+=8;
		count++;

	}
	ssd.WaitAllEventDone(1);
	gettimeofday(&end,0);
	diff=(long long)(end.tv_sec*1000000+end.tv_usec)-(long long)(start.tv_sec*1000000+start.tv_usec);
	printf("%dk read for %d round: %lld usec. each %lf usec iops=%lf\n",
		4*len, count, diff,((double)(diff))/count,1000000/(((double)(diff))/count));
}


int pf_4K_read_A(CVSSD& ssd, unsigned long addr, unsigned long round, bool HasPattern)
{
	unsigned long i=0;
	unsigned long count=0;
	int rand_len;
	long long diff;
	struct timeval start,end;
	unsigned char* pBuf;
//	rand_len=rand()%32+1;
	rand_len=1;
	gettimeofday(&start,0);

//	for(i=0;i<round;i++)
	while(i<round)
	{
		pBuf = ssd.Cbuf.Allocate(rand_len);
		if (ssd.ReadByLPN(addr+i, rand_len, pBuf)==false)
		{
			printf("ReadByLPN failure %lu\n",addr);
			return false;
		}
		i++;
//		if ((i%16)==8)
//			i+=8;
		count++;

	}
	ssd.WaitAllEventDone(1);
	gettimeofday(&end,0);
	diff=(long long)(end.tv_sec*1000000+end.tv_usec)-(long long)(start.tv_sec*1000000+start.tv_usec);
	printf("%dk read for %d round: %lld usec. each %lf usec iops=%lf\n",
		4*rand_len, count, diff,((double)(diff))/count,1000000/(((double)(diff))/count));
}

int* gar_LatestValue=0;
unsigned long gOffsetLatestValueAR=0;
int g_version=0;
FILE* err_fp;

int load_offsetLatest(FILE* log_fp, long long int loadrange,int *alloc_mem)
{
	long long int i; 
	for(i=0;i<loadrange;i++)
		fscanf(log_fp,"%d",&alloc_mem[i]);
}
int dump_offsetLatest(long long int log_start , long long int log_range, int *alloc_mem)
{
	long long int i;
	FILE* log_fp;
	log_fp=fopen("ttest_last_write.log","w");
	if(log_fp==NULL)
	{		
		printf("dump log err\n");
		exit(0);
	}
	fprintf(log_fp," %lld %lld %d\n",log_start,log_range,g_version);
	for(i=0;i<log_range;i++)
		fprintf(log_fp,"%d\n",alloc_mem[i]);
	
	fclose(log_fp);
	printf("dump ttest log end\n");
	
}


int hybrid_randswsr(CVSSD& ssd, unsigned long addr,unsigned long range, unsigned long round, int Seed, int len, bool check_all,bool is_write)
{

	long int seed;
	long long int log_range;
	long long int log_start;
	long long int old_range;
	long long int old_start;
	int *alloc_mem=NULL;
	srand(time(0));
	FILE* log_fp;
	if((err_fp=fopen("ttest.err","a"))==NULL)
	{
		printf("error open errlog file, exit");
		exit(0);
	}

	
	if((log_fp=fopen("ttest_last_write.log","r"))==NULL)
	{
		printf("cannot find ttest_last_write.log, use all new log\n");
		alloc_mem=gar_LatestValue=(int*)malloc(range*sizeof(int));
		ASSERT(gar_LatestValue,"gar_LatestValue alloced failure\n");
		gOffsetLatestValueAR=addr;
		g_version=rand();
		memset(gar_LatestValue,0,range*sizeof(int));
		log_start=addr;
		log_range=range;
	}
	else
	{
		fscanf(log_fp," %lld %lld %d\n",&old_start,&old_range,&g_version);
		printf("read old_start %lld old_range %lld %d %d \n",old_start,old_range, addr,range);
		log_start=(addr>old_start)?old_start:addr;
		log_range=((addr+range)>(old_start+old_range))?((addr+range)-log_start):((old_start+old_range)-log_start);		
		printf("calc new_start %lld new_range %lld\n",log_start,log_range);
		alloc_mem=(int*)malloc(log_range*sizeof(int));
		
		memset(alloc_mem,0,log_range*sizeof(int));
		load_offsetLatest(log_fp,old_range,alloc_mem+(old_start-log_start));
		gar_LatestValue =alloc_mem+addr-log_start;
		gOffsetLatestValueAR=addr;
		
		fclose(log_fp);
	}
//	printf("plz input rand seed:");
//	scanf("%ld",&seed);

	if(is_write)
	{
		printf("start to 4k sequetial write from %d for range=%d \n",addr,range);
		if(pf_4K_randwriteA(ssd,addr,range,round,true,len,false)==false)
			printf("FAIL! Quit\n");

	}
	else
	{
		if(check_all)
		{
			printf("start to 4k sequetial read from %d for range=%d \n",addr,range);
			pf_4K_read_hybrid(ssd,addr,range,true);
		} 
		else
		{
			printf("start to 4k random read from %d for range=%d \n",addr,range);
			pf_4K_randread_hybrid(ssd,addr,range,round,true,len);
			
		}
		
	}
	
	if(is_write)
		dump_offsetLatest(log_start ,log_range,alloc_mem);
	free(alloc_mem);
	fclose(err_fp);
	printf("exec done\n");
	
}

int randsw_seqsr(CVSSD& ssd, unsigned long addr,unsigned long range, unsigned long round, int Seed, int len, bool isSim)
{

	long int seed;
	gar_LatestValue=(int*)malloc(range*sizeof(int));
	ASSERT(gar_LatestValue,"gar_LatestValue alloced failure\n");
	gOffsetLatestValueAR=addr;
	memset(gar_LatestValue,0,range*sizeof(int));


//	printf("plz input rand seed:");
//	scanf("%ld",&seed);
	printf("start to 4k rand %s from %d for round=%d within range=%d\n",
		isSim?"SimuWrite":"write", addr,round,range);
	srand48(Seed);
	if (isSim || pf_4K_randwriteA(ssd,addr,range,round,true,len)==true)
	{
		printf("start to 4k sequetial read from %d for range=%d \n",addr,range);
		//srand48(seed);
		pf_4K_read(ssd,addr,range,true);
	}
	else
		printf("FAIL!Quit\n");
	free(gar_LatestValue);
}

int pf_4K_randwrite(CVSSD& ssd, unsigned long addr,unsigned long range, unsigned long round, bool HasPattern, int len)
{
	return pf_4K_randwriteA(ssd, addr, range, round, HasPattern, len);
}

int pf_4K_randwriteA(CVSSD& ssd, unsigned long addr,unsigned long range, unsigned long round, bool HasPattern, int len)
{
	unsigned long i;
	bool ret=true;
	int rand_len,j;
	long long diff;
	struct timeval start,end;
	unsigned char* pBuf;
	unsigned long offset;
//	int multipler;
	if (HasPattern==false)
		gSlience=true;

	gettimeofday(&start,0);
	for(i=0;i<round;i++)
	{
		if (len==-1)
			rand_len=rand()%32+1;
		else
			rand_len=len;
		pBuf = ssd.Cbuf.Allocate(rand_len);
		if (pBuf==0)
		{
			printf("%s %d Cbuf Allocate failure\n",__FILE__,__LINE__);
			ret=false;
			goto l_end;
		}

//		multipler=(range - rand_len)/32;
//		offset = addr + ((lrand48() % multipler)*32);
//		printf("multipler=%d, offset=%d\n",multipler,offset);
		offset = addr + lrand48() % (range - rand_len);
//		offset = addr + rand_len*i;
		if (HasPattern)
		{
			if (gar_LatestValue)
			{
				for(j=0;j<rand_len;j++)
					gar_LatestValue[offset+j-gOffsetLatestValueAR]++;
			}
			PageBaseFill(offset, rand_len, pBuf);
		}
//		printf("rand_len=%d\n",rand_len);
		if (ssd.WriteByLPN(offset, rand_len, pBuf)==false)
		{
			printf("[%s:%d] WriteByLPN failure %lu\n",__FUNCTION__,__LINE__,addr);
			return false;
		}

	}
l_end:
	ssd.WaitAllEventDone(1);
	gettimeofday(&end,0);
	diff=(long long)(end.tv_sec*1000000+end.tv_usec)-(long long)(start.tv_sec*1000000+start.tv_usec);
	printf("%dk write for %d round: %lld usec. each %lf usec iops=%lf\n",
		4*rand_len, round, diff,((double)(diff))/round,1000000/(((double)(diff))/round));
	if (HasPattern==false)
		gSlience=false;
	return ret;
}


int pf_4K_randread_hybrid(CVSSD& ssd, unsigned long addr, unsigned long range, unsigned long round, bool HasPattern, int len)
{
	if (HasPattern)
		ssd.SetFunc(Check_data_hybrid);
	else
		gSlience=true;
	
	return pf_4K_randread_A(ssd, addr,range,round,HasPattern, len);
}

int pf_4K_randread(CVSSD& ssd, unsigned long addr, unsigned long range, unsigned long round, bool HasPattern, int len)
{
	if (HasPattern)
		ssd.SetFunc(Check_data);
	else
		gSlience=true;

	return pf_4K_randread_A(ssd, addr,range,round,HasPattern, len);

	
}
int pf_4K_randread_A(CVSSD& ssd, unsigned long addr, unsigned long range, unsigned long round, bool HasPattern, int len)
{
	unsigned long i;
	int rand_len;
	long long diff;
	struct timeval start,end;
	unsigned char* pBuf;
	unsigned char Pattern[4096];
	unsigned long offset;

	gettimeofday(&start,0);
	for(i=0;i<round;i++)
	{
		if (len==-1)
			rand_len=rand()%32+1;
		else
			rand_len=len;
		pBuf = ssd.Cbuf.Allocate(rand_len);
		ASSERT(pBuf,"fail to allocate buf\n");
		offset = addr + lrand48() % (range - rand_len);
		if (ssd.ReadByLPN(offset, rand_len, pBuf)==false)
		{
			printf("ReadByLPN failure %lu\n",addr);
			return false;
		}

	}
	ssd.WaitAllEventDone(1);
	gettimeofday(&end,0);
	diff=(long long)(end.tv_sec*1000000+end.tv_usec)-(long long)(start.tv_sec*1000000+start.tv_usec);
	printf("%dk read for %d round: %lld usec. each %lf usec iops=%lf\n",
		4*rand_len,round, diff,((double)(diff))/round,1000000/(((double)(diff))/round));
	if (HasPattern==false)
		gSlience=false;
}

int log_replay_chk(CVSSD& ssd, char* filename, unsigned long offset, unsigned long range)
{
	gar_LatestValue=(int*)malloc(range*sizeof(int));
	ASSERT(gar_LatestValue,"gar_LatestValue alloced failure\n");
	gOffsetLatestValueAR=offset;
	memset(gar_LatestValue,0,range*sizeof(int));

	log_replay(ssd,filename,offset,range, 0);
	printf("start to 4k sequetial read from %d for range=%d \n",offset,range);
	if (pf_4K_read(ssd,offset,range,true)==false)
		printf("fail to read all back to check\n");
	free(gar_LatestValue);
	return 0;
}
long long time2us(struct timeval tm)
{
	return (long long)(tm.tv_sec*1000000) + tm.tv_usec;

}
int log_replay(CVSSD& ssd, char* filename, unsigned long offset, unsigned long range, int fg_delay)
{
	int cn;
	int j;
	int ret;
	FILE* fp;
	unsigned char *pBuf;
	char szBuf[512];
	char nm_process[32];
	fp = fopen(filename,"r");
	if (fp==NULL)
	{
		printf("Fail to open %s\n",filename);
		return -1;
	}
	char cmd;
	char type;
	double tm_submit, tm_start;
	unsigned long long addr,addr2;
	unsigned long len;
//	unsigned int tag;
//	unsigned int wait;
	double time;
	unsigned long total_len=0;
	unsigned int count=0;
	int skip_count=0;
	struct timeval start,end,s2,e2, curr;
	long long diff=0;

	ssd.SetFunc(Check_data);
	gettimeofday(&start,0);
	gettimeofday(&s2,0);
	tm_start = time2us(start);
	gSlience=true;
	while(feof(fp)==false)
	{
		cn=fscanf(fp,"%c %llu %lu %lf\n",&cmd,&addr,&len,&tm_submit);
	//	printf("[%c] %llu %lu %lf\n",cmd,addr,len,tm_submit);
		gettimeofday(&curr,0);
		diff = tm_submit - (time2us(curr)-tm_start);
		if ((diff>0)&&(diff<1000000))
		{
			if ((ret=usleep(diff))<0)
				exit(ret);
		}
		else if (diff>1000000)
		{
			sleep(diff/1000000);
			usleep(diff%1000000);
		}
		addr=addr+offset;
		if (cmd=='N')
		{
			skip_count++;
			continue;
		}
		if ((range>0) && ((addr>(range+offset)) || addr<0))
		{
			skip_count++;
	//		printf("skip io %c %llu %llu\n", cmd, addr, len);
			continue;
		}
		if (len==0)
		{
			skip_count++;
			continue;
		}
		total_len+=len;
		count++;

		while ((pBuf = ssd.Cbuf.Allocate(len))==0)
		{
			printf("fail to allocate %d  pages\n",len);
			ssd.WaitAllEventDone(1);				
		}
		if (cmd=='r' || cmd=='R')
		{
			ssd.ReadByLPN(addr,len,pBuf);
		}
		if (cmd=='w' || cmd=='W')
		{

			if (gar_LatestValue)
			{
				for(j=0;j<len;j++)
					gar_LatestValue[addr+j-gOffsetLatestValueAR]++;
			}
			PageBaseFill(addr, len, pBuf);
			ssd.WriteByLPN(addr,len,pBuf);
		}

		if((count%1000)==0)
		{
			gettimeofday(&e2,0);
			diff=(long long)(e2.tv_sec*1000000+e2.tv_usec)-(long long)(s2.tv_sec*1000000+s2.tv_usec);
			printf("[%d] workload for %llu usec.\n",count, diff);
			gettimeofday(&s2,0);
		}

//		printf("[%d] workload for %llu usec.\n",count, diff);
//		fflush(stdout);
//		if (wait)
//		ssd.WaitAllEventDone(1);

	}
	ssd.WaitAllEventDone(1);
	gettimeofday(&end,0);
	fclose(fp);
	diff=(long long)(end.tv_sec*1000000+end.tv_usec)-(long long)(start.tv_sec*1000000+start.tv_usec);
	printf("skip number %d total len=%lu total count %u\n",skip_count,total_len,count);
	printf("replay workload for %llu usec.\n", diff);
	return 0;
}
