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

version=1.0.00
release=1

OS_Type=`uname -a | grep Ubuntu`
if [ "$OS_Type" ]; then
    MODULE_NAME="SOFA-${version}-${release}_amd64.deb"
else
    MODULE_NAME="SOFA-${version}-${release}.x86_64.rpm"
fi
SERVICE_NAME="SOFA"

function show_usage()
{
    echo "Usage: deploy_sofa.sh [version] $version [release] $release"
}

if [ $# -le 1 ]; then
    if [ $# -eq 1 ]; then
        version=$1
    fi
elif [ $# -eq 2 ]; then
    version=$1
    release=$2
    release=$3
else
    show_usage
    exit 1
fi

echo "$0: deply SOFA version=$version release=$release"

if [ "$OS_Type" ]; then
	dpkg -i $MODULE_NAME
else
	rpm -ivh $MODULE_NAME
fi

ln -s /usr/sofa/bin/tool/sys_files/80-lfsm.rules /etc/udev/rules.d/80-lfsm.rules
udevadm control --reload-rules && udevadm trigger
if [ ! -f "/usr/data/sofa/config/sofa_config.xml" ]; then
    echo "Copy config file"
    mkdir -p /usr/data/sofa/config
    cp -a /usr/sofa/config/sofa_config.xml /usr/data/sofa/config/sofa_config.xml
fi
echo "$0: deploy SOFA done"

