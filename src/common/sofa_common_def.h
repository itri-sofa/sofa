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

#ifndef SOFA_COMMON_DEF_H_
#define SOFA_COMMON_DEF_H_

#define LOGICAL_SECT_BYTES 512
#define KERNEL_SECT_BYTES 512

#define EXPON_SECT_BYTES 9
#define EXPON_PAGE_BYTES  12    //KERNEL PAGE_SIZE is 4096 bytes
#define EXPON_PAGE_SECTS    3

#define SOFA_PAGE_BYTES 4096
#define SOFA_PAGE_SECTS  8
#define MOD_VAL_PAGE_SECTS  (SOFA_PAGE_SECTS - 1)

#define BYTE_BITS   8
#define EXPON_BYTE_BITS    3
#define MOD_VAL_BYTE    (BYTE_BITS - 1)

#define EXPON_MB_BYTES  20
#endif /* SOFA_COMMON_DEF_H_ */
