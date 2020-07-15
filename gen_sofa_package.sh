#!/usr/bin/env bash
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

build_dir=`pwd`
export LANG=en_US
error_result=error_result.txt

target_dir="$build_dir/SOFA_Release/"

platform="linux"
version=1.0.00
release=1

function error_message()
{
    echo "$1"
    echo "$1" >> $error_result
}

function env_prepare()
{
    echo "gen_sofa_package.sh: prepare environment"
    mkdir -p $target_dir

    /bin/rm -rf $target_dir/*    
}

function build_sofa()
{
    sofa_src_dir="$build_dir/src"
    sofa_build_sc="build_sofa.sh"

    #TODO: we should modify build_sofa.sh and pass the inst dir as parameter
    cd $sofa_src_dir

    #TODO: how to catch the return code when build fail
    sh $sofa_build_sc
    build_test=$?
    echo "build test :"${build_test}
  
    if [ "${build_test}" == "0" ]; then
       echo -e "******************************************"
       echo -e "* build_sofa.sh is successfully executed *"
       echo -e "******************************************"
    else
       echo -e "*******************************************"
       echo -e "* build_sofa.sh have errors in script !!! *"
       echo -e "*******************************************"
       exit 1
    fi

    #copy binary since the build_sofa.sh didn't support inst dir as parameter
    rm -rf $target_dir/sofa_daemon
    /bin/cp $sofa_src_dir/UserDaemon/sofa_daemon $target_dir

    rm -rf $target_dir/sofa.ko
    /bin/cp $sofa_src_dir/sofa_main/sofa.ko $target_dir

    rm -rf $target_dir/lfsmdr.ko
    /bin/cp $sofa_src_dir/storage_core/lfsm/lfsmdr.ko $target_dir

	rm -rf $target_dir/rfsioctl
	/bin/cp $sofa_src_dir/storage_core/lfsm/rfsioctl $target_dir
    cd $build_dir
    return 0
}

function build_tool()
{
    util_src_dir="./utils/tools/"

    cd $util_src_dir

    #admin tool
    cd ./admtools/
    make
    admtool_build_test=$?
    echo "admtool test:"${admtool_build_test}

    if [ "${admtool_build_test}" == "0" ]; then
        echo -e "--------> admtool test success"
    else
        echo -e "--------> admtool test have errors !!!"
        exit 1
    fi

    rm -rf $target_dir/bin/sofaadm
    /bin/cp sofaadm $target_dir/bin/
    cd ..

    cd $build_dir
    return 0
}

function build_src()
{
    build_sofa

    build_tool

    return 0;
}

function prepare_bin()
{
    mkdir -p $target_dir/bin/
    /bin/cp -R $build_dir/bin/* $target_dir/bin/
    chmod 755 -R $target_dir/bin/*
}

function prepare_cfg()
{
    mkdir -p $target_dir/config/
    /bin/cp -R $build_dir/conf/* $target_dir/config/
}


function prepare_tool()
{
    mkdir -p $target_dir/bin/tool/disk_tool
    /bin/cp $build_dir/utils/tools/disk_tool/* $target_dir/bin/tool/disk_tool/
	chmod 755 $target_dir/bin/tool/disk_tool/*
}

function prepare_sys()
{
    mkdir -p $target_dir/bin/tool/sys_files
    /bin/cp $build_dir/utils/BuildScript/init_scripts/* $target_dir/bin/tool/sys_files
    chmod 755 $target_dir/bin/tool/sys_files/sofa_service
}


#parameter 1: platform: linux or ubuntu
#parameter 2: version
#parameter 3: release
#parameter 4: target directory
function gen_packages()
{
    cd utils/BuildScript

    if [ "$1" == "linux" ]; then
        sh Generate_sofa_RPM.sh $2 $3 $4
        /bin/rm -rf $build_dir/packages/SOFA-*.rpm
        /bin/mv SOFA-*.rpm $build_dir/packages/
    else
        sh Generate_deb.sh $2 $3 $4
    fi

    cd ../DeployScript
    /bin/cp deploy_sofa.sh $build_dir/packages/
    /bin/cp undeploy_sofa.sh $build_dir/packages/
    cd $build_dir
}

function verify_para()
{
    if [ "$1" != "linux" ] && [ "$1" != "ubuntu" ]
    then
        echo "gen_sofa_package.sh: wrong platform parameter $1"
        return 1
    fi

    if [ "$1" == "ubuntu" ]
    then
        echo "gen_sofa_package.sh not support platform build now $1"
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
echo "$0 start to generate sofa release packages"


env_prepare
prepare_bin
prepare_cfg
prepare_tool
prepare_sys


build_src $target_dir
retval=$?
if [ "$retval" == 1 ]
then
    echo "$0 build sofa and related compnent fail"
    cd $build_dir
    exit 1
fi

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


