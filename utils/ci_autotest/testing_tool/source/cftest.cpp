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
#include "../include/ssdconf.h"
#ifdef WIN32
#include "../include/vssd.h"
#include <conio.h>
#else
#include "../include/alssd2.h"
#include "../include/cftest.h"
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <time.h>
#include "../include/bfun.h"

#define SEUPAGE 1024
//#define CN_BLOCK_OF_SEU 16

void wrcf(CVSSD& ssd, unsigned char* pBuf, int cn_pages, int cn_used, int round)
{
	int i;
	int addr;
	int ret;
	
	//cn_used=rand()%2046;
	cn_used=10;
	printf("SEU= %d",cn_used);	
	round=1;        //len=16
	
	for(i=0;i<round;i++)
	{
		addr=cn_used*SEUPAGE+i;
		SeqFill(addr,PAGE_SIZE,pBuf);
		if(ssd.WriteByLPN(addr,1,pBuf)==false)
			printf("WriteByLPN failure %ul\n");
		else
			printf("Write LPN %d done\n", addr);
	}
	
#ifdef ALSSD2_H		
		ssd.SetFunc(Check_data);
#endif
	for(i=0;i<round;i++)
	{
#ifdef ALSSD2_H	
			pBuf = ssd.Cbuf.Allocate(1);
#endif
		addr=cn_used*SEUPAGE+i;
		if(ssd.ReadByLPN(addr,1,pBuf)==false)
			printf("ReadByLPN failure %d\n",addr);
		else
			printf("Read LPN %d done\n", addr);
/*
		ret = Check_data(addr,pBuf,1);
		if(ret==0)
			printf("LPN %d off=%d: compare failure\n", addr, addr%SEUPAGE);		
*/
	}

#ifdef ALSSD2_H	
	/* check all event are finished */
	do
	{
		ret=ssd.WaitEvents(1);
		printf("show cn_outcommand %d \n",ret);
	}while (ret>0);
#endif
}

void wwcf(CVSSD& ssd, unsigned char* pBuf, int cn_pages, int cn_used, int round)
{
	int j;
	int addr;
	int ret;
			
	round=1;
	//cn_used=rand()%2046;
	cn_used=20;
	printf("SEU= %d",cn_used);

#ifdef ALSSD2_H		
		ssd.SetFunc(Check_data);
#endif		
	for(j=0;j<round;j++)
	{
#ifdef ALSSD2_H	
			pBuf = ssd.Cbuf.Allocate(1);
#endif
		addr=cn_used*SEUPAGE+j;
		SeqFill(addr,PAGE_SIZE,pBuf);
		if(ssd.WriteByLPN(addr,1,pBuf)==false)
			printf("WriteByLPN failure %ul\n");
		else
			printf("Write LPN %d done\n", addr);
	}
							
#ifdef ALSSD2_H		
		ssd.SetFunc(Check_data);
#endif		
	for(j=0;j<round;j++)
	{
#ifdef ALSSD2_H	
			pBuf = ssd.Cbuf.Allocate(1);
#endif	
		addr=cn_used*SEUPAGE+j;
		RandomFill(addr,PAGE_SIZE,pBuf);
		if(ssd.WriteByLPN(addr,1,pBuf)==false)
			printf("WriteByLPN failure %ul\n");
		else
			printf("Write LPN %d done\n", addr);
	}	

#ifdef ALSSD2_H	
	/* check all event are finished */
	do
	{
		ret=ssd.WaitEvents(1);
		printf("show cn_outcommand %d \n",ret);
	}while (ret>0);
#endif


}


void wccf_sour(CVSSD& ssd, unsigned char* pBuf, int cn_pages, int cn_used, int round)
{
	int source;
	int dest;
	int i;
	int addr;

	round=1;	
	//source=rand()%2046;
	source=30;
	printf("source SEU= %d\n",source);	
	dest=source+2;	

#ifdef ALSSD2_H		
		ssd.SetFunc(Check_data);
#endif		
	
	for(i=0;i<round;i++)
	{
#ifdef ALSSD2_H	
			pBuf = ssd.Cbuf.Allocate(1);
#endif
		addr=source*SEUPAGE+i;
		SeqFill(addr,PAGE_SIZE,pBuf);
		if(ssd.WriteByLPN(addr,1,pBuf)==false)
			printf("WriteByLPN failure %ul\n");
		else
			printf("Write LPN %d done\n", addr);
	}
		
	bLFSM_copy(source*CN_BLOCK_OF_SEU*cn_pages*8, dest*CN_BLOCK_OF_SEU*cn_pages*8, round);
}


void wccf_dest(CVSSD& ssd, unsigned char* pBuf, int cn_pages, int cn_used, int round)
{
	int source;
	int dest;
	int i;
	int addr;

	round=1;
	//source=rand()%2046;
	source=40;
	dest=source+2;
	printf("dest SEU= %d\n",dest);
			
#ifdef ALSSD2_H		
		ssd.SetFunc(Check_data);
#endif
	for(i=0;i<round;i++)
	{
#ifdef ALSSD2_H	
			pBuf = ssd.Cbuf.Allocate(1);
#endif
		addr=dest*SEUPAGE+i;
		SeqFill(addr,PAGE_SIZE,pBuf);
		if(ssd.WriteByLPN(addr,1,pBuf)==false)
			printf("WriteByLPN failure %ul\n");
		else
			printf("Write LPN %d done\n", addr);
	}	

	bLFSM_copy(source*CN_BLOCK_OF_SEU*cn_pages*8, dest*CN_BLOCK_OF_SEU*cn_pages*8, round);

}

void wecf(CVSSD& ssd, unsigned char* pBuf, int cn_pages, int cn_used, int round)
{
	int i;
	int addr;
	
	round=1;
	//cn_used=rand()%2046;
	cn_used=50;
	printf("SEU= %d\n",cn_used);	

#ifdef ALSSD2_H		
		ssd.SetFunc(Check_data);
#endif
		
	for(i=0;i<round;i++)
	{
#ifdef ALSSD2_H	
			pBuf = ssd.Cbuf.Allocate(1);
#endif
		addr=cn_used*SEUPAGE+i;
		SeqFill(addr,PAGE_SIZE,pBuf);
		if(ssd.WriteByLPN(addr,1,pBuf)==false)
			printf("WriteByLPN failure %ul\n");
		else
			printf("Write LPN %d done\n", addr);
	}

	bLFSM_erase(cn_used*CN_BLOCK_OF_SEU*cn_pages*8, round);
}

void crcf_sour(CVSSD& ssd, unsigned char* pBuf, int cn_pages, int cn_used, int round)
{
	int source;
	int dest;
	int i;
	int ret;
	int addr;

	round=1;
	//source=rand()%2046;
	source=60;
	printf("source SEU= %d\n",source);
	dest=source+2;	

	bLFSM_copy(source*CN_BLOCK_OF_SEU*cn_pages*8, dest*CN_BLOCK_OF_SEU*cn_pages*8,round);	
/*	
	for(i=0;i<round;i++)
	{
		addr=source*SEUPAGE+i;
		if(ssd.ReadByLPN(addr,1,pBuf)==false)
			printf("ReadByLPN failure %d\n",addr);
		else
			printf("Read LPN %d done\n", addr);

		ret = Check_data(addr,pBuf,1);
		if(ret==0)
			printf("LPN %d off=%d: compare failure\n", addr, addr%SEUPAGE);		

	}
*/

#ifdef ALSSD2_H	
		ssd.SetFunc(Check_data);
		for(i=0;i<round;i++)
		{
			pBuf = ssd.Cbuf.Allocate(1);
			addr = source*SEUPAGE+i;			
			if (ssd.ReadByLPN(addr,1,pBuf)==false)
			{
				printf("ReadByLPN failure %d\n",addr);
			}
		}
	/* check all event are finished */
	do
	{
		ret=ssd.WaitEvents(1);
		printf("show cn_outcommand %d \n",ret);

	}while (ret>0);
#endif	


	
}

void crcf_dest(CVSSD& ssd, unsigned char* pBuf, int cn_pages, int cn_used, int round)
{
	int source;
	int dest;
	int i;
	int ret;
	int addr;

	round=1;	
	//source=rand()%2046;
	source=70;
	dest=source+2;
	printf("dest SEU= %d\n",dest);
		
	bLFSM_copy(source*CN_BLOCK_OF_SEU*cn_pages*8, dest*CN_BLOCK_OF_SEU*cn_pages*8, round);

#ifdef ALSSD2_H		
		ssd.SetFunc(Check_data);
#endif
		
	for(i=0;i<round;i++)
	{
#ifdef ALSSD2_H	
			pBuf = ssd.Cbuf.Allocate(1);
#endif
		addr=dest*SEUPAGE+i;
		if(ssd.ReadByLPN(addr,1,pBuf)==false)
			printf("ReadByLPN failure %d\n",addr);
		else
			printf("Read LPN %d done\n", addr);
		ret = Check_data(addr,pBuf,1);
		printf("ret = %d\n",ret);
		if(ret==0)
			printf("LPN %d off=%d: compare failure\n", addr, addr%SEUPAGE);		
	}
}

void cwcf_sour(CVSSD& ssd, unsigned char* pBuf, int cn_pages, int cn_used, int round)
{
	int i;
	int source;
	int dest;
	int addr;

	round=1;
	//source=rand()%2046;
	source=80;
	printf("source SEU= %d\n",source);
	dest=source+2;	
	
	bLFSM_copy(source*CN_BLOCK_OF_SEU*cn_pages*8, dest*CN_BLOCK_OF_SEU*cn_pages*8, round);

#ifdef ALSSD2_H		
		ssd.SetFunc(Check_data);
#endif
		
	for(i=0;i<round;i++)
	{
#ifdef ALSSD2_H	
			pBuf = ssd.Cbuf.Allocate(1);
#endif
		addr=source*SEUPAGE+i;
		SeqFill(addr,PAGE_SIZE,pBuf);
		if(ssd.WriteByLPN(addr,1,pBuf)==false)
			printf("WriteByLPN failure %ul\n");
		else
			printf("Write LPN %d done\n", addr);
	}
}

void cwcf_dest(CVSSD& ssd, unsigned char* pBuf, int cn_pages, int cn_used, int round)
{
	int i;
	int source;
	int dest;
	int addr;

	round=1;
	//source=rand()%2046;
	source=90;
	dest=source+2;
	printf("dest SEU= %d\n",dest);

	bLFSM_copy(source*CN_BLOCK_OF_SEU*cn_pages*8, dest*CN_BLOCK_OF_SEU*cn_pages*8, round);

#ifdef ALSSD2_H		
		ssd.SetFunc(Check_data);
#endif
		
	for(i=0;i<round;i++)
	{
#ifdef ALSSD2_H	
			pBuf = ssd.Cbuf.Allocate(1);
#endif
		addr=dest*SEUPAGE+i;
		SeqFill(addr,1,pBuf);
		if(ssd.WriteByLPN(addr,PAGE_SIZE,pBuf)==false)
			printf("WriteByLPN failure %ul\n");
		else
			printf("Write LPN %d done\n", addr);
	}

}

void cccf_sour(CVSSD& ssd, unsigned char* pBuf, int cn_pages, int cn_used, int round)
{
	int source;
	int dest[2];
	round=1;

	source=100;
	dest[0]=source+2;
	dest[1]=source+4;
	printf("Copy source SEU= %d to dest SEU= %d , %d\n", source, dest[0], dest[1]);
	
	bLFSM_copy(source*CN_BLOCK_OF_SEU*cn_pages*8, dest[0]*CN_BLOCK_OF_SEU*cn_pages*8, round);
	bLFSM_copy(source*CN_BLOCK_OF_SEU*cn_pages*8, dest[1]*CN_BLOCK_OF_SEU*cn_pages*8, round);
}

void cccf_dest(CVSSD& ssd, unsigned char* pBuf, int cn_pages, int cn_used, int round)
{
	int source[2];
	int dest;
	round=1;

	dest=120;
	source[0]=dest+2;
	source[1]=dest+4;	
	printf("Copy source SEU= %d, %d to dest SEU= %d\n", source[0], source[1], dest);

	bLFSM_copy(source[0]*CN_BLOCK_OF_SEU*cn_pages*8, dest*CN_BLOCK_OF_SEU*cn_pages*8, round);
	bLFSM_copy(source[1]*CN_BLOCK_OF_SEU*cn_pages*8, dest*CN_BLOCK_OF_SEU*cn_pages*8, round);
}


void cecf_sour(CVSSD& ssd, unsigned char* pBuf, int cn_pages, int cn_used, int round)
{
	int source;
	int dest;
	round=1;
	
	source=200;
	dest=source+2;
	printf("source SEU= %d\n",source);

	bLFSM_copy(source*CN_BLOCK_OF_SEU*cn_pages*8, dest*CN_BLOCK_OF_SEU*cn_pages*8, round);
	bLFSM_erase(source*CN_BLOCK_OF_SEU*cn_pages*8, round);
}

void cecf_dest(CVSSD& ssd, unsigned char* pBuf, int cn_pages, int cn_used, int round)
{
	int source;
	int dest;
	round=16;
	
	source=220;
	dest=source+2;
	printf("dest SEU= %d\n",dest);

	bLFSM_copy(source*CN_BLOCK_OF_SEU*cn_pages*8, dest*CN_BLOCK_OF_SEU*cn_pages*8, round);
	bLFSM_erase(dest*CN_BLOCK_OF_SEU*cn_pages*8, round);
}


void ercf(CVSSD& ssd, unsigned char* pBuf, int cn_pages, int cn_used, int round)
{
	int i;
	int ret;
	int addr; 
		
	round=1;
	cn_used=240;
	printf("SEU= %d\n",cn_used);
	
	bLFSM_erase(cn_used*CN_BLOCK_OF_SEU*cn_pages*8, round);

#ifdef ALSSD2_H		
		ssd.SetFunc(Check_data);
#endif
		
	for(i=0;i<round;i++)
	{
#ifdef ALSSD2_H	
			pBuf = ssd.Cbuf.Allocate(1);
#endif
		addr=cn_used*SEUPAGE+i;
		if(ssd.ReadByLPN(addr,1,pBuf)==false)
			printf("ReadByLPN failure %d\n",addr);
		else
			printf("Read LPN %d done\n", addr);
/*
		ret = Check_data(addr,pBuf,1);
		printf("ret = %d\n",ret);
		if(ret==0)
			printf("LPN %d off=%d: compare failure\n", addr, addr%SEUPAGE);		
*/
	}
}	

void ewcf(CVSSD& ssd, unsigned char* pBuf, int cn_pages, int cn_used, int round)
{
	int i;
	int addr;
	int ret;
		
	round=1;
	cn_used=260;
	printf("SEU= %d\n", cn_used);
	
	bLFSM_erase(cn_used*CN_BLOCK_OF_SEU*cn_pages*8, round);
	for(i=0;i<round;i++)
	{
		addr=cn_used*SEUPAGE+i;
		ret = Check_data(addr,pBuf,1);
		printf("ret = %d\n",ret);
	}

#ifdef ALSSD2_H		
		ssd.SetFunc(Check_data);
#endif
		
	for(i=0;i<round;i++)
	{
#ifdef ALSSD2_H	
			pBuf = ssd.Cbuf.Allocate(1);
#endif	
		addr=cn_used*SEUPAGE+i;
		SeqFill(addr,1,pBuf);
		if(ssd.WriteByLPN(addr,PAGE_SIZE,pBuf)==false)
			printf("WriteByLPN failure %ul\n");
		else
			printf("Write LPN %d done\n", addr);
/*
		ret = Check_data(addr,pBuf,1);
		printf("w_ret = %d\n",ret);
*/
	}


}

void eecf(CVSSD& ssd, unsigned char* pBuf, int cn_pages, int cn_used, int round)
{
	int i,j,ret;
	int addr;
	
	cn_used=280;
	round=1;
	printf("SEU= %d\n",cn_used);
	
	for(i=0;i<2;i++)
	{
		bLFSM_erase(cn_used*CN_BLOCK_OF_SEU*cn_pages*8, round);
	}	
#ifdef ALSSD2_H		
		ssd.SetFunc(Check_data);
		for(j=0;j<round;j++)
		{
			pBuf = ssd.Cbuf.Allocate(1);
			addr = cn_used*SEUPAGE+j;			
			if (ssd.ReadByLPN(addr,1,pBuf)==false)
			{
				printf("ReadByLPN failure %d\n",addr);
			}
		}
	/* check all event are finished */
	do
	{
		ret=ssd.WaitEvents(1);
		printf("show cn_outcommand %d \n",ret);

	}while (ret>0);
#endif	
	
}

void eccf_sour(CVSSD& ssd, unsigned char* pBuf, int cn_pages, int cn_used, int round)
{
	int source;
	int dest;
	int i;
	int addr;
	int ret;
	
	//source=rand()%2046;
	source=300;
	dest=source+2;
	printf("source SEU= %d\n",source);
	round=1;

	bLFSM_erase(source*CN_BLOCK_OF_SEU*cn_pages*8,round);
	
	bLFSM_copy(source*CN_BLOCK_OF_SEU*cn_pages*8, dest*CN_BLOCK_OF_SEU*cn_pages*8, round);	

#ifdef ALSSD2_H	
		ssd.SetFunc(Check_data);
		for(i=0;i<round;i++)
		{
			pBuf = ssd.Cbuf.Allocate(1);
			addr = source*SEUPAGE+i;			
			if (ssd.ReadByLPN(addr,1,pBuf)==false)
			{
				printf("ReadByLPN failure %d\n",addr);
			}
		}
	/* check all event are finished */
	do
	{
		ret=ssd.WaitEvents(1);
		printf("show cn_outcommand %d \n",ret);

	}while (ret>0);
#endif	

}

void eccf_dest(CVSSD& ssd, unsigned char* pBuf, int cn_pages, int cn_used, int round)
{
	int source;
	int dest;
	
	//source=rand()%2046;
	source=320;
	dest=source+2;
	printf("source SEU= %d\n",source);
	round=1;

	bLFSM_erase(dest*CN_BLOCK_OF_SEU*cn_pages*8,round);
		
	bLFSM_copy(source*CN_BLOCK_OF_SEU*cn_pages*8, dest*CN_BLOCK_OF_SEU*cn_pages*8, round);
}

void rrcf(CVSSD& ssd, unsigned char* pBuf, int cn_pages, int cn_used, int round)
{
	int i,j;
	int ret;
	int addr;
	
	//cn_used=rand()%2046;
	cn_used=340;
	printf("SEU= %d\n", cn_used);
	round=1;

	for(j=0;j<2;j++)
	{
#ifdef ALSSD2_H		
		ssd.SetFunc(Check_data);
#endif
		for(i=0;i<round;i++)
		{
#ifdef ALSSD2_H
			pBuf = ssd.Cbuf.Allocate(1);
#endif
			addr=cn_used*SEUPAGE+i;
			if(ssd.ReadByLPN(addr,1,pBuf)==false)
				printf("ReadByLPN failure %d\n",addr);
			else
				printf("Read LPN %d done\n",addr);
//			ret = Check_data(addr,pBuf,1);
//			printf("ret = %d\n",ret);
//			if(ret==0)
//				printf("LPN %d off=%d: compare failure\n", addr, addr%SEUPAGE);		
		}
	}

	/* check all event are finished */
#ifdef ALSSD2_H		
	do
	{
		ret=ssd.WaitEvents(1);
		printf("show cn_outcommand %d \n",ret);

	}while (ret>0);
#endif
	
}

void rwcf(CVSSD& ssd, unsigned char* pBuf, int cn_pages, int cn_used, int round)
{
	int addr;
	int ret;
	int i;
		
	//cn_used=rand()%2046;
	cn_used=360;
	printf("SEU= %d\n", cn_used);
	round=1;

#ifdef ALSSD2_H		
		ssd.SetFunc(Check_data);
#endif
		
	for(i=0;i<round;i++)
	{
#ifdef ALSSD2_H	
			pBuf = ssd.Cbuf.Allocate(1);
#endif	
		addr=cn_used*SEUPAGE+i;
		if(ssd.ReadByLPN(addr,1,pBuf)==false)
					printf("ReadByLPN failure %d\n",addr);
		else
			printf("Read LPN %d done\n", addr);
		ret = Check_data(addr,pBuf,1);
		printf("ret = %d\n",ret);
		if(ret==0)
			printf("LPN %d off=%d: compare failure\n", addr, addr%SEUPAGE);	
	}

#ifdef ALSSD2_H		
		ssd.SetFunc(Check_data);
#endif
		
	for(i=0;i<round;i++)
	{
#ifdef ALSSD2_H	
			pBuf = ssd.Cbuf.Allocate(1);
#endif	
		addr=cn_used*SEUPAGE+i;
		SeqFill(addr ,PAGE_SIZE,pBuf);
		if(ssd.WriteByLPN(addr,1,pBuf)==false)
			printf("WriteByLPN failure %ul\n");
		else
			printf("Write LPN %d done\n", addr);
	}	
}



void recf(CVSSD& ssd, unsigned char* pBuf, int cn_pages, int cn_used, int round)
{
	int i;
	int addr;
	int ret;
		
	//cn_used=rand()%2046;
	cn_used=370;
	printf("SEU= %d\n", cn_used);
	round=1;

#ifdef ALSSD2_H		
		ssd.SetFunc(Check_data);
#endif
		
	for(i=0;i<round;i++)
	{
#ifdef ALSSD2_H	
			pBuf = ssd.Cbuf.Allocate(1);
#endif	
		addr=cn_used*SEUPAGE+i;
		if(ssd.ReadByLPN(addr,1,pBuf)==false)
					printf("ReadByLPN failure %d\n",addr);
		else
			printf("Read LPN %d done\n", addr);
		ret = Check_data(addr,pBuf,1);
		printf("ret = %d\n",ret);
		if(ret==0)
			printf("LPN %d off=%d: compare failure\n", addr, addr%SEUPAGE);	
	}
	
	bLFSM_erase(cn_used*CN_BLOCK_OF_SEU*cn_pages*8, round);
}

void rccf_sour(CVSSD& ssd, unsigned char* pBuf, int cn_pages, int cn_used, int round)
{
	int i;
	int source;
	int dest;
	int addr;
	int ret;
	
	round=1;
	//source=rand()%2046;
	source=380;
	printf("source SEU= %d\n",source);
	dest=source+2;	

#ifdef ALSSD2_H		
		ssd.SetFunc(Check_data);
#endif
		
	for(i=0;i<round;i++)
	{
#ifdef ALSSD2_H	
			pBuf = ssd.Cbuf.Allocate(1);
#endif	
		addr=source*SEUPAGE+i;
		if(ssd.ReadByLPN(addr,1,pBuf)==false)
					printf("ReadByLPN failure %d\n",addr);
		else
			printf("Read LPN %d done\n", addr);
		ret = Check_data(addr,pBuf,1);
		printf("ret = %d\n",ret);
		if(ret==0)
			printf("LPN %d off=%d: compare failure\n", addr, addr%SEUPAGE);	
	}
	
	bLFSM_copy(source*CN_BLOCK_OF_SEU*cn_pages*8, dest*CN_BLOCK_OF_SEU*cn_pages*8, round);
}

void rccf_dest(CVSSD& ssd, unsigned char* pBuf, int cn_pages, int cn_used, int round)
{
	int i;
	int source;
	int dest;
	int addr;
	int ret;
	
	round=1;
	//source=rand()%2046;
	source=400;
	dest=source+2;
	printf("dest SEU= %d\n", dest);	

#ifdef ALSSD2_H		
		ssd.SetFunc(Check_data);
#endif
		
	for(i=0;i<round;i++)
	{
#ifdef ALSSD2_H	
			pBuf = ssd.Cbuf.Allocate(1);
#endif	
		addr=source*SEUPAGE+i;
		if(ssd.ReadByLPN(addr,1,pBuf)==false)
					printf("ReadByLPN failure %d\n",addr);
		else
			printf("Read LPN %d done\n", addr);
		ret = Check_data(addr,pBuf,1);
		printf("ret = %d\n",ret);
		if(ret==0)
			printf("LPN %d off=%d: compare failure\n", addr, addr%SEUPAGE);	
	}
	
	bLFSM_copy(source*CN_BLOCK_OF_SEU*cn_pages*8, dest*CN_BLOCK_OF_SEU*cn_pages*8, round);
}
