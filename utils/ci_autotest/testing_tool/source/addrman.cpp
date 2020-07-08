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
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ssdconf.h"
#include "addrman.h"

AddrManer::AddrManer(void)
{
  int i;
  for(i=0; i<CLASS; i++)
    ar_used_active_sblk[i]=0xffffffff;
  for(i=0; i<CN_SBLK; i++)
  {
    ar_used_sblk[i]=0;
    ar_written_done_sblk[i]=0;
  }  
//  memset(ar_used_sblk,0,CN_SBLK);
//  memset(ar_written_done_sblk,0,CN_SBLK);  
  new_sel_active=0;
}

AddrManer::~AddrManer(void)
{
}

void AddrManer::WriteDoneCallback(int addr)
{
  int seu;
  int page;
  int cn_pages_in_sblk = CN_PAGES * CN_BLOCK_OF_SEU;  
  seu = (int)addr / cn_pages_in_sblk;

  ASSERT(seu<CN_SBLK,"WriteDoneCallback seu overflow\n");
  ar_written_done_sblk[seu]++;
//  page = addr % cn_pages_in_sblk;  
//  if(ar_written_done_sblk[seu]<page)
//    ar_written_done_sblk[seu]=page;
  ASSERT(ar_written_done_sblk[seu]<=cn_pages_in_sblk,"WriteDoneCallback overflow\n");
  return;
}

int AddrManer::IsAllocate(int addr)
{
  int seu;
  int page;
  int cn_pages_in_sblk = CN_PAGES * CN_BLOCK_OF_SEU;  
  seu = (int)addr / cn_pages_in_sblk;
  page = addr % cn_pages_in_sblk;
  
  if (page < ar_written_done_sblk[seu])
    return true;
  else
    return false;
  
}

void AddrManer::PrintAddr(void)
{
  int i;
  int seu;
  for(i=0;i<CLASS;i++)
  {
    printf("SB %d Last Page = %d \n",ar_active_sblk[i] ,ar_used_active_sblk[i]);
    seu = ar_active_sblk[i];
    if((seu>=0)&&(seu<CN_SBLK))
      printf("Written sblk=%d \n",ar_written_done_sblk[seu]);
  }  
}

int AddrManer::FreeSEU(int s_range, int s_offset)
{
//  int new_sel_active;
  int i;
  new_sel_active = ((rand()%s_range)+s_offset)%CN_SBLK;
  ar_written_done_sblk[new_sel_active] = ar_written_done_sblk[new_sel_active]-ar_used_sblk[new_sel_active];  
  ar_used_sblk[new_sel_active] = 0;

  for(i=0; i<CLASS; i++)  
  {
    if( new_sel_active == ar_active_sblk[i])
    {
      ar_used_active_sblk[i]=0xffffffff;
      break;
    }
  }  
  return new_sel_active;
}

int AddrManer::AlloAddr(int len, int s_range, int s_offset)
{
  unsigned int addr;
  int sel_active=0;
  int cn_pages_in_sblk = CN_PAGES * CN_BLOCK_OF_SEU;

  sel_active = rand()%CLASS;
  new_sel_active=ar_active_sblk[sel_active];  
  if (ar_used_active_sblk[sel_active]>=cn_pages_in_sblk) // the seu full
  {
    do{
        new_sel_active = ((rand()%s_range)+s_offset)%CN_SBLK;
    }while (ar_used_sblk[new_sel_active]>0); // seu has been used

    ar_used_sblk[new_sel_active] = 1; 
    ar_used_active_sblk[sel_active] = 0; 
    ar_active_sblk[sel_active] = new_sel_active;
    printf("Assign a new active sblk %d for class %d\n", new_sel_active, sel_active);
  }
  
  addr = ar_active_sblk[sel_active] * cn_pages_in_sblk + ar_used_active_sblk[sel_active]; // seu*1024+page
  ar_used_active_sblk[sel_active] = ar_used_active_sblk[sel_active] + len;
  ar_used_sblk[new_sel_active] = ar_used_active_sblk[sel_active];
//  printf("ar_used_sblk[%d] =%d \n",new_sel_active,ar_used_sblk[new_sel_active] );
  return addr;
}