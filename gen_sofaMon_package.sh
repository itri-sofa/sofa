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

build_dir=`pwd`
export LANG=en_US

target_dir="$build_dir/SOFAMon_Release/"

platform="linux"
version=1.0.00
release=1
sc_name="gen_sofaMon_package"

function env_prepare()
{
    echo "$sc_name: prepare environment"
    mkdir -p $target_dir

    /bin/rm -rf $target_dir/*    
}

function build_sofaMon()
{
    cd $build_dir/src_miscComp/sofa_monitor
    sh build_sofa_mon.sh install $target_dir
    cd $build_dir
    return 0
}

#parameter 1: platform: linux or ubuntu
#parameter 2: version
#parameter 3: release
#parameter 4: target directory
function gen_packages()
{
    cd utils/BuildScript

    if [ "$1" == "linux" ]; then
        sh Generate_sofaMon_RPM.sh $2 $3 $4
        /bin/rm -rf $build_dir/packages/SOFAMon-*.rpm
        /bin/mv SOFAMon-*.rpm $build_dir/packages/
    else
        sh Generate_sofaMon_deb.sh $2 $3 $4
    fi

    cd ../DeployScript

    /bin/cp deploy_sofaMon.sh $build_dir/packages/
    /bin/cp undeploy_sofaMon.sh $build_dir/packages/
    cd $build_dir
}

function verify_para()
{
    if [ "$1" != "linux" ] && [ "$1" != "ubuntu" ]
    then
        echo "$sc_name: wrong platform parameter $1"
        return 1
    fi

    if [ "$1" == "ubuntu" ]
    then
        echo "$sc_name not support platform build now $1"
        return 1
    fi

    return 0
}

function show_usage()
{
    echo "Usage $0 [platform: linux|ubuntu] $1 [version] $2 [release] $3"
}

if [ $# -le 1 ]; then
    if [ $# -eq 1 ]; then
        platform=$1
    fi
elif [ $# -eq 2 ]; then
    platform=$1
    version=$2
elif [ $# -eq 3 ]; then
    platform=$1
    version=$2
    release=$3
else
    show_usage
    exit 1
fi

verify_para $platform $version $release $target_dir
retval=$?
if [ "$retval" == 1 ]
then
    show_usage
    cd $build_dir
    exit 1
fi

echo "$0 platform=$platform verion=$version release=$release tmp_dir=$target_dir"
echo "$0 start to generate sofa monitor release packages"


env_prepare
build_sofaMon

gen_packages $platform $version $release $build_dir
retval=$?
if [ "$retval" == 1 ]
then
    echo "$0 generate packages fail"
    cd $build_dir
    exit 1
fi

rm -rf $target_dir

echo "Generate sofa packages success"


