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

function parse_testing_result()
{
    result_str="`cat $tOutput`"
    result_arr=(${result_str})

    total_str=${result_arr[0]}
    run_str=${result_arr[1]}
    success=${result_arr[2]}
    fail=${result_arr[3]}

    gtotal_test=$((gtotal_test + total_str))
    grun_test=$((grun_test + run_str))
    gsuccess_test=$((gsuccess_test + success))
    gfail_test=$((gfail_test + fail))
}

function run_rw_testing()
{
    tTarget=$1
    tScope=$2
    tOutput="rwtest_run_all_01234567890.out"

    log_msg "$test_scfn: [INFO] test rw_testing with target $tTarget and $tScope scope"

    cd rw_testing
    sh rwtest_runall.sh $tTarget $tScope $tOutput
   
    parse_testing_result $tOutput
    
    rm -rf $tOutput
    
    cd ..    
    if [ $gfail_test -eq 0 ]; then
        ret=0
    else
        ret=1
    fi

    return $ret
}

function run_file_testing()
{
    tTarget=$1
    tScope=$2
    tOutput="ftest_run_all_01234567890.out"

    log_msg "$test_scfn: [INFO] test file_testing with target $tTarget and $tScope scope"

    cd file_testing
    sh ftest_runall.sh $tTarget $tScope $tOutput

    parse_testing_result $tOutput

    rm -rf $tOutput

    cd ..
    if [ $gfail_test -eq 0 ]; then
        ret=0
    else
        ret=1
    fi

    return $ret
}

function run_fio_testing()
{
    tTarget=$1
    tOutput="fio_test_run_all_01234567890.out"

    log_msg "$test_scfn: [INFO] fio test with target $tTarget"

    cd fio_testing
    sh fio_test_runall.sh $tTarget $tOutput

    #parse_testing_result $tOutput

    rm -rf $tOutput

    cd ..
    if [ $gfail_test -eq 0 ]; then
        ret=0
    else
        ret=1
    fi

    return $ret
}
