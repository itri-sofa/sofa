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
build_type="normal"
storage_type="lfsm"
inst_dir="/usr/sofa/"
error_test="error_test.txt"       #<--
error_result="error_result.txt"   #<--
error=""

function error_message()
{
    echo "$1"
    echo "$1" >> $error_result
}

function setup_one_para()
{
    if [ $1 == "lfsm" ]; then
        storage_type=$1
        build_type="normal"
    elif [ $1 == "lfsm_fake_test" ]; then
        storage_type=$1
        build_type="normal"
    elif [ $1 == "normal" ]; then
        build_type=$1
        storage_type="lfsm"
    elif [ $1 == "clean" ]; then
        build_type="clean"
        storage_type="lfsm"
    elif [ $1 == "install" ]; then
        build_type="install"
        storage_type="lfsm"
    else
        echo "build_sofa.sh: wrong parameter: $1"
        exit 1
    fi
}

function setup_two_para()
{
    if [ $1 == "lfsm" ]; then
        storage_type=$1
    elif [ $1 == "lfsm_fake_test" ]; then
        storage_type=$1
    elif [ $1 == "install" ]; then
        storage_type="lfsm"
        build_type="install"
        inst_dir=$2
        return
    else
        ecoh "build_sofa.sh: wrong 1st parameter $1, expected lfsm or lfsm_fake_test"
        exit 1
    fi

    if [ $2 == "normal" ]; then
        build_type="normal"
    elif [ $2 == "install" ]; then
        build_type="install"
    else
        echo "build_sofa.sh: wrong 2nd parameter $2, expected normal or install"
        exit 1
    fi
}

function setup_three_para()
{
    if [ $1 == "lfsm" ]; then
        storage_type=$1
    elif [ $1 == "lfsm_fake_test" ]; then
        storage_type=$1
    else
        echo "build_sofa.sh: wrong 1st parameter $1, expected lfsm or lfsm_fake_test"
        exit 1
    fi

    if [ $2 == "install" ]; then
        build_type="install"
    else
        echo "build_sofa.sh: wrong 2nd parameter $2, expected install"
        exit 1
    fi

    inst_dir=$3
}

function clean_all()
{
    cd $build_dir/storage_core/$storage_type 
    make clean
    cd $build_dir   

    cd $build_dir/sofa_main/
    make clean
    cd $build_dir

    cd $build_dir/UserDaemon/
    make clean
    cd $build_dir
}

function build_all()
{
    cd $build_dir/storage_core/$storage_type
    make
    lfsm_build_test=$?
    echo "--------> lfsm build test :"${lfsm_build_test}
    cd $build_dir

    if [ "${lfsm_build_test}" == "0" ]; then
       echo -e "--------> lfsm "make" success"
    else                          
       echo -e "--------> /src/storage/lfsm "make" have errors in script !!!"
       echo -e "--------> Error message is in /usr/error_result "
       exit 1
    fi

    cp storage_core/$storage_type/Module.symvers ./sofa_main/

    cd $build_dir/sofa_main
    make
    sofa_main_build_test=$?
    echo "--------> sofa_main build test :"${sofa_main_build_test}
    cd $build_dir
    
    if [ "${sofa_main_build_test}" == "0" ]; then
        echo -e "--------> sofa_main "make" success"
    else
        echo -e "--------> /src/sofa_main "make" have errors in script !!!"
        echo -e "--------> Error message is in /usr/error_result "
        exit 1
    fi

    cd $build_dir/UserDaemon/
    make
    UserDaemon_build_test=$?
    echo "--------> UserDaemon build test :"${UserDaemon_build_test}
    cd $build_dir

    if [ "${UserDaemon_build_test}" == "0" ]; then
        echo -e "--------> UserDaemon "make" success"
    else
        echo -e "--------> /src/sofa_main "make" have errors in script !!!"
        echo -e "--------> Error message is in /usr/error_result"
        exit 1
    fi

}

function install_all()
{
    mkdir -p $inst_dir
    
    if [ "$storage_type" == "lfsm" ]; then
        rm -rf $inst_dir/lfsmdr.ko
        rm -rf $inst_dir/rfsioctl
        cp $build_dir/storage_core/$storage_type/lfsmdr.ko $inst_dir/
        cp $build_dir/storage_core/$storage_type/rfsioctl $inst_dir/
    else
        rm -rf $inst_dir/fake_lfsmdr.ko
        rm -rf $inst_dir/rfsioctl
        cp $build_dir/storage_core/$storage_type/fake_lfsmdr.ko $inst_dir/
        cp $build_dir/storage_core/$storage_type/rfsioctl $inst_dir/
    fi

    rm -rf $inst_dir/sofa.ko
    cp $build_dir/sofa_main/sofa.ko $inst_dir/

    rm -rf $inst_dir/sofa_daemon
    cp $build_dir/UserDaemon/sofa_daemon $inst_dir/
}

export LANG=en_US


if [ $# -eq 0 ]
then
    build_type="normal"
    storage_type="lfsm"
elif [ $# -eq 1 ]
then
    setup_one_para $1
elif [ $# -eq 2 ]
then
    setup_two_para $1 $2
elif [ $# -eq 3 ]
then
    setup_three_para $1 $2 $3
else
    echo "$0: wrong para list: $@"
    exit 1
fi

echo "build sofa storage with storage = $storage_type and build = $build_type with dir $inst_dir"

clean_all
if [ "$build_type" == "clean" ]; then
    echo "$0: clean all done"
    exit 0
fi

build_all
if [ "$build_type" == "normal" ]; then
    echo "$0: build all with storage type $storage_type done"
    exit 0
fi

install_all
echo "$0: build and install all with storage type $storage_type and install to $inst_dir done"

