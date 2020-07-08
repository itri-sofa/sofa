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

unsigned char getch() 
{ 
	char ch;
	scanf("%d",&ch); 
	return ch;
}

//int CN_BLOCK_OF_SEU;
//int CN_PAGES;

//#define CN_SBLK 2046
//#define Class 3
/*
#if VENDOR == 1
        #if CE_NUM == 2
                #define CN_BLOCK_OF_SEU 32
                #define CN_PAGES 64
        #else
                #define CN_BLOCK_OF_SEU 16
                #define CN_PAGES 64
        #endif 
#elif VNEDOR == 0
        #define CN_BLOCK_OF_SEU 16
        #define CN_PAGES 128
//#error "ABCCD"
#endif
*/


int sim_LFSM_access_Ex(CVSSD& ssd, int isW, int cn_pages, int cn_used, int round)
{
	int i,j;
	unsigned char buf_sample[PAGE_SIZE];
	unsigned char empty_sample[PAGE_SIZE];
	unsigned char buf[PAGE_SIZE] __attribute__ ((aligned(PAGE_SIZE)));
	unsigned char * pBuf;
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
		
#ifdef ALSSD2_H
		pBuf = ssd.Cbuf.Allocate(1);
		ssd.SetFunc(Check_data);
#else
		pBuf = buf;		
#endif		
		if (isW)
		{
			PageBaseFill(addr, 1, pBuf);
//			RandomFill(addr,PAGE_SIZE,pBuf);
			if (ssd.WriteByLPN(addr, 1, pBuf)==false)
			{
				printf("WriteByLPN failure %lu\n",addr);
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
#ifndef ALSSD2_H
			ret = Check_data(addr, pBuf, 1);
			if (ret==0)
			{
				fprintf(fp_log,"LPN %d\n", addr);
				printf("LPN %d off=%d: compare failure\n",addr,addr%2048);
				fail_count++;
			
			}
			printf("Read LPN %d done return type %d \n",addr,ret);
#endif

		}
	}
#ifdef ALSSD2_H
        ssd.WaitAllEventDone(isW);
#endif
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

int sim_LFSM_W(AddrManer& adm, CVSSD& ssd, int round, int s_range, int s_offset)
{
        int addr=0;
        int i;
        unsigned char * pBuf;
        unsigned char buf[PAGE_SIZE] __attribute__ ((aligned(PAGE_SIZE)));                
                
        for (i=0; i<round; i++)
        {
#ifdef ALSSD2_H
                /// memory locate a new buffer ///
                pBuf = ssd.Cbuf.Allocate(1);
#else
                pBuf=buf;
#endif	

                addr = adm.AlloAddr(1, s_range, s_offset);
//                printf("addr %d\n",addr);        

		PageBaseFill(addr, 1, pBuf);
		if (ssd.WriteByLPN(addr, 1, pBuf)==false)
		{
			printf("WriteByLPN failure %lu\n",addr);
			return false;
		}
		printf("Write LPN %d command submit\n",addr);
        }
        return true;

}
int sim_LFSM_R(AddrManer& adm, CVSSD& ssd, int round, int s_range, int s_offset)
{
        int seu=0;
        int offset=0;
        int addr=0;
        int i;
        int cn_pages_in_sblk=CN_BLOCK_OF_SEU*CN_PAGES;
        int ret;
        
        unsigned char * pBuf;
        unsigned char buf[PAGE_SIZE] __attribute__ ((aligned(PAGE_SIZE)));                

        
        seu = ((rand()%s_range)+s_offset)%CN_SBLK;
        offset = rand()%cn_pages_in_sblk;	
        addr = seu * cn_pages_in_sblk + offset;
        for(i=0; i<round; i++)
        {		
#ifdef ALSSD2_H
                /// memory locate a new buffer ///
                pBuf = ssd.Cbuf.Allocate(1);
#else
                pBuf=buf;
#endif	
                addr=addr+i;
                if (ssd.ReadByLPN(addr, 1, pBuf)==false)
                {
                        printf("ReadByLPN failure %d\n",addr);
                        return false;
                }
        }
#ifndef ALSSD2_H
        ret = Check_data(addr, pBuf, round);
/*        
        if ( (offset) >= ar_used_sblk[seu] && ret==1)
                printf("Read LPN %d done is empty \n",addr);	
        else if( (offset) < ar_used_sblk[seu] )
        {
                if (ret==2)
                        printf("Read LPN %d done is sequential \n",addr);
                else
                {
                        printf("Read LPN %d done is fail report",addr);				
				return false;
                }
        }
*/        
#endif	
        return true;			

}
int sim_LFSM_E(AddrManer& adm, CVSSD& ssd, int s_range, int s_offset)
{
        int new_sel_active;
        new_sel_active = adm.FreeSEU(s_range,s_offset);

        bLFSM_erase(new_sel_active*CN_BLOCK_OF_SEU*CN_PAGES*8,CN_BLOCK_OF_SEU);
        printf("Erase a used sblk %d \n", new_sel_active);
//        sleep(1);
        
        return true;

}
int sim_LFSM_C(AddrManer& adm, CVSSD& ssd, int round, int s_range, int s_offset)
{
        int seu=0;
        int offset=0;
        int addr=0;
        int dest=0;
        int cn_pages_in_sblk=CN_BLOCK_OF_SEU*CN_PAGES;
        int trycount=0;

        do{
                seu = ((rand()%s_range)+s_offset)%CN_SBLK;
                offset = rand()%cn_pages_in_sblk;
        	addr = seu * cn_pages_in_sblk + offset;
        	trycount++;
        	if (trycount>100)
        	{
        	        printf("No address is written so quit copy\n");
        	        return false;
        	}        
	}while(adm.IsAllocate(addr)==false);
		
	dest = adm.AlloAddr(round, s_range, s_offset);
        printf("addr %d\n",addr);        

	bLFSM_copy(addr*8,dest*8,round);	
        adm.WriteDoneCallback(addr);
	printf("Copy from lpn %d to lpn %d in length %d \n", addr, dest, round);
//	sleep(1);
	
	return true;

}

int sim_LFSM_access_mix(CVSSD& ssd, int cn_pages, int times, int len, int s_offset)
{

        AddrManer adm;
        int count=0;
        int stepcount=0;
	int i,j;
	int op_ran=0;
//	unsigned char buf_sample[PAGE_SIZE];
//	unsigned char empty_sample[PAGE_SIZE];
//	unsigned char buf[PAGE_SIZE] __attribute__ ((aligned(PAGE_SIZE)));
//	unsigned char *pBuf; //[PAGE_SIZE*MAX_PAGE] __attribute__ ((aligned(PAGE_SIZE)));
	int cn_blocks = CN_BLOCK_OF_SEU; //16
	int cn_pages_in_sblk = cn_pages * cn_blocks; //16*64 = 1024
	int cn_superblk = CN_SBLK; // 1SB = 4MB
//	char ar_used_sblk[CN_SBLK];  // table for seu writtern (0:unwrittern 1:writtern)
//	int sel_active=2;
//	int ar_active_sblk[Class];  //the selected seu of three class
//	unsigned int ar_used_active_sblk[Class];  // page filled number of the seu 
//	int new_sel_active;
//	unsigned long addr;
	int fail_count=0; 
	int ret;
	int isW;
//	int seu,dest;
	int round;
//	unsigned int offset;
	int s_range=6;
//	int s_offset=200;
	
//	for(i=0; i<Class; i++)
//		ar_used_active_sblk[i]=0xffffffff;
	 

//	memset(empty_sample,0,PAGE_SIZE);
	srand( 12345 ); //initial srand
//    srand48(time(0));	
    srand48(12345);
	/// error file open and check ///
	FILE* fp_log;
	fp_log=fopen("errpage.log","wt");
	if (fp_log==0)
	{
		perror("open file error");
		getch();
		return 0;
	}
	
#ifdef ALSSD2_H
        ssd.SetFunc(Check_data);
        ssd.SetADM(&adm);
#endif	

	for(j=0; j<times; j++)
	{
        	/* select SEU */
        	round = rand()%len+1; // random length
        	op_ran = rand()%100; 
//        	printf("op_ran=%d\n",op_ran);
        	if(op_ran>=99)
        	        isW=2;
                else if( (op_ran>=45) && (op_ran<99) )
                        isW=1;
                else if( (op_ran>=5) && (op_ran<45) )
                        isW=0;
                else if( (op_ran>=0) && (op_ran<5))
                        isW=0;                           
//                printf("case %d\n",isW);	        
                
        	if(isW==1)
        	{
	                ret = sim_LFSM_W(adm, ssd, round, s_range, s_offset);
        	}	
        	else if(isW==0)
        	{
	                ret = sim_LFSM_R(adm, ssd, round, s_range, s_offset);
        	}
        	else if(isW==2) // for erase
        	{
	                ret = sim_LFSM_E(adm, ssd, s_range, s_offset);
                        adm.PrintAddr();		                
                        count++;
        	}
        	else if(isW==3) // for copy
        	{
	                ret = sim_LFSM_C(adm, ssd, round, s_range, s_offset);	
        	}
        	stepcount++;
        	printf("step %d \n", stepcount);
//      	adm.PrintAddr();

	}
	printf("Erase_time count=%d\n",count);
#ifdef ALSSD2_H	
	/* check all event are finished */
	do
	{
		ret=ssd.WaitEvents(1);
		//printf("show cn_outcommand %d \n",ret);

	}while (ret>0);
#endif	
	

		
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
			return true;
		}	
	}
	
}

int sim_LFSM_access(CVSSD& ssd, int isW, int cn_pages)
{
	int round = 160;
	int cn_used=10;
	bool IsAcessOk;	

	printf("input the super block no:");
	scanf("%d",&cn_used);
	printf("input the length in pages:");
	scanf("%d",&round);

	IsAcessOk = sim_LFSM_access_Ex(ssd, isW, cn_pages, cn_used, round);
	
	return IsAcessOk;


}
int simple_read(CVSSD& ssd, unsigned long addr, unsigned long len, bool IsIntact)
{
	unsigned char pBuf[PAGE_SIZE*MAX_PAGE] __attribute__ ((aligned (PAGE_SIZE)));
	unsigned char buf_sample[PAGE_SIZE] __attribute__ ((aligned (PAGE_SIZE)));
//	unsigned char empty_sample[PAGE_SIZE] __attribute__ ((aligned(PAGE_SIZE)));
		
	int is_special=0;
	int i,j;
	int error;
	int ret;
#ifdef ALSSD2_H
        ssd.SetFunc(Check_data);
#endif	
	if (len>MAX_PAGE)
	{
		printf("Len should be smaller than %d\n",MAX_PAGE);
		return false;
	}
	SeqFill(addr, PAGE_SIZE, buf_sample);
	//PageBaseFill(addr, len, buf_sample);
	//memset(empty_sample,0xff,PAGE_SIZE);
	
	/* check read fail */
	if (is_special==1)
		addr|=0x08000000;
	if (ssd.ReadByLPN(addr,len,pBuf)==false)
	{
		printf("ReadByLPN failure %d\n",addr);
		return false;
	}
#ifdef ALSSD2_H
        ssd.WaitAllEventDone(0);
#endif

	/* print the result to consle */		
	for(i=0; i<len; i++)
	{	
		printf("Page %d\n",addr+i);
		dumphex(pBuf+(i*PAGE_SIZE));
	}
	
	/* compare data content */
	ret = Check_data(addr, pBuf, len);
	if (ret==0)
	{
		printf("LPN %d : compare failure but id=%d\n",addr,(*(unsigned long*)pBuf)==(*(unsigned long*)buf_sample));
		error=0;
		if(IsIntact)
		{
			printf("Do you want to list errors?");
			if (getch()!=1)
				return false;
			for(j=0;j<1024;j++)
			{
				if (((unsigned long*)pBuf)[j] != ((unsigned long*)buf_sample)[j])
				{
					printf("%d: offset=%d orig: %x read:%x\n",error, j*4,((unsigned long*)buf_sample)[j],((unsigned long*)pBuf)[j]);
					error++;
				}
			}
			printf("Total error count=%d\n", error);
		}
		return ret;
	}
	return ret;
}

int simple_write(CVSSD& ssd, unsigned long addr, unsigned long len, int seq)
{
	unsigned char pBuf[PAGE_SIZE*MAX_PAGE] __attribute__ ((aligned (PAGE_SIZE)));

	if (len>MAX_PAGE)
	{
		printf("Len should be smaller than MAX_PAGE\n");
		return false;
	}
	if (seq)
		//SeqFill(addr, PAGE_SIZE*len, pBuf);
		PageBaseFill(addr, len, pBuf);
	else
		RandomFill(addr,PAGE_SIZE*len,pBuf);
	if (ssd.WriteByLPN(addr,len,pBuf)==false)
		return false;
	printf("Write cmd done\n");
	
	return true;
}
int gSeed;
int main_interactive(void)
{
	int op;
	bool ret;
	int id_disk;
	int is_special=0;
	int error;
	int cn_pages;
	unsigned char buf[PAGE_SIZE*MAX_PAGE] __attribute__ ((aligned (PAGE_SIZE)));
	unsigned char buf_sample[PAGE_SIZE] __attribute__ ((aligned (PAGE_SIZE)));
	unsigned long i,j;
	unsigned long addr;
	unsigned long len;
	int ar_addr[30]={13, 14, 230, 240, 940, 1070, 1080, 1090, 1100, 1110, 1120, 1240, 1390, 1420, 1750, 1780, 2020, 2040};
	int seq=1;
	CVSSD ssd;
	printf("Please input the id disk:");
	scanf("%d",&id_disk);
	ret = ssd.Open(id_disk);
	ASSERT(ret,"Fail to open disk\n");
	printf("Disk Size: %llu GB\n", ssd.GetSize()>>21);
	printf("Input the page size per block:");
	scanf("%d",&cn_pages);
	memset(buf_sample,0,PAGE_SIZE);
	ssd.InitParameter(3,6);
#ifdef ALSSD2_H
        ssd.SetFunc(Check_data);
#endif	

l_begin:
	printf("Please input option: 1:R 2:W 3:W Test 7:single blk access 4: Read Test 5: Sim_LFSM_Write 6: SimRead, 9:");
	scanf("%d",&op);
	switch(op)
	{
	case 22:	        
	        printf("Plz input the start address (LPN), e.g. 262144:");
		scanf("%d",&addr);
		printf("Len=");
		scanf("%d",&len);		
		
		for(i=0;i<len;i++)
		{
                        PageBaseFill(i+addr, 1, buf+i*PAGE_SIZE);
                        if(ssd.WriteByLPN(i+addr,1,buf+i*PAGE_SIZE)==false)
	                        return false;        
                        printf("Write data into LPN %d done\n", i+addr);				
                }
//                ssd.WaitAllEventDone(0);
        
                bLFSM_erase(addr*8,len);
                
                for(i=0;i<len;i++)
		{        
        		PageBaseFill(i+addr, 1, buf+(len+i)*PAGE_SIZE);
			if ( ssd.ReadByLPN(i+addr, 1 , buf+(len+i)*PAGE_SIZE)==false)
			        return false;
		         printf("Read data into LPN %d done\n", i+addr);				
                	        				
		}
		
		ssd.WaitAllEventDone(0);
		for(i=0; i< len; i++)
		{
        		ret = Check_data ( addr+i, buf+(len+i)*PAGE_SIZE, 1);
//	        	dumphex(buf+(len+i)*PAGE_SIZE); 
	        	printf("ret= %d\n",ret);	                	                        
		}
		break;


	case 21:
                memset(buf+(16*PAGE_SIZE),0x5A,PAGE_SIZE);	
	        if(ssd.ReadByLPN(1024*7+5,1,buf+16*PAGE_SIZE)==false)
	                return false;
                ssd.WaitAllEventDone(0);	                
		dumphex(buf+16*PAGE_SIZE);
		getch();
	        SeqFill(1024*17,PAGE_SIZE*16,buf);
//	        SeqFill(1024*9,PAGE_SIZE,buf+PAGE_SIZE);	        
	        if(ssd.WriteByLPN(1024*17,16,buf)==false)
	                return false;
//                ssd.WaitAllEventDone(1);	                	                
//	        if(ssd.WriteByLPN(1024*9,1,buf+PAGE_SIZE)==false)
//	                return false;
//                ssd.WaitAllEventDone();
                memset(buf+(16*PAGE_SIZE),0x5A,PAGE_SIZE);
	        if(ssd.ReadByLPN(1024*7+5,1,buf+16*PAGE_SIZE)==false)
	                return false;
                ssd.WaitAllEventDone(0);	                
		dumphex(buf+16*PAGE_SIZE); 
		break;
	case 12:
		printf("Addr to read=");
		scanf("%d",&addr);
		printf("Len=");
		scanf("%d",&len);
		bLFSM_erase(addr*CN_BLOCK_OF_SEU*cn_pages*8,len);	
		break;
	case 10:
//		for(i=19;i>=0;i--)
		for(i=2031;i>651;i-=10)
		{
			printf("super eu=%d write\n",i);
			if (sim_LFSM_access_Ex(ssd,1,cn_pages,i,8)==false)
				return 0;
		}
		break;
	case 11:
		for(i=1;i<2051;i+=10)
		{
			printf("super eu=%d read\n",i);
			if (sim_LFSM_access_Ex(ssd,0,cn_pages,i,8)==false)
				break;
		}
		break;	
	case 5:
		sim_LFSM_access(ssd,1,cn_pages);
		break;
	case 6:
		sim_LFSM_access(ssd,0,cn_pages);
		break;

	case 1:
		printf("Addr to read=");
		scanf("%d",&addr);
		printf("Len=");
		scanf("%d",&len);
		simple_read(ssd, addr, len, 1);
		printf("Press any key to continue..\n");
		getch();
		break;
	case 2:
		printf("Addr to write=");
		scanf("%d",&addr);
		printf("Len=");
		scanf("%d",&len);
		printf("SeqPattern?");
		scanf("%d",&seq);
		simple_write(ssd, addr, len, seq);
		break;
	case 3:
		printf("Plz input the start address, e.g. 262144:");
		scanf("%d",&addr);
		printf("Len=");
		scanf("%d",&len);
		printf("Write TEst! Press Y to go\n");
		if (getch()!=1)
		{
			printf("Abort!\n");
			break;
		}

		for(i=addr;i<addr + len;i++)
		{
			SeqFill(i, PAGE_SIZE, buf);
			ret = ssd.WriteByLPN(i, 1, buf);
			printf("Write data into LPN %d with result:%d\r", i, ret);
		}
		break;
	case 4:
		printf("Plz input the start address, e.g. 262144:");
		scanf("%d",&addr);
		printf("Len=");
		scanf("%d",&len);
		for(i=addr;i<addr + len;i++)
		{
			SeqFill(i, PAGE_SIZE, buf_sample);
			ssd.ReadByLPN(i, 1, buf);
			printf("Read data into LPN %d with result:%d\r", i, ret);
			
			if (memcmp(buf, buf_sample, PAGE_SIZE)!=0)
			{
				error=0;
				printf("LPN %d : compare failure but id=%d\n",i,error,
				(*(unsigned long*)buf)==(*(unsigned long*)buf_sample));
				printf("Do you want to list errors?");
				if (getch()=='n')
					continue;
				for(j=0;j<1024;j++)
				{
					if (((unsigned long*)buf)[j] != ((unsigned long*)buf_sample)[j])
					{
						printf("%d: offset=%d orig: %08X read:%08X\n",error, j*4,((unsigned long*)buf_sample)[j],((unsigned long*)buf)[j]);
						error++;
					}

				}
				printf("Total error count=%d\n", error);

				getch();
			}
		}
		break;
	case 7:
		printf("Plz input the start address, e.g. 262144:");
		scanf("%d",&addr);
		printf("Len=");
		scanf("%d",&len);		
		printf("isRead=");
		scanf("%d",&op);
		for(i=addr;i<(addr+(len*16));i+=16)
		{
			if (op==1)
			{
				SeqFill(i, PAGE_SIZE, buf_sample);
				ret = ssd.ReadByLPN(i, 1, buf);		
				if (memcmp(buf, buf_sample, PAGE_SIZE)!=0)				
					printf("LPN %d offset=%d %d: compare failure\n",i,i%2048,(i%2048)/16);
			}
			else
			{
				SeqFill(i, PAGE_SIZE, buf);
				ret = ssd.WriteByLPN(i, 1, buf);			
				printf("Write data into LPN %d with result:%d\n", i, ret);				
			}
		}
		printf("The next spare addr=%d\n",i);
		break;
	case 9:
		return 0;
	}
	goto l_begin;

	return true;
}

int main_scp(int argc, char *argv[] )
{
	int i;
	unsigned int addr; 
	unsigned int len; 
	unsigned int dest;
	int s_offset=0;
	char* pNextNum;
	int seq;
	char comm[128];
	if (argc>1)
		strcpy(comm,argv[1]);
	if (argc>2)
	{
		addr = atoi(argv[2]);
		if (pNextNum=strchr(argv[2],'*'))
		{
			if (*pNextNum==0)
			{
				perror("wrong syntax");
				return false;
			}
			addr*=atoi(pNextNum+1);
		}
	}
	else
	{	
		printf("loss parameter: address\n");
		return false;
	}
	if (argc>3)
		len = atoi(argv[3]);
	else
	{
		printf("loss paremeter: length\n");
		return false;
	}	
	if (argc>4)
	{
		seq = atoi(argv[4]);
		dest = atoi(argv[4]);
		s_offset = atoi(argv[4]);
	}
	else
		seq = 1;
	
	
	printf("%s %u %u %d \n",comm, addr, len, seq);

	CVSSD ssd;
	int ret;
//	int cn_pages=64;
	unsigned char buf[PAGE_SIZE*MAX_PAGE] __attribute__ ((aligned (PAGE_SIZE)));
  //  srand48(time(0));			
  //  srand48(12345);
	int pDiskId=0;
	int pSeed=0;
	char* pEnvStr;
	
	pEnvStr=getenv("utest_seed48");
	if (pEnvStr)
	{
    	pSeed=atol(pEnvStr);
    	srand48(pSeed);
        printf("Get utest_seed48=%d\n",pSeed);
    }
    else
	{
//                srand48(time(0));	
                srand48(12345);
	}
	
	
	pEnvStr=getenv("utest_trgdisk");
	if (pEnvStr)
	{
    	pDiskId=atoi(pEnvStr);
        printf("Get env var utest_trgdisk=%d\n",pDiskId);
        if (ssd.Open(pDiskId)==false)
            ASSERT(0,"Fail to open disk\n");    	
    }
    else
	{
    	    
    	for(i=1;i<3;i++)
    	{
		    if(ssd.Open(i))
		    {
		        pDiskId=i;
                break;
           } 
        }		
        ASSERT(i<3,"Fail to open disk\n");
    }
	
	//printf("Disk Size: %llu GB\n", ssd.GetSize()>>21);
	ssd.InitParameter(NUM_SECTOR_IN_PAGE_ORDER,6);
	
	/* comment case*/
	if (strcmp(comm,"randswsr")==0)
	    ret=randsw_seqsr(ssd,s_offset,addr,len,pSeed,1,true);
	else if (strcmp(comm,"perfsw")==0)
	    ret=pf_4K_write(ssd,addr,len,false,1);
    else if (strcmp(comm,"perfsr")==0)
	    ret=pf_4K_read(ssd,addr,len,true);
	else if (strcmp(comm,"perfrandsw")==0)
	    ret=pf_4K_randwrite(ssd,s_offset,addr,len,false,1);
    else if (strcmp(comm,"randsw")==0)
        ret=pf_4K_randwrite(ssd,s_offset,addr,len,true,1);
        
    else if (strcmp(comm,"perfrandsr")==0)
	    ret=pf_4K_randread(ssd,s_offset,addr,len,false,1);	    
	else if (strcmp(comm,"randsr")==0)
	    ret=pf_4K_randread(ssd,s_offset,addr,len,true,1);	    	    
	else if (strcmp(comm,"perfw")==0)
        ret=pf_var_write(ssd,addr,len);
	else if (strcmp(comm,"r")==0)
	{
		ret=simple_read(ssd, addr, len, 0);
		//printf("result:%d",ret);
	}	
	else if (strcmp(comm,"w")==0)
		ret=simple_write(ssd, addr, len, seq);
	else if (strcmp(comm,"sr")==0)
	{
		if(addr<CN_SBLK)
		{
			ret=sim_LFSM_access_Ex(ssd, 0, CN_PAGES, addr, len);
			//printf("%d \n", ret);
		}	
		else
			printf("over the max number of sEU\n");
	}	
	else if (strcmp(comm,"sw")==0)
	{
		if(addr<CN_SBLK)
			ret=sim_LFSM_access_Ex(ssd, 1, CN_PAGES, addr, len);	
		else
			printf("over the max number of sEU\n");
	}	
	else if (strcmp(comm,"erase")==0)
		return bLFSM_erase(addr*CN_BLOCK_OF_SEU*CN_PAGES*8,len);
	else if (strcmp(comm,"eraselpn")==0)
		return bLFSM_erase(addr*8,len);		 
	else if (strcmp(comm,"markbad")==0)
		return bLFSM_markbad(addr*CN_BLOCK_OF_SEU,len);
	else if (strcmp(comm,"dynpagequery")==0)
		return bLFSM_query(addr,len,0);
	else if (strcmp(comm,"dynblkquery")==0)
		return bLFSM_query(addr,len,1);
	else if (strcmp(comm,"staticquery")==0)
		return bLFSM_query(-1,1,1);		
	else if (strcmp(comm,"copy")==0)
		return bLFSM_copy(addr*CN_BLOCK_OF_SEU*CN_PAGES*8,dest*CN_BLOCK_OF_SEU*CN_PAGES*8,len);
	else if (strcmp(comm,"copylpn")==0)
		return bLFSM_copy(addr*8,dest*8,len);
	else if (strcmp(comm, "mixrw" )==0)
		ret=sim_LFSM_access_mix(ssd, CN_PAGES, addr, len, s_offset);
	else if (strcmp(comm, "rwe" )==0){}
	else if (strcmp(comm, "cmp" ) ==0)
	{
		if (ssd.ReadByLPN(dest,len,buf)==false)
		{
			printf("ReadByLPN failure %d\n",addr);
			return false;
		}
#ifdef ALSSD2_H
                ssd.SetFunc(Check_data);
                ssd.WaitAllEventDone(0);
#endif
/*
        	for(i=0; i<len; i++)
        	{	
		        printf("Page %d\n",addr+i);
        		dumphex(buf+(i*PAGE_SIZE));
                }
*/		
		return Check_data(addr, buf, len);
	}
        else if (strcmp(comm, "wrconf" )==0)
                wrcf(ssd, buf, CN_PAGES, addr, len);
        else if (strcmp(comm, "wwconf" )==0)
                wwcf(ssd, buf, CN_PAGES, addr, len);
        else if (strcmp(comm, "wcconfS" )==0)
                wccf_sour(ssd, buf, CN_PAGES, addr, len);
        else if (strcmp(comm, "wcconfD" )==0)
                wccf_dest(ssd, buf, CN_PAGES, addr, len);
        else if (strcmp(comm, "weconf" )==0)
                wecf(ssd, buf, CN_PAGES, addr, len);
        else if (strcmp(comm, "crconfS" )==0)
                crcf_sour(ssd, buf, CN_PAGES, addr, len);
        else if (strcmp(comm, "crconfD" )==0)
                crcf_dest(ssd, buf, CN_PAGES, addr, len);
        else if (strcmp(comm, "cwconfS" )==0)
                cwcf_sour(ssd, buf, CN_PAGES, addr, len);
        else if (strcmp(comm, "cwconfD" )==0)
                cwcf_dest(ssd, buf, CN_PAGES, addr, len);
        else if (strcmp(comm, "ccconfS" )==0)
                cccf_sour(ssd, buf, CN_PAGES, addr, len);
        else if (strcmp(comm, "ccconfD" )==0)
                cccf_dest(ssd, buf, CN_PAGES, addr, len);
        else if (strcmp(comm, "ceconfS" )==0)
                cecf_sour(ssd, buf, CN_PAGES, addr, len);
        else if (strcmp(comm, "ceconfD" )==0)
                cecf_dest(ssd, buf, CN_PAGES, addr, len);        
        else if (strcmp(comm, "erconf" )==0)
                ercf(ssd, buf, CN_PAGES, addr, len);
        else if (strcmp(comm, "ewconf" )==0)
                ewcf(ssd, buf, CN_PAGES, addr, len);
        else if (strcmp(comm, "eeconf" )==0)
                eecf(ssd, buf, CN_PAGES, addr, len);        
        else if (strcmp(comm, "ecconfS" )==0)
                eccf_sour(ssd, buf, CN_PAGES, addr, len);        
        else if (strcmp(comm, "ecconfD" )==0)
                eccf_dest(ssd, buf, CN_PAGES, addr, len);        
        else if (strcmp(comm, "rrconfD" )==0)
                rrcf(ssd, buf, CN_PAGES, addr, len);
        else if (strcmp(comm, "rwconf" )==0)
                rwcf(ssd, buf, CN_PAGES, addr, len);
        else if (strcmp(comm, "reconf" )==0)
                recf(ssd, buf, CN_PAGES, addr, len);
        else if (strcmp(comm, "reconS" )==0)
                rccf_sour(ssd, buf, CN_PAGES, addr, len);                                                                                        
        else if (strcmp(comm, "rcconfD" )==0)
                rccf_dest(ssd, buf, CN_PAGES, addr, len);



	else
	{
		printf("unknown command %s .\n",comm);
		return false;	
	}

	return ret;
}

int main(int argc,char *argv[])
{
	int ret;
/*	
        char* penv_CE_NUM;
        char* penv_EVB;
        penv_CE_NUM = getenv("CE_NUM");
        penv_EVB = getenv("TF_EVB_VERSION");
        
        if ((int)penv_CE_NUM==1 && (int)penv_EVB==1)
        {
                CN_BLOCK_OF_SEU=16;
                CN_PAGES=64;
        }    
        else if ((int)penv_CE_NUM==2 && (int)penv_EVB==1)
        {
                CN_BLOCK_OF_SEU=32;
                CN_PAGES=64;        
        }
        else if ((int)penv_CE_NUM==1 && (int)penv_EVB==0)
        {
                CN_BLOCK_OF_SEU=16;
                CN_PAGES=128;        
        }

        printf("CE_NUM=%s\n",penv_CE_NUM);
        printf("EVB_VERSION=%s\n",penv_EVB);
        ASSERT((penv_CE_NUM>0)&&(penv_EVB>0),"ERROR DEFINE VENDOR and CE\n");        
 */       	
        #if CN_PAGES == 0 || CN_BLOCK_OF_SEU == 0
                #error "error define\n"
        #else
                printf("cn_pages=%d cn_blocks=%d\n",CN_PAGES,CN_BLOCK_OF_SEU);
        #endif 

	if (argc>1)
	{
		ret = main_scp(argc, argv);
		//printf("%d\n",ret);
		if (ret==0)
			return 0;
		else if (ret==1)
			return 1;
		else if (ret==2)
			return 2;
		else if (ret==3)
			return 3;
			
	}
	else
		main_interactive();		

	return 1;
}

