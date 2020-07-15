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
dir=`pwd`

echo "Ready to build iozone"

deploy_dir="/usr/sofa/"
export LANG=en_US

function errorexit()
{
        echo "[X]" $1
	cd $dir
        exit 1
}

function execute()
{
        echo "[*] $1"
        result="`$1`"
        if [ $? -ne 0 ]; then
                errorexit "Failed to \"$1\""
        fi
}

function build_clean()
{
    cd ./iozone3_394/src/current
    execute "make clean"
    cd $dir
}

function build_only()
{
    build_clean
    cd ./iozone3_394/src/current
    execute "make linux"
    cd $dir
}

function build_inst()
{
    build_only

    echo "copy iozone to $1/bin/tool/benchmark_tool/"
    mkdir -p $1/bin/tool/benchmark_tool/
    rm -rf $1/bin/tool/benchmark_tool/iozone
    /bin/cp ./iozone3_394/src/current/iozone $1/bin/tool/benchmark_tool/

    echo "copy iozone_test.sh to $1/bin/test/correctness/"
    mkdir -p $1/bin/testing/correctness/
    chmod 777 ./iozone_test.sh
    /bin/cp ./iozone_test.sh $1/bin/testing/correctness/
}

if [ $# -eq 0 ]; then
    build_only
elif [ $# -eq 1 ]; then
    if [ "$1" == "clean" ]; then
        build_clean
    elif [ "$1" == "install" ]; then
        build_inst $deploy_dir
    else
        echo "Usage: $0 [action = install | clean] $1 [deploy_dir] $2"
    fi
elif [ $# -eq 2 ]; then
    if [ "$1" == "clean" ]; then
        build_clean
    elif [ "$1" == "install" ]; then
        build_inst $2
    else
        echo "Usage: $0 [action = install | clean] $1 [deploy_dir] $2"
    fi
else
    echo "Usage: $0 [action = install | clean] $1 [dir] $2"
fi

