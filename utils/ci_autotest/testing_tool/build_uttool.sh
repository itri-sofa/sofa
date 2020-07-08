##
## Copyright (c) 2015-2025 Industrial Technology Research Institute.
##
## Licensed to the Apache Software Foundation (ASF) under one
## or more contributor license agreements.  See the NOTICE file
## distributed with this work for additional information
## regarding copyright ownership.  The ASF licenses this file
## to you under the Apache License, Version 2.0 (the
## "License"); you may not use this file except in compliance
## with the License.  You may obtain a copy of the License at
##
##   http://www.apache.org/licenses/LICENSE-2.0
##
## Unless required by applicable law or agreed to in writing,
## software distributed under the License is distributed on an
## "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
## KIND, either express or implied.  See the License for the
## specific language governing permissions and limitations
## under the License.
##
g++ vtest.cpp source/alssd2.cpp source/bfun.cpp source/mem512.cpp \
 perftest.cpp source/cftest.cpp source/addrman.cpp -I include -g -o ttest -laio \
-DCN_SBLK=2046 -DCN_PAGES=64 -DCN_BLOCK_OF_SEU=16 -DNUM_SECTOR_IN_PAGE_ORDER=3

g++ vtest.cpp source/alssd2.cpp source/bfun.cpp source/mem512.cpp \
 perftest.cpp source/cftest.cpp source/addrman.cpp -I include -g -o 8kttest -laio \
-DCN_SBLK=2046 -DCN_PAGES=64 -DCN_BLOCK_OF_SEU=16 -DNUM_SECTOR_IN_PAGE_ORDER=4


g++ vtest.cpp source/alssd2.cpp source/bfun.cpp source/mem512.cpp \
 perftest.cpp source/cftest.cpp source/addrman.cpp -I include -g \
-o 8kttestff -laio \
-DCN_SBLK=2046 -DCN_PAGES=64 -DCN_BLOCK_OF_SEU=16 -DNUM_SECTOR_IN_PAGE_ORDER=4 \
-DFIXPAYLOAD=0xff

g++ vtest.cpp source/alssd2.cpp source/bfun.cpp source/mem512.cpp \
 perftest.cpp source/cftest.cpp source/addrman.cpp -I include -g \
-o 8kttest55 -laio \
-DCN_SBLK=2046 -DCN_PAGES=64 -DCN_BLOCK_OF_SEU=16 -DNUM_SECTOR_IN_PAGE_ORDER=4 \
-DFIXPAYLOAD=0x55

g++ vtest.cpp source/alssd2.cpp source/bfun.cpp source/mem512.cpp \
 perftest.cpp source/cftest.cpp source/addrman.cpp -I include -g \
-o 8kttest00 -laio \
-DCN_SBLK=2046 -DCN_PAGES=64 -DCN_BLOCK_OF_SEU=16 -DNUM_SECTOR_IN_PAGE_ORDER=4 \
-DFIXPAYLOAD=0x00

g++ vtest.cpp source/alssd2.cpp source/bfun.cpp source/mem512.cpp \
 perftest.cpp source/cftest.cpp source/addrman.cpp -I include -g -o vtest -laio \
-DCN_SBLK=2046 -DCN_PAGES=64 -DCN_BLOCK_OF_SEU=16 -DNUM_SECTOR_IN_PAGE_ORDER=0

g++ ftest.cpp -o ftest
gcc filetest.c -o filetest -lm