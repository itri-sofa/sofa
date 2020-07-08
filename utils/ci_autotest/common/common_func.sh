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

# Parameter 1: message
# test_log_file defined in common/config_test_env.sh
function log_msg()
{
    echo "$1"
    echo "`date`: $1" >> ${test_log_file}
}

function log_msg_noconsole()
{
    echo "`date`: $1" >> ${test_log_file}
}

# Parameter 1: log file
# PATH_TEST defined in common/config_test_env.sh
function errorexit()
{
    log_msg $1
    cd $PATH_TEST
    exit 1
}

# Parameter 1: command user want to execute
function execute()
{
    log_msg "$test_scfn: [INFO] execute cmd: $1"
    result="`$1`"
    if [ $? -ne 0 ]; then
        errorexit "$test_scfn: [ERROR] Failed to execute \"$1\""
    fi
}

# Description: execute command at background
# Parameter 1: command user want to execute
function execute_bg()
{
    log_msg "$test_scfn: [INFO] execute cmd: $1"
    $1 &
    if [ $? -ne 0 ]; then
        errorexit "$test_scfn: [ERROR] Failed to execute \"$1\" at background"
    fi
}

# Description: execute command at background
# Parameter 1: command user want to execute
function execute_bg_result2file()
{
    log_msg "$test_scfn: [INFO] execute cmd: $1 > $2"
    $1 > $2 &
    if [ $? -ne 0 ]; then
        errorexit "$test_scfn: [ERROR] Failed to execute \"$1\" at background"
    fi
}

# TODO: Lego: the output of fdisk will vary in different linux distribution,
#             we should use an better way
# gDiskSize defined at common_global_var.sh
function get_disk_size()
{
    fdisk -l /dev/$1 > get_diskSize_fntmp 2> /dev/null
    disksize=`awk '{ if (substr($6,0,5)=="bytes") { print $5/4096 ;exit(1)}}' get_diskSize_fntmp`

    rm -rf get_diskSize_fntmp
    gDiskSize=$disksize
}

# gDisksNum defined at common_global_var.sh
# gDisksMap defined at common_global_var.sh
function get_disks_Num()
{
    IFS=$'\n' gDisksMap=($(awk 'NR!=1{if($3 >= 0) print $2=substr($2,6)}' /proc/lfsm/status))
    gDisksNum=${#gDisksMap[@]}
#    for((i=0;i<gDisksNum;i++))
#    do
#        echo "Disk[$i] = ${gDisksMap[$i]}"
#    done
}

# Description: simulate to plug disk 
# Parameter 1: scsi host number for the disk
function plug_disk()
{
    host=$1
    echo "- - -" >/sys/class/scsi_host/host$host/scan
    sleep 5
}

# Description: simulate to remove disk 
# Parameter 1: disk name
# return value
#    host: the scsi host number
function unplug_disk()
{
    disk=$1
    host=$(readlink /sys/block/$disk | awk -F 'host' '{print $2}' |cut -d'/' -f 1)
    echo "Erase data for disk $disk"
    hdparm --trim-sector-ranges 0:32768 32768:32768 --please-destroy-my-drive $disk
    echo "Unplug $disk"
    echo 1 > /sys/block/$disk/device/delete
    return $host
}

# Description: wait for rebuild finish after disk plug 
# return value
#    0: rebuild success
#    1: rebuild failure
function wait_rebuild_finish()
{
    timeout_cnt=0
    while((timeout_cnt<200))
    do
        dmesg -c | grep "exec rebuild all ret= 0"
        if(($?==0))
        then
            log_msg "lfsm rebuild ok"
            return 1
        fi

        timeout_cnt=$((timeout_cnt+1))
        sleep 30
    done
    log_msg "[time out] can not find \"exec rebuild all ret= 0\""
    return 0
}

# Description: wait for lfsm startup success 
# return value
#    0: startup success
#    1: startup failure
function wait_startup_finish()
{
    timeout_cnt=0
    while((timeout_cnt<200))
    do
        dmesg -c | grep "main INFO initial all sofa components done"
        if(($?==0))
        then
            log_msg "lfsm startup ok"
            return 1
        fi

        timeout_cnt=$((timeout_cnt+1))
        sleep 30
    done
    log_msg "[time out] can not find \"main INFO initial all sofa components done\""
    return 0
}
# return value
#    0: driver lfsmdr.ko not exist in kernel space
#    1: driver lfsmdr.ko exist in kernel space
function check_lfsm_drv()
{
    LFSM_KO=`lsmod | grep lfsmdr`

    if [ "$LFSM_KO" ]; then
        retval=1
    else
        retval=0
    fi

    return "$retval"
}

function check_sofa_drv()
{
    SOFA_KO=`lsmod | grep sofa`

    if [ "$SOFA_KO" ]; then
        retval=1
    else
        retval=0
    fi

    return "$retval"
}

function execute_bg_wait_all()
{
    for((i=0;i<$2;i++))
    do
        CmdRStat=`ps -ef | grep $1 | grep -v grep | awk -F' ' '{print $2}'`;
        if test "$CmdRStat" = ""; then
            return 0
        else
            sleep 1
        fi
    done
    
    CmdRStat=`ps -ef | grep $1 | grep -v grep | awk -F' ' '{print $2}'`;
    if test "$CmdRStat" = ""; then
        return 0
    fi
    
    return 1
}

function check_sofa_userD()
{
    UserDState=`ps -ef | grep sofa_daemon | grep -v grep | awk -F' ' '{print $2}'`;

    if test "$UserDState" = ""; then
        retval=0
    else
        retval=1
    fi

    return "$retval"
}

# return value
#    0: sofa service not ready
#    1: sofa service is running
function check_sofa_service()
{
    check_lfsm_drv
    retval1=$?

    check_sofa_drv
    retval2=$?

    check_sofa_userD
    retval3=$?

    if [ "$retval1" == 1 ] && [ "$retval2" == 1 ] && [ "$retval3" == 1 ]
    then
        return 1
    else
        return 0
    fi

    return 0
}

# PATH_SOFA defined in common/config_test_env.sh
# PATH_TEST defined in common/config_test_env.sh
function start_sofasrv()
{
    check_sofa_service
    retval=$?
    if [ "$retval" == 1 ]
    then
        log_msg "sofa service running, please stop it."
        cd $dir
        exit 1
    fi

    ${PATH_SOFA}/bin/start-sofa.sh > /dev/null 2>&1 &

    #TODO: using tool to check whether service is ready
    sleep 20

    check_sofa_service
    retval=$?
    if [ "$retval" == 0 ]
    then
        log_msg "fail start sofa service, exit."
        cd ${PATH_TEST}
        exit 1
    fi
}

function stop_sofasrv()
{
    ${PATH_SOFA}/bin/stop-sofa.sh > /dev/null 2>&1 &

    #TODO: using tool to check whether service is stop
    sleep 20

    check_sofa_service
    retval=$?
    if [ "$retval" == 1 ]
    then
        log_msg "fail to stop sofa service, exit"
        cd ${PATH_TEST}
        exit 1
    fi
}

function reset_sofasrv()
{
    ${PATH_SOFA}/bin/sofaadm --config --updatefile cfgfn=/usr/data/sofa/config/sofa_config.xml lfsm_reinstall 1
}

function uninitialize_sofasrv()
{
    ${PATH_SOFA}/bin/sofaadm --config --updatefile cfgfn=/usr/data/sofa/config/sofa_config.xml lfsm_reinstall 0
}

function check_correct_num()
{
   gCorrectNum=`awk 'BEGIN{count=0}{if($5=="type"){if($6==2) count+=1}}END{print count}' $1`
}

function get_run_times()
{
    r=0

    if [ -f "$file_run_times" ]
    then
        r=$(cat "$file_run_times")
        r=$((r+1))
    fi
    return $r
}

function set_run_times()
{
    echo $1 > "$file_run_times"
}

function get_test_arg()
{
    declare -n result="$1"
    file_test_arg="${PATH_LOG}/${test_scfn}".arg
    IFS=" " read -r -a result <<< "$(cat "$file_test_arg")"
}

function set_test_arg()
{
    echo "$1 $2" > "$file_test_arg"
}

function reset_cpu()
{
    echo 128 >/proc/sys/kernel/sysrq
    echo b > /proc/sysrq-trigger
}
