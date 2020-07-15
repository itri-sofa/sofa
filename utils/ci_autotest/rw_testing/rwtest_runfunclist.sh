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

function run_onlysw4times()
{
    tTarget=$1
    tScope=$2

    grun_test=$((grun_test + 1))
    log_msg "$test_scfn: [INFO] test onlysw4times.scp with target $tTarget and $tScope scope"
    if [ "$tScope" == "min" ]; then
        #write 4G data = 1048576 pages
        sh onlysw4times.scp $tTarget 1048567
    elif [ "$tScope" == "mdt" ]; then
        #write 40G data = 10485760 pages
        sh onlysw4times.scp $tTarget 10485760
    else
        #write 200G data = 52428800 pages
        sh onlysw4times.scp $tTarget 52428800
    fi

    ret=$?

    if [ "$ret" != 0 ]; then
        log_msg "$test_scfn: [ERROR] test onlysw4times.scp fail"
        gfail_test=$((gfail_test + 1))
    else
        gsuccess_test=$((gsuccess_test + 1))
    fi
    return $ret
}

function run_scsw4times()
{
    tTarget=$1
    tScope=$2

    grun_test=$((grun_test + 1))

    log_msg "$test_scfn: [INFO] test scsw4times.scp with target $tTarget and $tScope scope"
    if [ "$tScope" == "min" ]; then
        #write 10M data = 2560 pages
        sh scsw4times.scp $tTarget 2560
    elif [ "$tScope" == "mdt" ]; then
        #write 100M data = 25600 pages
        sh scsw4times.scp $tTarget 25600
    else
        #write 1G data = 256000 pages
        sh scsw4times.scp $tTarget 256000
    fi

    ret=$?

    if [ "$ret" != 0 ]; then
        log_msg "$test_scfn: [ERROR] test scsw4times.scp fail"
        gfail_test=$((gfail_test + 1))
    else
        gsuccess_test=$((gsuccess_test + 1))
    fi
    return $ret
}

function run_sw4times()
{
    tTarget=$1
    tScope=$2

    grun_test=$((grun_test + 1))

    log_msg "$test_scfn: [INFO] test sw4times.scp with target $tTarget and $tScope scope"
    if [ "$tScope" == "min" ]; then
        #write 4G data = 1048576 pages
        sh sw4times.scp $tTarget 1048576
    elif [ "$tScope" == "mdt" ]; then
        #write 40G data = 10485760 pages
        sh sw4times.scp $tTarget 10485760
    else
        #write 200G data = 52428800 pages
        sh sw4times.scp $tTarget 52428800
    fi

    ret=$?

    if [ "$ret" != 0 ]; then
        log_msg "$test_scfn: [ERROR] test sw4times.scp fail"
        gfail_test=$((gfail_test + 1))
    else
        gsuccess_test=$((gsuccess_test + 1))
    fi
    return $ret
}

function run_randw_Ten_times()
{
    tTarget=$1
    tScope=$2

    grun_test=$((grun_test + 1))

    log_msg "$test_scfn: [INFO] test randw_Ten_times.scp with target $tTarget and $tScope scope"
    if [ "$tScope" == "min" ]; then
        #write 4G data = 1048576 pages
        sh randw_Ten_times.scp $tTarget 1048576
    elif [ "$tScope" == "mdt" ]; then
        #write 40G data = 10485760 pages
        sh randw_Ten_times.scp $tTarget 10485760
    else
        #write 200G data = 52428800 pages
        sh randw_Ten_times.scp $tTarget 52428800
    fi

    ret=$?

    if [ "$ret" != 0 ]; then
        log_msg "$test_scfn: [ERROR] test randw_Ten_times.scp fail"
        gfail_test=$((gfail_test + 1))
    else
        gsuccess_test=$((gsuccess_test + 1))
    fi
    return $ret
}

function run_randw_page_32()
{
    tTarget=$1
    tScope=$2

    grun_test=$((grun_test + 1))

    log_msg "$test_scfn: [INFO] test randw_page_32.scp with target $tTarget and $tScope scope"
    if [ "$tScope" == "min" ]; then
        #write 4G data = 1048576 pages
        sh randw_page_32.scp $tTarget 1048576
    elif [ "$tScope" == "mdt" ]; then
        #write 40G data = 10485760 pages
        sh randw_page_32.scp $tTarget 10485760
    else
        #write 200G data = 52428800 pages
        sh randw_page_32.scp $tTarget 52428800
    fi

    ret=$?

    if [ "$ret" != 0 ]; then
        log_msg "$test_scfn: [ERROR] test randw_page_32.scp fail"
        gfail_test=$((gfail_test + 1))
    else
        gsuccess_test=$((gsuccess_test + 1))
    fi
    return $ret
}

function run_randw_page_7()
{
    tTarget=$1
    tScope=$2

    grun_test=$((grun_test + 1))

    log_msg "$test_scfn: [INFO] test randw_page_7.scp with target $tTarget and $tScope scope"
    if [ "$tScope" == "min" ]; then
        #write 4G data = 1048576 pages
        sh randw_page_7.scp $tTarget 1048576
    elif [ "$tScope" == "mdt" ]; then
        #write 40G data = 10485760 pages
        sh randw_page_7.scp $tTarget 10485760
    else
        #write 200G data = 52428800 pages
        sh randw_page_7.scp $tTarget 52428800
    fi

    ret=$?

    if [ "$ret" != 0 ]; then
        log_msg "$test_scfn: [ERROR] test randw_page_7.scp fail"
        gfail_test=$((gfail_test + 1))
    else
        gsuccess_test=$((gsuccess_test + 1))
    fi
    return $ret
}

function run_randw_sector()
{
    tTarget=$1
    tScope=$2

    grun_test=$((grun_test + 1))

    log_msg "$test_scfn: [INFO] test randw_sector.scp with target $tTarget and $tScope scope"
    if [ "$tScope" == "min" ]; then
        #write 10M data = 2560 pages
        sh randw_sector.scp $tTarget 2560
    elif [ "$tScope" == "mdt" ]; then
        #write 100M data = 25600 pages
        sh randw_sector.scp $tTarget 25600
    else
        #write 1G data = 256000 pages
        sh randw_sector.scp $tTarget 256000
    fi

    ret=$?

    if [ "$ret" != 0 ]; then
        log_msg "$test_scfn: [ERROR] test randw_sector.scp fail"
        gfail_test=$((gfail_test + 1))
    else
        gsuccess_test=$((gsuccess_test + 1))
    fi
    return $ret
}

#TODO: we should erase disk first, complicate testing
function run_rw_perf()
{
    tTarget=$1
    tScope=$2

    grun_test=$((grun_test + 1))

    log_msg "$test_scfn: [INFO] test rw_perf.scp with target $tTarget"
    sh rw_perf.scp $tTarget

    ret=$?

    if [ "$ret" != 0 ]; then
        log_msg "$test_scfn: [ERROR] test rw_perf.scp fail"
        gfail_test=$((gfail_test + 1))
    else
        gsuccess_test=$((gsuccess_test + 1))
    fi
    return $ret
}

