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

sc_name="check_sofa_service.sh"

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
        echo "$sc_name: sofa service is running"
    else
        if [ "$retval1" == 0 ] && [ "$retval2" == 0 ] && [ "$retval3" == 0 ]; then
            echo "$sc_name: sofa service not running"
        else
            echo "$sc_name: sofa service not healthy"
            if [ "$retval1" == 0 ]; then
                echo "$scname: kernel module lfsmdr not exist"
            fi

            if [ "$retval2" == 0 ]; then
                echo "$scname: kernel module sofa not exist"
            fi

            if [ "$retval3" == 0 ]; then
                echo "$scname: user daemon sofa_daemon not exist"
            fi
        fi
    fi
}

check_sofa_service
