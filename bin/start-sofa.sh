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

# Get exclusive sofa flock to prevent lunch sofa twice in short time
SOFA_FLOCK_FILE=/dev/shm/sofa_flock_file
flock -x $SOFA_FLOCK_FILE -c "sleep 30 &"

#Perform system setting for SOFA
#sysctl -w vm.dirty_background_ratio=0
#sysctl -w vm.dirty_ratio=5
#sysctl -w vm.dirty_writeback_centisecs=20
#sysctl -w vm.dirty_expire_centisecs=200
#sysctl -w vm.vfs_cache_pressure=4096
#sysctl -w vm.min_free_kbytes=131072

#TODO: only validate under some linux version (example: CentOS 7 or Fedora 20)
systemctl stop irqbalance
systemctl stop firewalld

cur_dir=`pwd`
sofa_dir="/usr/sofa/"
sofa_udata_dir="/usr/data/sofa/"

mkdir -p $sofa_udata_dir/config

CONFIG_FILE=`ls $sofa_udata_dir/config/* | grep sofa_config.xml`

if [ "$CONFIG_FILE" ]; then
    echo "Config file exist $CONFIG_FILE"
    cp -a $sofa_udata_dir/config/* $sofa_dir/config/
fi

cd $sofa_dir

DaemonState=`ps -ef | grep sofa_daemon | grep -v grep | awk -F' ' '{print $2}'`;

if test "$DaemonState" = ""; then
    chmod 777 sofa_daemon
    ./sofa_daemon -c "$CONFIG_FILE" $@ &
else
    echo "sofa_daemon is running"
    cd $cur_dir
    exit 0
fi

cd $cur_dir

#TODO bad design, we want to give damon some time to start and change sofa_config.xml when need
sleep 10

#Move to here is when sofa daemon start from fresh restart, it will change sofa_config.xml 
if [ "$CONFIG_FILE" == "" ]; then
    echo "Config file isn't exist, ready to copy config file"
    cp -a $sofa_dir/config/* $sofa_udata_dir/config/
fi
