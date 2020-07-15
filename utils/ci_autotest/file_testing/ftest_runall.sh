#!/bin/bash
##
## Copyright (c) 2015-2020 Industrial Technology Research Institute.
##
## Licensed to the Apache Software Foundation (ASF) under one
## or more contributor license agreements.  See the NOTICE file
## distributed with this work for additional information
## regarding copyright ownership.  The ASF licenses this file
## to you under the Apache License, Version 2.0 (the
## "License"); you may not use this file except in compliance
## with the License.  You may obtain a copy of the License at
##
##    http://www.apache.org/licenses/LICENSE-2.0
##
## Unless required by applicable law or agreed to in writing,
## software distributed under the License is distributed on an
## "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
## KIND, either express or implied.  See the License for the
## specific language governing permissions and limitations
## under the License.
##
##
##
##
##
##
##  
##


#########################
#     include files     #
#########################

source ../common/config_test_env.sh
source ../common/common_func.sh
source ../common/common_global_var.sh
#source rwtest_runfunclist.sh

gtotal_test=5
grun_test=0
gsuccess_test=0
gfail_test=0

source ./ftest_runfunclist.sh

#########################
#   setup testing para  #
#########################

gtarget_dev=lfsm
gscope="min"
goutput_file="ftest_runall.sh.1234567890.out"

if [ $# -eq 1 ] ; then
    gtarget_dev=$1
    gscope="min"
    goutput_file="ftest_runall.sh.1234567890.out"
elif [ $# -eq 2 ] ; then
    gtarget_dev=$1
    gscope=$2
    goutput_file="ftest_runall.sh.1234567890.out"
elif [ $# -eq 3 ] ; then
    gtarget_dev=$1
    gscope=$2
    goutput_file=$3
else
    gtarget_dev=lfsm
    gscope="min"
    goutput_file="ftest_runall.sh.1234567890.out"
fi

log_msg "$test_scfn: [INFO] start testing with target dev=$gtarget_dev scope = $gscope output=$goutput_file"

run_ftest_formatfs $gtarget_dev $gscope
run_ftest_multidir $gtarget_dev $gscope
run_ftest_r_w $gtarget_dev $gscope
run_ftest_randsizefileRW $gtarget_dev $gscope
run_iozone_rw $gtarget_dev $gscope
 
[ -f $goutput_file ] && rm -f $goutput_file
echo "$gtotal_test $grun_test $gsuccess_test $gfail_test" > $goutput_file
log_msg "total_test=$gtotal_test run_test=$grun_test success=$gsuccess_test fail=$gfail_test"

if [ $gfail_test -gt 0 ]; then
    exit 1
fi
