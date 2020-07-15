/*
 * Copyright (c) 2015-2020 Industrial Technology Research Institute.
 *
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 *
 *
 *
 *
 *
 *
 *  
 */
#define _GNU_SOURCE  
#include <stdio.h> 
#include <unistd.h> 
#include <stdint.h> 
#include <string.h> 
#include <malloc.h> 
#include <errno.h> 
#include <sys/stat.h> 
#include <sys/ioctl.h> 
#include <fcntl.h> 



#define BLKDISCARD _IO(0x12,119) 
#define OFFSET (0) 
#define BUF_SIZE (512<<10)   // 512K 

char data[]="0123456789abcaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa2aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa8aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaabbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb8888bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc5cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccddddddddddddddddddddddddddddddddddddddddddddd7ddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddd5ddddddddddddddddddddddddddddddddddddddeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee1eeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee9eeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee8eeeeeeeffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff7fffffffffffffffffffffffffffffffffffffffffffffffggggggg6gggggggggggggggggggggggggggggggggghhhhhhhhhhhhhh5hhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii4jjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkk3#includejkl;;;jklkladfsjkl;dfasiodfsioeqrwkjl;reqeicvxj;klcvxzjklfasisakjlreqwiew000000000000000000asdfkjk;ljcxvidsafiorewqk;l-----------------------dsfakjdfjkasaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaabbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbvvvvvvvvvvvvvvvvvvvvvvvvvvddddddddddddddddddddddwewqqqqqqqqqqqqqqqqqqqqqqqqqqqqjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllll,,,,,,,,,,,,,,,,,,,,,,,,,mmmmmmmmmmmmmmmmmmmmmmmmooooooooooooooooooooooooooooooooooopppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppfffffffffffffffffffffffffff111111111111111111111111111111111111333333333333333333333333333333333333354fffffffffffffffffffyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa2aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa8aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaabbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb8888bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc5cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccddddddddddddddddddddddddddddddddddddddddddddd7ddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddd5ddddddddddddddddddddddddddddddddddddddeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee1eeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee9eeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee8eeeeeeeffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff7fffffffffffffffffffffffffffffffffffffffffffffffggggggg6gggggggggggggggggggggggggggggggggghhhhhhhhhhhhhh5hhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii4jjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkk3#includejkl;;;jklkladfsjkl;dfasiodfsioeqrwkjl;reqeicvxj;klcvxzjklfasisakjlreqwiew000000000000000000asdfkjk;ljcxvidsafiorewqk;l-----------------------dsfakjdfjkasaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaabbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbvvvvvvvvvvvvvvvvvvvvvvvvvvddddddddddddddddddddddwewqqqqqqqqqqqqqqqqqqqqqqqqqqqqjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllll,,,,,,,,,,,,,,,,,,,,,,,,,mmmmmmmmmmmmmmmmmmmmmmmmooooooooooooooooooooooooooooooooooopppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppfffffffffffffffffffffffffff111111111111111111111111111111111111333333333333abcz1234567890";


char data2[]="~!@#$%^&*(abcaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa2aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa8aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaabbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb8888bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc5cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccddddddddddddddddddddddddddddddddddddddddddddd7ddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddd5ddddddddddddddddddddddddddddddddddddddeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee1eeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee9eeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee8eeeeeeeffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff7fffffffffffffffffffffffffffffffffffffffffffffffggggggg6gggggggggggggggggggggggggggggggggghhhhhhhhhhhhhh5hhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii4jjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkk3#includejkl;;;jklkladfsjkl;dfasiodfsioeqrwkjl;reqeicvxj;klcvxzjklfasisakjlreqwiew000000000000000000asdfkjk;ljcxvidsafiorewqk;l-----------------------dsfakjdfjkasaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaabbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbvvvvvvvvvvvvvvvvvvvvvvvvvvddddddddddddddddddddddwewqqqqqqqqqqqqqqqqqqqqqqqqqqqqjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllll,,,,,,,,,,,,,,,,,,,,,,,,,mmmmmmmmmmmmmmmmmmmmmmmmooooooooooooooooooooooooooooooooooopppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppfffffffffffffffffffffffffff111111111111111111111111111111111111333333333333333333333333333333333333354fffffffffffffffffffyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa2aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa8aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaabbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb8888bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc5cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccddddddddddddddddddddddddddddddddddddddddddddd7ddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddd5ddddddddddddddddddddddddddddddddddddddeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee1eeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee9eeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee8eeeeeeeffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff7fffffffffffffffffffffffffffffffffffffffffffffffggggggg6gggggggggggggggggggggggggggggggggghhhhhhhhhhhhhh5hhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii4jjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkk3#includejkl;;;jklkladfsjkl;dfasiodfsioeqrwkjl;reqeicvxj;klcvxzjklfasisakjlreqwiew000000000000000000asdfkjk;ljcxvidsafiorewqk;l-----------------------dsfakjdfjkasaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaabbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbvvvvvvvvvvvvvvvvvvvvvvvvvvddddddddddddddddddddddwewqqqqqqqqqqqqqqqqqqqqqqqqqqqqjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllll,,,,,,,,,,,,,,,,,,,,,,,,,mmmmmmmmmmmmmmmmmmmmmmmmooooooooooooooooooooooooooooooooooopppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppfffffffffffffffffffffffffff111111111111111111111111111111111111333333333333acba(*&^%$#@!~";


static int trimAction(int fd, uint64_t start, uint64_t len) { 
//    printf("###### trim trim start ###################### \n");
	uint64_t r[2]; 
	r[0] = start; 
	r[1] = len; 
	int ret= ioctl(fd, BLKDISCARD, &r); 
         
//    printf("###### trim trim end: %d  ###################### \n", ret);
    return ret;

}

char* dev_name = "/dev/nvme0n1"; 
//char* dev_name = "/dev/sdd"; 


void showFormat(){
    printf("\n");
    printf("########################################### [format] ################################################\n");
    printf("(1)Trim/Write/Read all data                                 :      trim_tool.out  t/w/r  all  device1  device2  ...\n");
    printf("                                                          ex:      trim_tool.out  t  all  /dev/nvme0n1  /dev/nvme1n1  /dev/sdk2\n");
    printf("\n");
    printf("(2)Trim/Write/Read partial data                             :      trim_tool.out  t/w/r  partial  startIdx len device1  device2  ...\n");
    printf("       For trim use  512B-multiple for startIdx and len.  ex:      trim_tool.out  t  partial 0 512  /dev/nvme1n1  /dev/sdk3 \n");
    printf("       For w/r  use 1024B-multiple for startIdx and len.  ex:      trim_tool.out  w  partial 0 4096 /dev/nvme1n1  /dev/sdk3 \n");
    printf("\n");
}


int checkNumber(char * s){
  int ret =0;
  int i=0;

  int len= strlen(s);
  for(;i<len;i++){
    if (s[i] < '0' ||  s[i] >'9'){
      printf("[ERR] String involes non-number %c in %s\n", s[i], s);
      return -1;
    } 
  }

  return ret;
}

unsigned long trim_16M= 16777216 ;
unsigned long trim_32M= 33554432 ;
unsigned long trim_64M= 67108864 ;


int doTrim(int range, long long unsigned sIdx , long long unsigned len,  int devLen ,  char devices[20][40] ){
   int ret=0;
  // printf("range %d , sIdx %llu, len %llu, devLen %d\n\n", range, sIdx, len, devLen);
   int i =0;
   int fd;
   off_t size;
   unsigned long trimBlock=trim_64M;
  

   long long unsigned idx;
   unsigned long  trimSize=0;
   long long unsigned count=0;
   unsigned int rate=0;
   idx = sIdx;
   len = idx+len;

   for( ; i< devLen ; i++){
      fd = open(devices[i] , O_RDWR | O_DIRECT);
      size = lseek(fd,0, SEEK_END);
      lseek(fd, 0, SEEK_SET);   
       
      if ( range ==1 || ( len > (long long unsigned)size)  ){
         len = (long long unsigned ) size;
      } 
   
      printf("%d -- %s  ,startingIdx: %llu trim: %llu bytes in capacity: %llu bytes\n", i, devices[i], sIdx, len, (long long unsigned)size);
      
      count=1;
      idx =sIdx;
      printf("================= Trim %s ================\n",devices[i]);

      if (idx >= len){
           printf("[ERR] Trim failed because starting idx= %llu >= capacity= %llu bytes \n", idx, len );
           close(fd);
           return -1;
      }

      for( ; idx <len;idx +=trimBlock){
         if ( idx+trimBlock > len){
              trimSize= len -idx;
         }else{
              trimSize= trimBlock;
         } 
 
        if( trimAction(fd, idx, trimSize) <0 ){
            printf("[ERR] no-%d  %s trim failed from idx= %llu with trim len= %d bytes\n",count, devices[i], idx, trimSize );
            close(fd);
            return -1;
        }else{
            rate = ((float) (idx+trimSize) / len) *100;
            printf("%d%% finished!!!  no-%d %s trim done from idx= %llu with trim len= %d bytes.",rate , count, devices[i], idx, trimSize );
        }
        fflush(stdout);
        printf("\r");
        count++;
      }

      close(fd);       
      printf("\n\n");
   }

   
   return ret;
}


int doWriteReadAction(int action, int range, long long unsigned sIdx , long long unsigned len,  int devLen ,  char devices[20][40] ){
   int ret=0;
  // printf("range %d , sIdx %llu, len %llu, devLen %d\n\n", range, sIdx, len, devLen);
   int i =0;
   int fd;
   off_t size;
   int wrBlock=4096;

   long long unsigned idx;
   int wrSize=0;
   long long unsigned count=0;
   long long unsigned rate=0;
  
   void* buf = NULL;
   buf = memalign(4096 , BUF_SIZE);
   memset(buf, 0 , BUF_SIZE);

   idx = sIdx;
   len = idx+len;

   /*
   if (action ==1 && range == 1){
      len = trim_32M;
   }
   */

   int ioLen=0;
   int total=0;
   for( ; i< devLen ; i++){
      fd = open(devices[i] , O_RDWR | O_DIRECT);
      size = lseek(fd,0, SEEK_END);
      lseek(fd, 0, SEEK_SET);

      if ( range ==1 || ( len > (long long unsigned)size)  ){
         len = (long long unsigned ) size;
      }    
      printf("%d -- %s  ,startingIdx: %llu %s: %llu bytes in capacity: %llu bytes\n", i, devices[i], sIdx, ( (action ==1) ? "write" : "read"  ) ,len, (long long unsigned)size);
      
      count=1;
      idx =sIdx;
      total = len -idx;
      printf("================= %s %s ================\n", ( (action ==1) ? "write" : "read" ) , devices[i]);
      if (idx >= len){
           printf("[ERR] %s failed because starting idx= %llu >= capacity= %llu bytes \n",( (action ==1 ) ? "write" : "read" ) , idx, len );
           close(fd);
           return -1;
      }

      for( ; idx <len;idx +=wrBlock){
         if ( idx+wrBlock > len){
              wrSize= len -idx;
         }else{
              wrSize= wrBlock;
         } 


         if (action ==1 ){  // Write 
             memset(buf, 0, BUF_SIZE);
             if ( (count %2) ==0){
               strcpy(buf,data);
               ioLen = pwrite(fd, buf, wrBlock, idx );  
             }else{
               strcpy(buf,data2);
               ioLen = pwrite(fd, buf, wrBlock, idx );  
             }                   
             if (ioLen <=0 ){              
                printf("[ERR] no-%d  %s write failed from idx= %llu with len= %d bytes , ioLen=%d \n",count, devices[i], idx, wrSize, ioLen );
                close(fd);
                return -1;
             }

         }else{ // Read
             
             memset(buf, 0 , BUF_SIZE);
             ioLen = pread(fd, buf, wrSize, idx);
         } 

 
        rate = ((float) (idx+ wrSize) / len) *100;

        if ( action ==1){
            printf("%d%% finished!!!  no-%d %s %s done from idx= %llu with iolen= %d bytes. data= ",rate , count, devices, (action ==1 )? "write" : "read" , idx, ioLen );
            if( (count % 2) ==  0){
               printf("%.12s ..... %.12s", data, (data+4084) ); 
            }else{
               printf("%.12s ..... %.12s", data2, (data2+4084) ); 
            }
        }else{
            if ( devLen ==1  && total ==4096  ){                
                 printf("%d%% finished!!!  no-%d %s %s done from idx= %llu with iolen= %d bytes.\n ",rate , count, devices[i], (action ==1 )? "write" : "read" , idx, ioLen );
                 printf("[ data ] : \n\n");
                 int i=0;
                 int s=-1;
                 int e=0;
                 printf("[");
                 for( ; i< ioLen;i++){ 
                     if (  ((char *)buf)[i] == '\0' ){
                       printf(" "); 
                     }else{
                       if (s ==-1){
                          s=i;
                       }
                       e=i;
                     }  
                     printf("%c", ((char*)buf)[i]);
                 }
                 printf("]");
                 printf("\n\n");
                 if ( s == -1) {
                 	printf("[ data ] ===>  sIdx=%d , eIdx=%d len=%d\n",-1,-1, 0 );
                 }else{ 
                 	printf("[ data ] ===>  sIdx=%d , eIdx=%d len=%d\n",s,e, (e-s+1) );
                 }
                 close(fd);
                 return 0;
            } 
           

            printf("%d%% finished!!!  no-%d %s %s done from idx= %llu with iolen= %d bytes. data= %.12s ..... ",rate , count, devices[i], (action ==1 )? "write" : "read" , idx, ioLen, buf );
            if (ioLen == 4096){
               printf("%.12s", (buf+4083) );
            }
        }

        fflush(stdout);
        printf("\r");
        count++;
      }

      close(fd);       
      printf("\n\n");
   }

   
   return ret;
}




int main( int argc, char *argv[]) { 


    int k=0;
    for (;k<argc;k++){
       printf("argv[%d]= %s\n", k, argv[k]);
    }
    printf("\n");

    if (argc < 3){
        printf("[ERR] Format error !!\n");
        showFormat();
        return 0;
    }


    int action=-1;
    if( !strcmp(argv[1] , "t") ){
        action =0; 
    }else if( !strcmp(argv[1] , "w") ){
        action =1;
    }else if( !strcmp(argv[1] , "r") ){
        action =2; 
    }else{
       printf("[ERR] Please assign t/w/r but yours : %s\n", argv[1]);
       showFormat();
       return 0; 
    }

    printf("1. action=%d %s\n",action, argv[1] );
    int range=-1;

     if( !strcmp(argv[2] , "partial") ){
         range =0;
     }else if( !strcmp(argv[2] , "all") ){
         range =1;
     }else{
       printf("[ERR] Please assign all/partial but yours : %s\n", argv[2]);
       showFormat();
       return 0; 
     }
   

     printf("2. range=%d %s\n",range, argv[2] );

     long long unsigned sIdx=0;
     long long unsigned len=0;
     k=3; 
     if (range==0){
        if ( checkNumber(argv[3]) <0 ){
           printf("[ERR] Please assign startIdx with number only. %s \n", argv[3]);
           showFormat(); 
           return 0;
        }

        if ( checkNumber(argv[4]) <0 ){
           printf("[ERR] Please assign len with number only. %s \n", argv[4]);
           showFormat(); 
           return 0;
        }

        sscanf(argv[3], "%llu", &sIdx);
        sscanf(argv[4], "%llu", &len);         

        printf("3. sIdx=%llu , len=%llu \n",sIdx, len );
        k=5;
     }


     if (action ==0){
        if ( (sIdx % 512) != 0 ){
           printf("[ERR] For trim, please assign 512-multiple (512B) on startIdx like 0, 512, 1024...ect ERR: %llu\n",sIdx );
           showFormat();
           return 0;
        } 
        if ( (len % 512) != 0 ){
           printf("[ERR] For trim, please assign 512-multiple (512B) on len like 512, 1024...ect ERR: %llu\n",len );
           showFormat();
           return 0;
        } 
     }else{
        if ( (sIdx % 4096) != 0 ){
           printf("[ERR] For W/R, please assign 4096-multiple (4k) on startIdx like 0, 4096, 8192...ect ERR: %llu\n",sIdx );
           showFormat();
           return 0;
        } 
        if ( (len % 4096) != 0 ){
           printf("[ERR] For W/R, please assign 4096-multiple (4K) on len like 4096, 8192...ect ERR: %llu\n",len );
           showFormat();
           return 0;
        } 
     }

     char devices[50][40];
     int devLen = argc -k;
     int idx=0;
     int fd;
     for(;k<argc;k++){        
        fd = open(argv[k], O_RDWR | O_DIRECT);
        if (fd < 0 ){
           printf("[ERR] Please give corrent device name like /dev/nvme1n1 or /dev/sdb. ERR= %s\n", argv[k] );
           showFormat();
           return 0;
        }
        close(fd);
        strcpy(devices[idx],argv[k]);
        printf("devices idx= %d  %s\n", idx, devices[idx]);
        idx++;
     } 
     
     printf("devLen=%d \n\n\n", devLen);

     if (devLen ==0 ){
        printf("[ERR] Please assign dev like /dev/nvme3n1. \n");
        showFormat();
        return 0;
     } 


     int ret=0;
     if (action ==0 ){
          ret = doTrim(range, sIdx, len , devLen, devices );
          if (!ret){
             printf("###### Trim successfully #######\n");
          }else{
             printf("###### Trim Failed #######\n");
          }
     }else if (action ==1){

          ret = doWriteReadAction(action, range, sIdx, len , devLen, devices );
          if (!ret){
             printf("###### Write successfully #######\n");
          }else{
             printf("###### Write Failed #######\n");
          }
     }else if (action ==2){
 
          ret = doWriteReadAction(action, range, sIdx, len , devLen, devices );
          if (!ret){
             printf("###### Read successfully #######\n");
          }else{
             printf("###### Read Failed #######\n");
          }
     } 


}
