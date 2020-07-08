#!/bin/sh
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



cp $1 iozone_gen_out
file_name=iozone_gen_out
#set -x

write_gnuplot_file() {
	echo \#test : $query
	case $query in
  		(write) awk '$1 ~ /^[0-9]+/ { print  $1 " " $2 " " $3  }' < $file_name ;;
  		(rewrite)  awk '$1 ~ /^[0-9]+/ { print  $1 " " $2 " " $4  }' < $file_name ;;
  		(read)  awk '$1 ~ /^[0-9]+/ { print  $1 " " $2 " " $5  }' < $file_name ;;
  		(reread)  awk '$1 ~ /^[0-9]+/ { print  $1 " " $2 " " $6  }' < $file_name ;;
  		(randread)  awk '$1 ~ /^[0-9]+/ { print  $1 " " $2 " " $7  }' < $file_name ;;
  		(randwrite)  awk '$1 ~ /^[0-9]+/ { print  $1 " " $2 " " $8  }' < $file_name ;;
  		(bkwdread)  awk '$1 ~ /^[0-9]+/ { print  $1 " " $2 " " $9  }' < $file_name ;;
  		(recrewrite)  awk '$1 ~ /^[0-9]+/ { print  $1 " " $2 " " $10  }' < $file_name ;;
  		(strideread)  awk '$1 ~ /^[0-9]+/ { print  $1 " " $2 " " $11  }' < $file_name ;;
  		(fwrite)  awk '$1 ~ /^[0-9]+/ { print  $1 " " $2 " " $12  }' < $file_name ;;
  		(frewrite)  awk '$1 ~ /^[0-9]+/ { print  $1 " " $2 " " $13  }' < $file_name ;;
  		(fread)  awk '$1 ~ /^[0-9]+/ { print  $1 " " $2 " " $14  }' < $file_name ;;
  		(freread)  awk '$1 ~ /^[0-9]+/ { print  $1 " " $2 " " $15  }' < $file_name ;;
		(*)  echo "Usage : gengnuplot.sh <filename> <test>" >> /dev/stderr ; 
       		     echo "filename is the output of iozone -a" >> /dev/stderr ;
       		     echo "test is one of write rewrite read reread randread randwrite bkwdread recrewrite strideread fwrite frewrite fread freread" >> /dev/stderr ;;
	esac }

#filename=$1
filename=iozone_gen_out
query=$2
if (! [ -e $query ] ) ; then mkdir $query; fi
if ( [ $# -eq 2 ] ) ; 
	then 
		write_gnuplot_file > $query/`basename $file_name.gnuplot`
	else
		echo "Usage : gengnuplot.sh <filename> <test>" 2>&1
		echo "filename is the output of iozone -a" 2>&1
		echo "test is one of write rewrite read reread randread randwrite bkwdread recrewrite strideread fwrite frewrite fread freread" 2>&1
fi
