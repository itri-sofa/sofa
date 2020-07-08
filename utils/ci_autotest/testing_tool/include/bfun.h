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
#ifndef BFUN_H
#define BFUN_H
typedef struct{
	int m_nSum; 		/* Number of writes */
	int m_nSize;  		/* Per-Req size in sector */
	int m_nMd;			/* Delay in millisecond */
	int m_nBatchSize; 
	int m_nType; 		/* 0, sequential; 1, random */
	int m_nRw; 			/* 0, read request; 1, write request, 2, check the correctness of the read/write pair*/
	char* m_pPayload;
	int m_nPayloadLen;
	unsigned long long m_start_sector;
} sftl_ioctl_simu_w_t;


void dumphex(unsigned char* );
int Check_data(unsigned long, unsigned char* , int);
int Check_data_hybrid(unsigned long, unsigned char* , int);
void SeqFill(unsigned long, unsigned long, unsigned char* );
void PageBaseFill(unsigned long, unsigned long, unsigned char* );
void NormalFill(unsigned long, unsigned long, unsigned char* );
void RandomFill(unsigned long, unsigned long, unsigned char* );
//int sim_LFSM_access_Ex(CVSSD&, unsigned char*,int,int,int.int);
bool bLFSM_erase(unsigned long, unsigned long );
bool bLFSM_copy(unsigned long, unsigned long, unsigned long );
bool bLFSM_markbad(unsigned long, unsigned long);
bool bLFSM_query(unsigned long, unsigned long, bool );
#endif