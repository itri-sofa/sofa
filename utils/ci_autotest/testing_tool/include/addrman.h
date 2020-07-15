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
#ifndef AddrManer_H
#define AddrManer_H

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

//#define CN_SBLK 2046
#define CLASS 3

class AddrManer
{
protected:
  unsigned int ar_used_sblk[CN_SBLK]; //table for seu writtern page
  unsigned int ar_used_active_sblk[CLASS]; //page filled number of the seu
  int ar_written_done_sblk[CN_SBLK];
  int ar_active_sblk[CLASS]; //selected seu being used
  int new_sel_active;  
public:
  AddrManer(void);
  ~AddrManer(void);
  int AlloAddr(int len, int s_range, int s_offset);
  int FreeSEU(int s_range, int s_offset);
  void PrintAddr(void);
  int IsAllocate(int addr);
  void WriteDoneCallback(int addr) ;
};

#endif