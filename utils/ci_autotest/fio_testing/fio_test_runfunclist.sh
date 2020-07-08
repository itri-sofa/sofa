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

function run_fio_test_fs()
{
    tTarget=$1
    tFs=$2

    log_msg "$test_scfn: [INFO] test fio_test_fs.scp with target $tTarget and $tFs format"
    sh fio_test_fs.scp $tTarget $tFs

    ret=$?

    if [ "$ret" != 0 ]; then
        log_msg "$test_scfn: [ERROR] test fio_test_fs.scp fail"
        gfail_test=$((gfail_test + 1))
    else
        gsuccess_test=$((gsuccess_test + 1))
    fi
    return $ret
}

function run_fio_test_raw()
{
    tTarget=$1
    tPart_no=$2

    log_msg "$test_scfn: [INFO] test fio_test_fs.scp with target $tTarget and $tPart_no partitions"
    sh fio_test_raw.scp $tTarget $tPart_no

    ret=$?

    if [ "$ret" != 0 ]; then
        log_msg "$test_scfn: [ERROR] test fio_test_raw.scp fail"
        gfail_test=$((gfail_test + 1))
    else
        gsuccess_test=$((gsuccess_test + 1))
    fi
    return $ret
}
