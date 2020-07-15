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
sofa_dir="/usr/sofa/"

admtool=$sofa_dir/bin/sofaadm
target="data"

lfsm_disk_list_str=""
num_groups=0

function get_group_disk()
{
    disk_list_str=`$admtool --lfsm --getgroupdisk $1`

    disk_list=($disk_list_str)
    
    startIndex=17
    indexJump=8
    for index in "${!disk_list[@]}"
    do
        if [ $startIndex -eq $index ]; then
            #echo "$index ${disk_list[index]}"
            mydisk=`echo ${disk_list[index]} | tr -d \, | tr -d \"`
            #echo "$index $mydisk"
            lfsm_disk_list_str="$lfsm_disk_list_str $mydisk"
            #echo "$index $lfsm_disk_list"
            startIndex=$(($startIndex + $indexJump))
        fi
    done
}

function get_group()
{
    group_list_str=`$admtool --lfsm --getgroup`

    group_list=($group_list_str)
 
    num_groups=${group_list[6]}
}

function get_all_datadisk()
{
    get_group
    
    for (( i=0; i<$num_groups; i=i+1 ))
    do
        get_group_disk $i
    done
}

function get_spare_disk()
{
    #disk_list_str=`$admtool --lfsm --getgroupdisk 0`
    disk_list_str=`$admtool --lfsm --getsparedisk`

    disk_list=($disk_list_str)

    startIndex=9
    indexJump=8
    for index in "${!disk_list[@]}"
    do
        if [ $startIndex -eq $index ]; then
            #echo "$index ${disk_list[index]}"
            mydisk=`echo ${disk_list[index]} | tr -d \, | tr -d \"`
            #echo "$index $mydisk"
            lfsm_disk_list_str="$lfsm_disk_list_str $mydisk"
            #echo "$index $lfsm_disk_list"
            startIndex=$(($startIndex + $indexJump))
        fi
    done
}

if [ $# -le 1 ]; then
    if [ "$1" == "spare" ]; then
        target=$1
    fi
else
    exit 1
fi

if [ "$target" == "data" ]; then
    get_all_datadisk
else
    get_spare_disk
fi

echo "$lfsm_disk_list_str"
