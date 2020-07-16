#!/bin/bash
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

