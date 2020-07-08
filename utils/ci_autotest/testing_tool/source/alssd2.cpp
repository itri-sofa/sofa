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
#include <errno.h>
#include <libaio.h>

#include "../include/ssdconf.h"
#include "../include/alssd2.h"
#include "../include/bfun.h"
#include "../include/mem512.h"
#include "../perftest.h"
CVSSD::CVSSD(void)
{
    pAdm=0;
    fp_chkdata=0;
    cn_send=0;
}


CVSSD::~CVSSD(void)
{
}

void CVSSD::SetFunc(CallBack pFun)
{
    fp_chkdata=pFun;
}

void CVSSD::SetADM(AddrManer* _pAdm)
{
    pAdm=_pAdm;
}

bool CVSSD::InitParameter(int m_l2no_sector_in_page, int m_l2no_page_in_block)
{
    Errfp=fopen("rat.log","w");
    if(Errfp==NULL)	
        printf("open file fail\n");
//    else 
 //       printf("open file ok\n");

    cn_totalevents=0;
    l2no_sector_in_page = m_l2no_sector_in_page;
    l2no_page_in_block = m_l2no_page_in_block;
    l2no_byte_in_sector = 9;
    lastaddr=99999;
    return true;
}
void CVSSD::SetDesc(char* name)
{
    strcpy(DriverDesc,name);
}
bool CVSSD::Open(int DriverID)
{
    unsigned int AccessMode;

    nr_events = MAX_PENDING_REQS;
    memset(&ctx, 0, sizeof(ctx));  // It's necessary
    int errcode = io_setup(nr_events, &ctx);
//    if (errcode == 0)
//         perror("io_setup");
	if (DriverID==-1)
	    {}
	else if (DriverID==99)
		sprintf(DriverDesc,"/dev/lfsm");
	else if (DriverID==98)
		sprintf(DriverDesc,"/dev/null");		
        else if (DriverID==97)
            sprintf(DriverDesc,"/dev/ram0");
	else
		sprintf(DriverDesc,"/dev/sd%c",DriverID+'a');
	printf("Open disk %s\n",DriverDesc);
	hDevice = open(DriverDesc, O_RDWR|O_NOATIME|O_DIRECT, S_IRUSR| S_IWUSR);
	//hDevice = open(DriverDesc, O_RDWR|O_LARGEFILE, S_IRUSR| S_IWUSR);
	
	if (hDevice==INVALID_HANDLE_VALUE)
		return false;
	
	cn_sector = GetSizeA()>>9;
    maxdepth=MAX_PENDING_REQS/3;
    cn_outcommand=0;	
    return true;
}

bool CVSSD::Close(bool isForce)
{
	if (isForce==false)
	    WaitAllEventDone(2);
    close(hDevice);    
    io_destroy(ctx);	
    hDevice=INVALID_HANDLE_VALUE;
    fclose(Errfp);
    return true;
}

bool CVSSD::WaitAllEventDone(int isW)
{
	int cn_done=0;
    while(cn_outcommand>0)
    {
        cn_done = WaitEvents(cn_outcommand);
        if (cn_done==-1)
        {
        	printf("%d cmds pending but get no resp\n",cn_outcommand);
        	return false;
		}
        cn_outcommand-=cn_done;
    }        
    return true;
}

bool CVSSD::ReadByLPN(INT64 lpn, INT64 len, unsigned char* pBuf)
{
	INT64 ret;
	int cn_done;
	unsigned long m_nBytes=(UINT32)(len<<(l2no_sector_in_page + l2no_byte_in_sector));
	INT64 Offset = lpn << (l2no_sector_in_page + l2no_byte_in_sector);
    struct iocb *iocbpp = (struct iocb *)malloc(sizeof(struct iocb));
    if(cn_outcommand>=maxdepth)
    {
        do{
        	cn_done=WaitEvents(1);
        	if (cn_done==-1)
	        {
    	    	printf("%d cmds pending but get no resp\n",cn_outcommand);
//                ASSERT(0,"cmds pending but get no resp\n");
        		return false;
			}        	
		    cn_outcommand-=cn_done;
//	    printf("Wait cmds: %02d \n",cn_outcommand);
//        }while(cn_outcommand>=maxdepth);
		}while(cn_outcommand>0);
    }    
    memset(iocbpp, 0, sizeof(struct iocb));

    iocbpp->aio_lio_opcode = IO_CMD_PREAD;
    iocbpp->aio_reqprio    = 0;
    iocbpp->aio_fildes     = hDevice;
    iocbpp->u.c.buf    = pBuf;
    iocbpp->u.c.nbytes = m_nBytes;//strlen(buf);
    iocbpp->u.c.offset = Offset; 
    ASSERT(ctx!=(io_context_t)0xFFFFFFFF,"invalid ctx\n");    
    int n = io_submit(ctx, 1, &iocbpp);
    cn_outcommand++;
    
    return true;    
}

bool CVSSD::ReadByPPN(unsigned long lbn, unsigned long id_page, unsigned long len, unsigned char* pBuf)
{
	printf("ReadByPPN Unsupport\n");
	return false;
}
bool CVSSD::BatchWriteByLPN(INT64* ar_lpn, INT64 len, unsigned char* pBuf)
{
	return BatchAccessByLPN(1,ar_lpn,len,pBuf);
}
bool CVSSD::BatchReadByLPN(INT64* ar_lpn, INT64 len, unsigned char* pBuf)
{
	return BatchAccessByLPN(0,ar_lpn,len,pBuf);
}
bool CVSSD::BatchAccessByLPN(int op, INT64* ar_lpn, INT64 len, unsigned char* pBuf)
{
    int k;
    int cn_done;
    INT64 ret;
    unsigned long m_nBytes=(UINT32)(len<<(l2no_sector_in_page + l2no_byte_in_sector));
    INT64 Offset;
    struct iocb* ar_iocb[BATCH_WRITE_DEPTH];
    struct iocb* piocb;

    for(k=0;k<BATCH_WRITE_DEPTH;k++)
    {
        ar_iocb[k] = (struct iocb *)malloc(sizeof(struct iocb));
        memset(ar_iocb[k], 0, sizeof(struct iocb));

        Offset = (ar_lpn[k]) << (l2no_sector_in_page + l2no_byte_in_sector);

        ar_iocb[k]->aio_lio_opcode = (op==1)?IO_CMD_PWRITE:IO_CMD_PREAD;
        ar_iocb[k]->aio_reqprio    = 0;
        ar_iocb[k]->aio_fildes     = hDevice;
        ar_iocb[k]->u.c.buf    = pBuf+ (k<< (l2no_sector_in_page + l2no_byte_in_sector));
        ar_iocb[k]->u.c.nbytes = m_nBytes;//strlen(buf);
        ar_iocb[k]->u.c.offset = Offset;
//        printf("addr=%lld %lld buf=%p len=%d\n",ar_lpn[k],Offset,ar_iocb[k]->u.c.buf,m_nBytes);                
    }
    ASSERT(ctx!=(io_context_t)0xFFFFFFFF,"invalid ctx\n");
    if(cn_outcommand>=(maxdepth-BATCH_WRITE_DEPTH))
    {
        do{
        	cn_done = WaitEvents(BATCH_WRITE_DEPTH);
			if (cn_done==-1)
	        {
    	    	printf("%d cmds pending but get no resp\n",cn_outcommand);
        		return false;
			}        				        	
            cn_outcommand-=cn_done;
        }while(cn_outcommand>=(maxdepth-BATCH_WRITE_DEPTH));    
    }        
    int n = io_submit(ctx, BATCH_WRITE_DEPTH, ar_iocb);
    lastaddr=Offset;
    cn_outcommand+=BATCH_WRITE_DEPTH;
    return true;    
    
}
bool CVSSD::WriteByLPN(INT64 lpn, INT64 len, unsigned char* pBuf)
{
	int cn_done;
    INT64 ret;
    unsigned long m_nBytes=(UINT32)(len<<(l2no_sector_in_page + l2no_byte_in_sector));
    INT64 Offset = lpn << (l2no_sector_in_page + l2no_byte_in_sector);
    //printf("m_nBtye: %lu offset: %lld \n",m_nBytes, Offset);
    if(cn_outcommand>=maxdepth)
    {
        do{
	       	cn_done = WaitEvents(1);
			if (cn_done==-1)
	        {
   		    	printf("%d cmds pending but get no resp\n",cn_outcommand);
   		    	exit(1);
       			return false;
			}        				        	
    	    cn_outcommand-=cn_done;    
        }while(cn_outcommand>=maxdepth);    
//        }while(cn_outcommand>0);
    }
    struct iocb *iocbpp = (struct iocb *)malloc(sizeof(struct iocb));
    memset(iocbpp, 0, sizeof(*iocbpp));

    iocbpp->aio_lio_opcode = IO_CMD_PWRITE;
    iocbpp->aio_reqprio    = 0;
    iocbpp->aio_fildes     = hDevice;

    iocbpp->u.c.buf    = pBuf;
    iocbpp->u.c.nbytes = m_nBytes;//strlen(buf);
    iocbpp->u.c.offset = Offset; 

    if ((m_nBytes%PAGE_SIZE!=0)||(Offset%PAGE_SIZE!=0))
    {
        printf("partial write cmd %llu %lu\n",Offset,m_nBytes);
        exit(1);
        return false;
    }
    //printf("len: %d \n",len);
    //printf("out size %lu \n in size %lu \n", iocbpp->u.c.nbytes, m_nBytes);
    cn_send++;
    //printf("Write LPN=%llu count=%d\n",lpn,cn_send);
    ASSERT(ctx!=(io_context_t)0xFFFFFFFF,"invalid ctx\n");
    int n = io_submit(ctx, 1, &iocbpp);
    if (n!=1)
    {
        perror("io_submit failure:");
        exit(1);
    }
    
//    if (lastaddr==Offset)
//        printf("lastaddr==offset\n");
//    ASSERT(lastaddr!=Offset,"lastaddr!=Offset\n");
    lastaddr=Offset;
//    sleep(1);
    cn_outcommand++;
    return true;
}
/*

bool CVSSD:Sync(void)
{
	struct iocb *iocbpp = (struct iocb *)malloc(sizeof(struct iocb));
    memset(iocbpp, 0, sizeof(struct iocb));

    iocbpp[0].data           = pBuf;
    iocbpp[0].aio_lio_opcode = IO_CMD_PWRITE;
    iocbpp[0].aio_reqprio    = 0;
    iocbpp[0].aio_fildes     = hDevice;

    iocbpp[0].u.c.buf    = pBuf;
    iocbpp[0].u.c.nbytes = m_nBytes;//strlen(buf); 
    iocbpp[0].u.c.offset = Offset;

    if ((m_nBytes%PAGE_SIZE!=0)||(Offset%PAGE_SIZE!=0))
    {
        printf("partial write cmd %llu %lu\n",Offset,m_nBytes);
        exit(1);
        return false;
    }
    int n = io_submit(ctx, 1, &iocbpp);
}
*/
/*
int CVSSD::WaitSpecificEvent(struct iocb* iocbpp)
{
    int i;
    struct io_event events[MAX_PENDING_REQS];
    struct timespec timeout = {1, 100};
    int cn_events;
    cn_events = io_getevents(ctx, 1, MAX_PENDING_REQS, events, &timeout);
    for(i=0;i<cn_events;i++)
    {
        if (events[0].res<0)
            printf("io[%d] return:%d\n",i,events[i].res);
        free(events[i].obj);
    }
    return cn_events;	    
}
*/
struct io_event events[MAX_PENDING_REQS];
int CVSSD::WaitEvents(int cn_min_events)
{
    int i;
    int isW;
    struct timespec timeout = {30, 0};
    int ret;    
    int ret2;
    int cn_events;
    long long int addr;
    const char* pNotation[5];
    int a=0;
    pNotation[0]="LPN correct only";
    pNotation[1]="all 0xFF";
    pNotation[2]="correct";
    pNotation[3]="incorrect";
    
    ASSERT(ctx!=(io_context_t)0xFFFFFFFF,"invalid ctx\n");
    cn_events = io_getevents(ctx, cn_min_events, MAX_PENDING_REQS, events, &timeout);
	if (cn_events==0)
	{
		printf("io_getevents: got nothing after 30 second!\n");
		return -1;
	}
//    FILE *fp1;  
/*
    fp1=fopen("rat.log","w");
    if(fp1==NULL)
    {	
        printf("open file fail\n");
    }
    else 
    {
        printf("open file ok\n");
    }
*/
    for(i=0;i<cn_events;i++)
    {
        if (((int)events[i].res)<0)
        {
            printf("io[%d] return:%s(%d).\nPress any key to cont\n",i,strerror(-events[i].res),events[i].res);
            exit(1);
//            getchar();
        }
        else
        {    
            addr = events[i].obj->u.c.offset >>(l2no_sector_in_page + l2no_byte_in_sector);        
            if(events[i].obj->aio_lio_opcode == 1 ) //IO_CMD_PWRITE
                isW=1;
            else if (events[i].obj->aio_lio_opcode == 0 ) //IO_CMD_PREAD
                isW=0;
            else
            {
             // ASSERT(0,"unkown event type\n ");
             	printf("Unknown event type %d\n",events[i].obj->aio_lio_opcode);
//                free(events[i].obj);
                continue;
			}
                                
            if(isW==1)
            {
                if (gSlience==false)
                    printf("======LPN: %lld in Write seq %d ======\n",
                    events[i].obj->u.c.offset >>(l2no_sector_in_page + l2no_byte_in_sector),cn_totalevents);            
                cn_totalevents++;
                if(pAdm!=0)
                    pAdm->WriteDoneCallback(addr);                    
            }
            else if(isW==0)
            {
                a++;
                //Check_data
                if (fp_chkdata>0)
                {    
                    ASSERT((fp_chkdata!=(CallBack)0xffffffff)&&(fp_chkdata!=(CallBack)0x00000000),"function pointer fail access \n");
                    ret = (*fp_chkdata)(events[i].obj->u.c.offset >>(l2no_sector_in_page + l2no_byte_in_sector), 
                        (unsigned char *)events[i].obj->u.c.buf, 
                        (unsigned long)(events[i].obj->u.c.nbytes>>(l2no_sector_in_page + l2no_byte_in_sector)));                
/*
                    if(ret!=1)
                    {
                        fprintf(Errfp,"LPN=%lld ; ret =  %d\n", addr, ret);
                    }
*/    
//                    if ((gSlience==false)&&(ret!=2))
                      if (gSlience==false)
                      {
                        printf("======LPN: %lld in Read type %d (%s) %p ======", addr, ret, pNotation[ret], *(unsigned long *)events[i].obj->u.c.buf );
//						if (ret!=2)
							printf("\n");
//						else
//							printf("\r");
					}                        
                
                    if(pAdm!=0)
                    {
                        ret2=pAdm->IsAllocate(addr);
                        if( ret2==1 && (ret==2 || ret==3) ){}
                        else if ( ret2==0 && ret==1 ){}
//      		          if(ret!=1){}
                        else
                        {
                            printf("===== Read data unexpected in addr %lld ======\n",addr);
//      	    	          ASSERT(0, "===== Read data unexpected in addr ");
                            dumphex((unsigned char *)events[i].obj->u.c.buf);                    
//              	          exit(1);
                            fprintf(Errfp,"Error LPN=%lld ret=%d \n", addr, ret);
                        }
                    }
//                    else
//                        printf("not access pAdm\n");
                }
            }
                
        }
        if (Cbuf.FreeBuf((unsigned char *)events[i].obj->u.c.buf, 
        (unsigned long)(events[i].obj->u.c.nbytes>>(l2no_sector_in_page + l2no_byte_in_sector)))==true) 
            free(events[i].obj);
        else  
		{		
			printf("freebuf failure\n");
//			printf("freebuf failure %p\n",events[i].obj->u.c.buf);
//            exit(1);
		}
    
    }
    
//
    return cn_events;
}


INT64 CVSSD::GetSizeA(void)
{
	INT64 end;
	end=lseek(hDevice,0,SEEK_END);
	return end;
}
