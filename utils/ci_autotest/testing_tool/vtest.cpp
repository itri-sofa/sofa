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
#include "include/destio.h"

int rawwrite (CVSSD& ssd, unsigned long addr, unsigned long len)
{
    unsigned char pBuf[PAGE_SIZE*MAX_PAGE] __attribute__ ((aligned (PAGE_SIZE)));

    if (len>MAX_PAGE) {
        printf("Len should be smaller than MAX_PAGE\n");
        return false;
    }
    PageBaseFill(addr, len, pBuf);

    if (ssd.WriteByLPN(addr, len, pBuf) == false) {
        return false;
    }

    ssd.WaitAllEventDone(1);
    printf("Write cmd done\n");

    return true;
}


int gdate=20140314;
int parse_logger (unsigned char *buffer, int idx_pg)
{
    int i, start;
    int round;
    struct sdestio *pdestio;
    pdestio = (struct sdestio* )buffer;
    if (idx_pg == 0) {
        if (pdestio[0].header.date!=gdate)
            return 0;
        start=1;
        round = pdestio[0].header.cn_item;    
        printf("=== idx %d, count %d ===\n", pdestio[i].header.idx, round);
    }

    for (i = start; i < 128; i++) {
        if ((int)pdestio[i].payload.type==0xFFFFFFFF) {
            break;
        }
        printf("[%d] %8llu %8llu (%d)\n", (int)pdestio[i].payload.type, 
            pdestio[i].payload.lpn, pdestio[i].payload.pbn, (int)pdestio[i].payload.err);
    }

    return 0;
}

int rawread_A (CVSSD& ssd, unsigned long addr, unsigned long len, bool fg_parse)
{
    unsigned char pBuf[PAGE_SIZE*MAX_PAGE] __attribute__ ((aligned (PAGE_SIZE)));
    int i;
    if (len>MAX_PAGE) {
        printf("Len should be smaller than MAX_PAGE\n");
        return false;
    }

    if (ssd.ReadByLPN(addr, len, pBuf) == false) {
        return false;
    }

    ssd.WaitAllEventDone(0);
    printf("Read cmd done\n");

    for (i = 0; i < len; i++) {
        printf("Page %d\n", addr+i);
        if (fg_parse == true) {
            parse_logger(pBuf+(i*PAGE_SIZE), i%128);
        } else {
            dumphex(pBuf+(i*PAGE_SIZE));
        }
    }

    return true;
}

int rawread(CVSSD& ssd, unsigned long addr, unsigned long len)
{
    return rawread_A(ssd, addr, len, false);
}

int gSeed=12345;
int gDelay=0;
int main(int argc, char* argv[])
{
    CVSSD ssd;
//    int Seed;
    int ret;
    char* pEnvStr;
    char devdesc[32];
    char cmd[32];
    unsigned long addr;
    unsigned int trgaddr;
    unsigned int len;
    unsigned long range;
    unsigned long round;
    char* pNextNum;

    if (argc < 3) {
        printf("vtest devname cmd src trg len\n");
        return 1;
    }
    strcpy(devdesc, argv[1]);
    strcpy(cmd, argv[2]);
    pEnvStr = getenv("utest_seed48");
    if (pEnvStr) {
        gSeed=atol(pEnvStr);
        srand48(gSeed);
        printf("Get utest_seed48=%d\n", gSeed);
    } else {
        srand48(11111);
    }

    pEnvStr = getenv("utest_udelay");
    if (pEnvStr) {
        gDelay = atol(pEnvStr);
        printf("Get utest_udelay=%d\n", gDelay);
    } else {
        gDelay = 0;
    }

    ssd.InitParameter(NUM_SECTOR_IN_PAGE_ORDER,6);

    if (argc == 5) {
        addr = atoi(argv[3]);
        if (pNextNum = strchr(argv[3], '*')) {
            if (*pNextNum == 0) {
                perror("wrong syntax");
                return false;
            }
            addr *= atoi(pNextNum+1);
        }
        if (pNextNum = strchr(argv[3], '/')) {
            if (*pNextNum == 0) {
                perror("wrong syntax");
                return false;
            }
            addr /= atoi(pNextNum+1);
        }
        round = atoi(argv[4]);
        len = 1;
        range = round;
    } else if (argc == 6) {
        range = atol(argv[3]);
        round = atol(argv[4]);
        len = 1;
        addr = atol(argv[5]);
    } else if (argc == 7) {
        range = atoi(argv[3]);
        round = atoi(argv[4]);
        addr = atoi(argv[5]);
        len = atoi(argv[6]);
    }    

    ssd.SetDesc(devdesc);
    if (ssd.Open(-1) == false) {
        printf("Fail to open %s\n",devdesc);
        return 1;
    }

    /* comment case*/
    if (strcmp(cmd,"hybrid_randw") == 0) {
        ret=hybrid_randswsr(ssd, addr, range, round, gSeed, len, false, true);
    } else if (strcmp(cmd,"hybrid_srcheck")==0) {
        ret=hybrid_randswsr(ssd,addr,range,round,gSeed,len,true,false);
    } else if (strcmp(cmd,"hybrid_randr")==0) {
        ret=hybrid_randswsr(ssd,addr,range,round,gSeed,len,false,false);
    } else if (strcmp(cmd,"randswsr")==0) {
        ret=randsw_seqsr(ssd,addr,range,round,gSeed,len,false);
    } else if (strcmp(cmd,"chkrandswsr")==0) {
        ret=randsw_seqsr(ssd,addr,range,round,gSeed,len,true);
    } else if (strcmp(cmd,"swrandsr")==0) {
        ret=pf_4K_write_with_randread(ssd,addr,round,false,len);
    } else if (strcmp(cmd,"perfsw")==0) {
        ret=pf_4K_write(ssd,addr,round,false,len,1);
    } else if (strcmp(cmd,"perfbsw")==0) {
        ret=pf_4K_batchwrite(ssd,addr,round,false,false,range,false);
    } else if (strcmp(cmd,"perfrbsw")==0) {
        ret=pf_4K_batchwrite(ssd,addr,round,false,true,range,false);
    } else if (strcmp(cmd,"perfrbsw_step")==0) {
        ret=pf_4K_batchwrite(ssd,addr,round,false,true,range,true);
    } else if (strcmp(cmd,"perfbsw_step")==0) {
        ret=pf_4K_batchwrite(ssd,addr,round,false,false,range,true);
    } else if (strcmp(cmd,"perfbsr")==0) {
        ret=pf_4K_batchread(ssd,addr,round,false,false,range,false);
    } else if (strcmp(cmd,"perfrbsr")==0) {
        ret=pf_4K_batchread(ssd,addr,round,false,true,range,false);
    } else if (strcmp(cmd,"perfrbsr_step")==0) {
        ret=pf_4K_batchread(ssd,addr,round,false,true,range,true);
    } else if (strcmp(cmd,"perfsr")==0) {
        ret=pf_4K_read(ssd,addr,round,false);
    } else if (strcmp(cmd,"perfrandsw")==0) {
        ret=pf_4K_randwrite(ssd,addr,range,round,false,len);
    } else if (strcmp(cmd,"randsw")==0) {
        ret=pf_4K_randwrite(ssd,addr,range,round,true,len);
    } else if (strcmp(cmd,"perfrandsr")==0) {
        ret=pf_4K_randread(ssd,addr,range,round,false,len);
    } else if (strcmp(cmd,"randsr")==0) {
        ret=pf_4K_randread(ssd,addr,range,round,true,len);
    } else if (strcmp(cmd,"r")==0) {
        len=round;
        ret=rawread(ssd, addr, len);
    } else if (strcmp(cmd,"w")==0) {
        len=round;
        ret=rawwrite(ssd, addr, len);
    } else if (strcmp(cmd,"sr")==0) {
        ret=pf_4K_read(ssd,addr,round,true);
    } else if (strcmp(cmd,"sw")==0) {
        ret=pf_4K_write(ssd,addr,round,true,len,1);
    } else if (strcmp(cmd,"jsw")==0) {
      ret=pf_4K_write(ssd,range,round,true,len,addr);
    } else if (strcmp(cmd,"jsr")==0) {
      ret=pf_4K_jump_read(ssd,range,round,true,len,addr);
    } else if (strcmp(cmd,"replay")==0) {// set round (i.e. range) to zero means no boundary
        ret=log_replay(ssd,argv[3],addr,round,0);
    } else if (strcmp(cmd,"chkreplay")==0) {// set round (i.e. range) to zero means no boundary
        ret=log_replay_chk(ssd,argv[3],addr,round);
    } else if (strcmp(cmd,"delayreplay")==0) {
        ret=log_replay(ssd,argv[3],addr,round,1);
    } else if (strcmp(cmd,"parse_log")==0) {
        len=round;
        ret=rawread_A(ssd,addr*128,len*128,true);
    } else {
        printf("unknown cmd %s\n",cmd);
    }

    ssd.Close(true);

    return 0;
}
