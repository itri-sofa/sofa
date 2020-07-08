#!/usr/bin/env bash
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

cur_dir=`pwd`
export LANG=en_US

function build_testing_tool()
{
    mkdir -p $1/testing_tool
    cd ./testing_tool
    sh build_uttool.sh
    cd $cur_dir

    cp ./testing_tool/ttest $1/testing_tool
    cp ./testing_tool/vtest $1/testing_tool
    cp ./testing_tool/8kttest $1/testing_tool
    cp ./testing_tool/8kttest00 $1/testing_tool
    cp ./testing_tool/8kttest55 $1/testing_tool
    cp ./testing_tool/8kttestff $1/testing_tool
    cp ./testing_tool/ftest $1/testing_tool
    cp ./testing_tool/filetest $1/testing_tool

    #build iozone
    cd ./iozone
    sh build_iozone.sh
    cd $cur_dir
    
    cp ./iozone/iozone3_394/src/current/iozone $1/testing_tool
}

function copy_all_scripts()
{
    mkdir -p $1/$2
    /bin/cp -rf ./$2/* $1/$2/
}

echo "install_ci_testscript.sh: prepare and copy all ci scripts to $1"

build_testing_tool $1

copy_all_scripts $1 common

copy_all_scripts $1 file_testing
copy_all_scripts $1 recovery_testing
copy_all_scripts $1 redundancy_testing
copy_all_scripts $1 rw_testing
copy_all_scripts $1 fio_testing
/bin/cp -f citest_runall.sh $1
/bin/cp -f citest_runsclist.sh $1


