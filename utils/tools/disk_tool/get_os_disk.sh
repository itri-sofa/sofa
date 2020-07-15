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

#NOTE: for now we only support scsi device /dev/sdX

os_disk_str=`mount | grep " / "`

if test "$os_disk_str" = ""; then
    #if fail to find, we assume the os be installed at /dev/sda"
#    echo "sda"
    exit 1
fi

tk_arr=($os_disk_str)
prefix="/dev/"
os_disk=${tk_arr[0]#$prefix}

if [[ $os_disk == *mapper* ]]; then
    os_disk_str=`mount | grep "/boot "`
    
    if test "$os_disk_str" = ""; then
#        echo "sda"
        exit 1
    fi
    tk_arr=($os_disk_str)
    os_disk=${tk_arr[0]#$prefix}
fi

os_disk=$(printf '%s' "$os_disk" | sed 's/[0-9]//g')

if [[ $os_disk != sd* ]]; then
    #echo "match SCSI disk naming"
    exit 1
fi

if [[ -d /sys/block/${os_disk}/ ]]; then
    echo ${os_disk}
    exit 0
else
    #echo "sda"
    exit 1
fi

