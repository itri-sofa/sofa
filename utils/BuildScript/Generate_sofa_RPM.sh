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

sofa_rel_dir=$dir/../../
version=1.0.00
release=1


function modify_rpm_spec()
{
    specfile=$1
    version=$2
    release=$3
    source=$4
    sed -i "s/Version:.*/Version:   $version/g" $specfile
    sed -i "s/Release:.*/Release:   $release/g" $specfile
    sed -i "s/Source:.*/Source:    $source/g" $specfile
}

function pack_release_file()
{
    rm -rf $dir/SOFA_Release.tar.gz
    cd $sofa_rel_dir
    tar -zcvf SOFA_Release.tar.gz SOFA_Release/

    mv SOFA_Release.tar.gz $dir/
    cd $dir
}

function prepare_rpm()
{
    mkdir -p $dir/SOFA-$version
    /bin/cp SOFA_Release.tar.gz $dir/SOFA-$version/
    tar -zcvf SOFA-$version.tar.gz SOFA-$version/
    rm -rf SOFA_Release.tar.gz
    rm -rf SOFA-$version

    modify_rpm_spec $dir/SPEC_for_Generate_RPM_Package/sofa.spec $version $release SOFA-$version.tar.gz
}

function build_rpm()
{
    mkdir -p /root/rpmbuild/SOURCES/
    mkdir -p /root/rpmbuild/SPECS/

    rm -rf /root/rpmbuild/SPECS/sofa.spec
    rm -rf /root/rpmbuild/SOURCES/SOFA-*.tar.gz

    /bin/cp ${dir}/SPEC_for_Generate_RPM_Package/sofa.spec /root/rpmbuild/SPECS/
    /bin/mv SOFA-*.tar.gz /root/rpmbuild/SOURCES/
    rpmbuild -bb /root/rpmbuild/SPECS/sofa.spec
    /bin/mv /root/rpmbuild/RPMS/x86_64/SOFA-*.rpm ${dir}/

    rm -rf /root/rpmbuild/BUILD/SOFA-*
    rm -rf /root/rpmbuild/SPECS/sofa.spec
    rm -rf /root/rpmbuild/SOURCES/SOFA-*.tar.gz
}



## Peter add ##
function build_rpm_SUSE()
{
    echo "peter ###### build_rpm pwd= $PWD , dir= ${dir}"
    mkdir -p /usr/src/packages/SOURCES/
    mkdir -p /usr/src/packages/SPECS/

    rm -f /usr/src/packages/SPECS/sofa.spec
    rm -f /usr/src/packages/SOURCES/SOFA-*.tar.gz

    /bin/cp ${dir}/SPEC_for_Generate_RPM_Package/sofa.spec /usr/src/packages/SPECS/
    /bin/mv SOFA-*.tar.gz /usr/src/packages/SOURCES/

    rpmbuild -bb /usr/src/packages/SPECS/sofa.spec
    /bin/mv /usr/src/packages/RPMS/x86_64/SOFA-*.rpm ${dir}/

    rm -rf /usr/src/packages/BUILD/SOFA-*
    rm -rf /usr/src/packages/SPECS/sofa.spec
    rm -rf /usr/src/packages/SOURCES/SOFA-*.tar.gz


}

## Peter_add ##




function show_usage()
{
    echo "Usage: $0 [version] $1 [release] $2 [release dir] $3"
}

if [ $# -le 1 ]; then
    if [ $# -eq 1 ]; then
        version=$1
    fi
elif [ $# -eq 2 ]; then
    version=$1
    release=$2
elif [ $# -eq 3 ]; then
    version=$1
    release=$2
    sofa_rel_dir=$3
else
    show_usage
    exit 1
fi

echo "$0 version=$version release=$release release_dir=$sofa_rel_dir"

pack_release_file

prepare_rpm

## Peter marked ##
## build_rpm
## Peter marked ##

## Peter add ##
## name:  Ubuntu ,   CentOS Linux,   SLES
echo "#### OS name= $(. /etc/os-release; echo $NAME) ## "
if [ "$(. /etc/os-release; echo $NAME)" = "SLES" ]; then
   echo "## build rpm for SUSE"
   build_rpm_SUSE
else
   echo "## build rpm for centOs & fedora"
   build_rpm
fi
## Peter add ##



echo "RPM Preparation Done!"

