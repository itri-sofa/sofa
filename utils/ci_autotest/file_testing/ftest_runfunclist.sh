#!/bin/bash
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

function run_ftest_formatfs()
{
    tTarget=$1
    tScope=$2

    grun_test=$((grun_test + 1))
    log_msg "$test_scfn: [INFO] test ftest_formatfs.scp with target $tTarget and $tScope scope"
    if [ "$tScope" == "min" ]; then
        sh ftest_formatfs.scp $tTarget 5
    elif [ "$tScope" == "mdt" ]; then
        sh ftest_formatfs.scp $tTarget 15
    else
        sh ftest_formatfs.scp $tTarget 50
    fi

    ret=$?

    if [ "$ret" != 0 ]; then
        log_msg "$test_scfn: [ERROR] test ftest_formatfs.scp fail"
        gfail_test=$((gfail_test + 1))
    else
        gsuccess_test=$((gsuccess_test + 1))
    fi
    return $ret
}

function run_ftest_multidir()
{
    tTarget=$1
    tScope=$2

    grun_test=$((grun_test + 1))
    log_msg "$test_scfn: [INFO] test ftest_multidir.scp with target $tTarget and $tScope scope"
    if [ "$tScope" == "min" ]; then
        sh ftest_multidir.scp $tTarget 20
    elif [ "$tScope" == "mdt" ]; then
        sh ftest_multidir.scp $tTarget 50
    else
        sh ftest_multidir.scp $tTarget 100
    fi

    ret=$?

    if [ "$ret" != 0 ]; then
        log_msg "$test_scfn: [ERROR] test ftest_multidir.scp fail"
        gfail_test=$((gfail_test + 1))
    else
        gsuccess_test=$((gsuccess_test + 1))
    fi
    return $ret
}

function run_ftest_r_w()
{
    tTarget=$1
    tScope=$2

    grun_test=$((grun_test + 1))
    log_msg "$test_scfn: [INFO] test ftest_r_w.scp with target $tTarget and $tScope scope"
    if [ "$tScope" == "min" ]; then
        sh ftest_r_w.scp $tTarget 200
    elif [ "$tScope" == "mdt" ]; then
        sh ftest_r_w.scp $tTarget 500
    else
        sh ftest_r_w.scp $tTarget 1000
    fi

    ret=$?

    if [ "$ret" != 0 ]; then
        log_msg "$test_scfn: [ERROR] test ftest_r_w.scp fail"
        gfail_test=$((gfail_test + 1))
    else
        gsuccess_test=$((gsuccess_test + 1))
    fi
    return $ret
}

function run_ftest_randsizefileRW()
{
    tTarget=$1
    tScope=$2

    grun_test=$((grun_test + 1))
    log_msg "$test_scfn: [INFO] test ftest_randsizefileRW.scp with target $tTarget and $tScope scope"
    if [ "$tScope" == "min" ]; then
        sh ftest_r_w.scp $tTarget 1
    elif [ "$tScope" == "mdt" ]; then
        sh ftest_r_w.scp $tTarget 5
    else
        sh ftest_r_w.scp $tTarget 10
    fi

    ret=$?

    if [ "$ret" != 0 ]; then
        log_msg "$test_scfn: [ERROR] test ftest_randsizefileRW.scp fail"
        gfail_test=$((gfail_test + 1))
    else
        gsuccess_test=$((gsuccess_test + 1))
    fi
    return $ret
}

function run_iozone_rw()
{
    tTarget=$1
    tScope=$2

    grun_test=$((grun_test + 1))
    log_msg "$test_scfn: [INFO] test iozone_rw.scp with target $tTarget and $tScope scope"
    if [ "$tScope" == "min" ]; then
        sh iozone_rw.scp $tTarget
    elif [ "$tScope" == "mdt" ]; then
        sh iozone_rw.scp $tTarget
    else
        sh iozone_rw.scp $tTarget
    fi

    ret=$?

    if [ "$ret" != 0 ]; then
        log_msg "$test_scfn: [ERROR] test iozone_rw.scp fail"
        gfail_test=$((gfail_test + 1))
    else
        gsuccess_test=$((gsuccess_test + 1))
    fi
    return $ret
}

