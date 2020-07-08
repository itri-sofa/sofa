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

dir=`pwd`
sofa_dir="/usr/sofa/"

trimtool=$sofa_dir/bin/tool/disk_tool/trim.scp

if [ $# -eq 0 ]; then
    echo "Usage: $0 disk list <sdb sdc sdd sde ... >"
    exit 1
fi

mydisk_list=($@)

for index in "${!mydisk_list[@]}"
do
    echo "cmd: trim ${mydisk_list[index]}"
    sh $trimtool ${mydisk_list[index]} &
    #nohup $trimtool ${mydisk_list[index]} > /dev/null 2>&1 &
done

for (( i=1; i<=3600; i=i+1 ))
do
    num_lines=`ps aux | grep trim.scp | wc -l`
   
    if [ $num_lines -eq 1 ]; then
        cont=`ps aux | grep trim.scp`
        break
    fi
    
    if [ $i -eq 3600 ]; then
        echo "$0: trim may not finish, please check it manaually"
    fi
    sleep 1
done

