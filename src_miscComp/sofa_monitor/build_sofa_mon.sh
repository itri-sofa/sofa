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

build_dir=`pwd`

export LANG=en_US
#export JAVA_HOME=/usr/java/jdk1.7.0_79
#export PATH=$PATH:/usr/java/jdk1.7.0_79/bin

#export ANT_HOME=/usr/apache-ant-1.9.7
#export PATH=$PATH:/usr/apache-ant-1.9.7/bin

function build_sofa_UI()
{
    cd $build_dir/SOFAUI
    ant -Dlibs.CopyLibs.classpath=lib/org-netbeans-modules-java-j2seproject-copylibstask.jar
    cd $build_dir
}

function build_sofa_mon()
{
    cd $build_dir/SOFAMonitor
    ant -Dlibs.CopyLibs.classpath=lib/org-netbeans-modules-java-j2seproject-copylibstask.jar
    cd $build_dir
}

inst_dir="/usr/sofaMon/"

echo "Start to build SOFA UI package"
build_sofa_UI

echo "Start to build SOFA monitor package"
build_sofa_mon

if [ $# -eq 1 ]; then
    if [ $1 == "install" ]; then
        cp $build_dir/SOFAUI/dist/SOFAUI.war $inst_dir
        cp $build_dir/SOFAMonitor/dist/SOFAMonitor.war $inst_dir
    fi
elif [ $# -eq 2 ]; then
    if [ $1 == "install" ]; then
        cp $build_dir/SOFAUI/dist/SOFAUI.war $2
        cp $build_dir/SOFAMonitor/dist/SOFAMonitor.war $2
    fi
fi

