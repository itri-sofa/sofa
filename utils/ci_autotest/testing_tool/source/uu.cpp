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
#include "include/bfun.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <time.h>


int sim_LFSM_access_Ex(CVSSD& ssd, unsigned char* pBuf, int isW, int cn_pages, int cn_used, int round)
{
	int i,j;
	unsigned char buf_sample[PAGE_SIZE];
	unsigned char empty_sample[PAGE_SIZE];
	memset(empty_sample, 0xff, PAGE_SIZE);
	int cn_blocks = CN_BLOCK_OF_SEU; //16
	int cn_pages_in_sblk = cn_pages * cn_blocks;
	int cn_superblk = CN_SBLK; // 1SB = 4MB
//	int round = cn_pages_in_sblk*1*3;
//	int round = 160;
	char ar_used_sblk[CN_SBLK];
	int sel_active=2;
	int ar_active_sblk[3];
	int fail_count = 0;
	unsigned int ar_used_active_sblk[3];
	ar_used_active_sblk[0]=0xffffffff;
	ar_used_active_sblk[1]=0xffffffff;
	ar_used_active_sblk[2]=0xffffffff;
	int new_sel_active;
	unsigned long addr;
	int ret;
	memset(ar_used_sblk,0,CN_SBLK);
//	srand(1);
	FILE* fp_log;
	fp_log=fopen("errpage.log","wt");
	if (fp_log==0)
	{
		perror("open file error");
		getch();
		return 0;
	}
	for(i=0;i<round;i++)
	{
		sel_active = rand()%3;
		sel_active = 0;
//		sel_active = (sel_active+1)%3;
		if (ar_used_active_sblk[sel_active]>=cn_pages_in_sblk)
		{
			do {
//				new_sel_active = (rand()%CN_SBLK);
				new_sel_active = cn_used;
				cn_used++;
			}while (ar_used_sblk[new_sel_active]>0);
			ar_used_sblk[new_sel_active] = 1;
			ar_used_active_sblk[sel_active] = 0;
			ar_active_sblk[sel_active] = new_sel_active;
			printf("Assign a new active sblk %d for class %d\n", new_sel_active, sel_active);
		}
		addr = ar_active_sblk[sel_active] * cn_pages_in_sblk + ar_used_active_sblk[sel_active];
		ar_used_active_sblk[sel_active]++;
		fail_count = 0;
		if (isW)
		{
			SeqFill(addr, PAGE_SIZE, pBuf);
//			RandomFill(addr,PAGE_SIZE,pBuf);
			if (ssd.WriteByLPN(addr, 1, pBuf)==false)
			{
				printf("WriteByLPN failure %lu\n",addr);
				fclose(fp_log);
				return false;
			}
			printf("Write LPN %d done\n",addr);
		}
		else
		{
			if (ssd.ReadByLPN(addr, 1, pBuf)==false)
			{
				fclose(fp_log);
				printf("ReadByLPN failure %d\n",addr);
				return false;
			}
			
			ret = Check_data(addr, pBuf, 1);
			if (ret==0)
			{
				fprintf(fp_log,"LPN %d\n", addr);
				printf("LPN %d off=%d: compare failure\n",addr,addr%2048);
				fail_count++;
			
			}
//			printf("Read LPN %d done return type %d \n",addr,ret);
//			return ret;			
		}		
	}
	for(i=0;i<1;i++)
		printf("SB %d Last Page = %d \n",ar_active_sblk[i] ,ar_used_active_sblk[i]);
		
	fclose(fp_log);		
	if ( fail_count <=0 )
	{
		if (isW)
		{
			printf("Write Test is OK!\n");
			return true;
		}	
		else
		{
			printf("Read Test is OK!\n");		
			return ret;
		}	
	}
}

