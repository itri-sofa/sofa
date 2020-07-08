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

##following kernel coding style
##/usr/src/kernels/${kernel version}/scripts/Lindent

yum -y install indent

remove_ctrl_m(){

### cat -v fileName  ==> know ^M
## grep -l --binary -P '\x0d' *
## grep -r $'\r' *
##
## to remove all \r in a file.
## tr -d $'\r' < filename
## tr -d '\r' <yourfile >newfile
##
##If use GNU sed, -i can perform in-place edit, so you won't need to write back:
## sed $'s/\r//' -i filename


    files=(`grep -l --binary -P '\x0d' *`)

    for i in "${files[@]}"
    do
       echo "Remove \r in file=$i" 
       tr -d '\r' < $i > "${i}_new"
       rm -f $i
       mv ./"${i}_new"  ./"${i}"
    done    
}

bfs_traverse(){

    remove_ctrl_m

    indent -npro -kr -i4 -ts4 -nut -sob -l80 -ss -ncs -cp1 *.c *.h 
    ## we skip *.h , because some macro will be parsed wrongly.
    ## indent -npro -kr -i8 -ts8 -sob -l80 -ss -ncs -cp1 *.c   *.h
    echo "indent alignmnet in the folder:$PWD"
    #folders=(`ls -d */`)
    folders=(`find . -maxdepth 1  -type d`)

    for i in "${folders[@]}"
    do       
       if [ "$i" = "." ]; then
            ##echo "skip folder=$i "
            continue
       fi 
       cd $i
       bfs_traverse
    done
    cd ..
}

bfs_traverse

echo "indent alignment with kernel format done!!!!"

exit 0







