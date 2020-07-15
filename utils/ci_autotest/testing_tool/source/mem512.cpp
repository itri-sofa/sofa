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
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/mem512.h"
#include "../include/ssdconf.h"

CPageBuffer::CPageBuffer(void)
{
  int i;
  for(i=0; i<MAX_PAGE_BUFFER; i++)
    bitmap[i]=0;
  pOrgBuf=(unsigned char*)malloc(MAX_PAGE_BUFFER*PAGE_SIZE+PAGE_SIZE);
  buffer=(unsigned char*)((((unsigned long)pOrgBuf+(PAGE_SIZE-1))/PAGE_SIZE)*PAGE_SIZE);
  memset(buffer,0x00,MAX_PAGE_BUFFER*PAGE_SIZE);  
}

CPageBuffer::~CPageBuffer(void)
{
  free(pOrgBuf);
}
/*
unsigned char * CPageBuffer::Allocate(int len)
{
  int i,safe_count=0;
  int spt=-1;
  for(i=0; i<MAX_PAGE_BUFFER;i++)
  {
    if (bitmap[i]==0)
    {
      if (safe_count==0)
        spt=i;
      safe_count++;
    }
    else
    {
      safe_count=0;
      spt=-1;
    }
    if (safe_count==len)
      break;
  }
  if (safe_count!=len)
    return 0;
  ASSERT(spt!=-1,"spt==-1\n");
  for(i=0;i<len;i++)
  {
    ASSERT(bitmap[i+spt]==0,"non zero\n");
    bitmap[i+spt]=1;
  }
  return (buffer+(spt*PAGE_SIZE));


}
*/
unsigned char * CPageBuffer::Allocate(int len)
{
  int i,j;
//  int len=1;
  int safe_count=0;
  int spt=0;
  int ept=0;
  for(i=0; i<MAX_PAGE_BUFFER; i++)
  {
    if ( bitmap[i]==0 )
    {
      safe_count++; 
//      printf("safe_count %d \n",safe_count);
    }
    else if ( bitmap[i]==1 && (safe_count<len) )
    {
      safe_count=0;
      spt=0;
      ept=0; 
    }  
    
//    if ( safe_count==1 )
//      spt=i;
    if ( safe_count==len )
    {
      ept=i+1;
      spt=ept-len;  
      break;
    }  
  }       
  if (spt==ept)
  {
    printf("no enough memory to locate \n");
    return 0;
  }  
  for (i=spt; i< ept; i++)
  {
      bitmap[i]=1;
//      printf("bitmap[%d]= %p\n",i,buffer+(spt*PAGE_SIZE));      
  }    
  ASSERT((spt>=0)&&(spt<=MAX_PAGE_BUFFER),"wrong buffer allocate index");
  return buffer+(spt*PAGE_SIZE);          
  
}

bool CPageBuffer::FreeBuf(unsigned char* ptr, int len)
{
  int idx;
  int i;
  
  idx=(ptr-buffer)/PAGE_SIZE;
//  printf("index %d\n",idx);
  if (idx > MAX_PAGE_BUFFER || idx < 0)
    return false;
  
  for(i=idx; i<idx+len; i++)
  {
    bitmap[i]=0;
    memset(ptr,0x00,PAGE_SIZE);
  }  
  return true;
}