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

#########################
#    set parameters     #
#########################

test_scfn=`echo $(basename $0) | cut -f1 -d'.'`
PATH_TEST=`pwd`
PATH_SOFA=/usr/sofa/

PATH_TEST_TOOL=${PATH_SOFA}/bin/testing/ci_test/testing_tool
PATH_LOG=${PATH_SOFA}/bin/testing/ci_test/log
mkdir -p $PATH_LOG

test_tool=${PATH_TEST_TOOL}/ttest
test_log_file=${PATH_LOG}/${test_scfn}.log

file_run_times="${PATH_LOG}/${test_scfn}".tag
file_test_arg="${PATH_LOG}/${test_scfn}".arg

