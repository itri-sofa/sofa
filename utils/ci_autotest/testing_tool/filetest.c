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

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <string.h>


#define SZ_SECTOR 512

int32_t randSize(int32_t seed_rd, int32_t sz_aveFile);
int32_t FindStr(FILE *f, int8_t *str);

int main(int32_t argc, int8_t *argv[])
{
    int8_t mode_rw[2] = "w"; //1 as write, 2 as read// atoi(argv[1])
    int32_t nm_totalFile = 5; //default 5 files
    int32_t i, id_fileCheck = 1; //default check index 1
    int32_t nm_fileName = 0;
    int32_t sz_eachFile, nm_sector, cnt_write, ck_error = 0;
    int32_t seed_rd; //input the seed from outside
    int32_t *pStuff; //array to store temp stuffing
    int32_t pos; //for checking deleted file in read function
    int32_t cnt_content; // for stuffing the checking for loop content
    double sz_std = 100; //standard deviation  (300~500kB)
    double u, v;
    double sz_aveFile = 400;  //default 400 kB
    int8_t *name_File;
    size_t result;

    name_File = malloc(1024);

    strcpy(mode_rw, argv[1]);
    if (strcmp (mode_rw, "d") == 0) {
        nm_fileName= atoi(argv[2]);
        sprintf(name_File, "./test_FileGenerate/F%05i.txt", nm_fileName);
        printf( "File name: %s\n", name_File);
        if (remove(name_File) != 0) {
            perror("Error deleting file\n");
        } else {
            printf("File successfully deleted\n");

            //==write deleted file into log
            FILE *pFile= fopen("./test_FileGenerate/deletedLog.log", "ab");
            if (pFile == NULL) {
                printf("Error file open for delte log!\n");
                exit(1);
            }
            fwrite(name_File , 1 , 160 , pFile);
        }
        return 0;
    }


    nm_totalFile= atoi(argv[2]);
    sz_aveFile= atoi(argv[3]);//kB
    seed_rd=  atoi(argv[4]); //input the seed from outside
    cnt_content=  atoi(argv[6]); //input the loop index from outside

    for (nm_fileName = 0; nm_fileName < nm_totalFile; nm_fileName++, seed_rd++) {
        sprintf(name_File, "%s/F%05i.txt", argv[5], nm_fileName);
        sz_eachFile = randSize(seed_rd,sz_aveFile);

        printf("sz_eachFile: %i; nm_fileName:%i \n", sz_eachFile, nm_fileName);
        //mode_rw=atoi(argv[1]); //BUG: after randSeed function, the mode will be changed
        printf("name_File: %s, mode_rw:%s \n", name_File, mode_rw);

        /* File opening */
        FILE *pFile= (strcmp (mode_rw,"w") == 0) ? fopen(name_File,"wb") : fopen(name_File,"rb");

        if (pFile == NULL) {
            if (strcmp (mode_rw,"w") == 0) {
                printf("Error file open! Already write till %s\n", name_File);
            }

            if (strcmp (mode_rw,"r") == 0) {
                //pos = FindStr(pFile, name_File);
                if (pos != -1) {
                    printf("File is delete, it's position is: %d\n", pos);
                } else {
                    printf("String is not found!\n");
                }
            }

            printf("Error file open! Already read till %s\n", name_File);
            exit(1);
        }

        /* Doing content stuffing */
        pStuff= (int32_t *)malloc(SZ_SECTOR);
        for (nm_sector=0; nm_sector*SZ_SECTOR< sz_eachFile; nm_sector++) {
            if (strcmp (mode_rw, "w") == 0) {
                for (cnt_write = 0; cnt_write < SZ_SECTOR/sizeof(int);
                        cnt_write = cnt_write + 4)  //cnt is only 1/4 of sizeSector
                {
                    pStuff[cnt_write]= nm_fileName;

                    if (cnt_write + 1 < SZ_SECTOR/sizeof(int)) {
                        pStuff[cnt_write + 1]= cnt_content; //nm_sector;

                        if (cnt_write + 2 < SZ_SECTOR/sizeof(int)) {
                            pStuff[cnt_write + 2]= nm_sector; //nm_sector;

                            if (cnt_write + 3 < SZ_SECTOR/sizeof(int)) {
                                pStuff[cnt_write + 3]= cnt_content; //nm_sector;
                            }
                        }
                    }
                }

                result = fwrite(pStuff, 1, SZ_SECTOR, pFile);
                if (result != SZ_SECTOR) {
                    printf ("Error writing! Already write till %s \n",name_File);
                    exit (3);
                }

                if (nm_sector== 2) {//sz_eachFile/SZ_SECTOR-2
                    printf("Debug:%i %i %i %i %i %i %i %i\n",
                            pStuff[0], pStuff[1], pStuff[2], pStuff[3],
                            pStuff[4], pStuff[5], pStuff[6], pStuff[7]);
                }
            }

            if (strcmp (mode_rw, "r") == 0) {
                fread (pStuff, 1, SZ_SECTOR, pFile );

                /* Correction checking*/
                if (nm_sector == 2 ) {//sz_eachFile/SZ_SECTOR-2
                    printf("Debug:%i %i %i %i %i %i %i %i\n",
                            pStuff[0], pStuff[1], pStuff[2], pStuff[3],
                            pStuff[4], pStuff[5], pStuff[6], pStuff[7]);
                }

                ck_error = 0;
                for (i = 0; i < SZ_SECTOR/sizeof(int32_t); i = i + 4) {//SZ_SECTOR /4
                    if (pStuff[i]!= nm_fileName) { //nm_sector
                        ck_error++;
                        if ((i+1) < SZ_SECTOR/sizeof(int32_t) &&
                                pStuff[i + 1] != cnt_content) {
                            ck_error++;
                            if ((i + 2) < SZ_SECTOR/sizeof(int32_t) &&
                                    pStuff[i + 2] != nm_sector) {
                                ck_error++;
                                if (i + 3 < SZ_SECTOR/sizeof(int32_t) &&
                                        pStuff[i + 3] != cnt_content) {
                                    ck_error++;
                                }
                            }
                        }
                    }
                }

                if (ck_error > 0) {
                    printf("error at file#%i sector#%i (content %i %i %i %i) \n",
                            nm_fileName, nm_sector,
                            pStuff[0], pStuff[1], pStuff[2], pStuff[3]);
                }
            }
        }

        free(pStuff);
        fclose(pFile);

        if (strcmp (mode_rw, "w") == 0) {
            printf("size of Stuff: %i =====> Write successfully ! \n", sz_eachFile);
        } else if (strcmp (mode_rw,"r") == 0) {
            if (ck_error==0) {
                printf("size of Stuff: %i =====> Read successfully ! \n", sz_eachFile);
            }
        }
    }

    return 0;
}

int32_t randSize (int32_t seed_rd, int32_t sz_aveFile)
{
    int32_t sz_std = sz_aveFile/4;

    srand(seed_rd);
    float u = rand() / (double)RAND_MAX;
    float v = rand() / (double)RAND_MAX;
    float sz_eachFile = (int32_t)sqrt(-2 * log(u)) * cos(2 * M_PI * v) * sz_std + sz_aveFile;
    sz_eachFile= sz_eachFile*1024; //to kB

    return sz_eachFile;
}

int32_t FindStr (FILE *f, int8_t *str)
{
    int32_t s_pos; //string position in the text
    int32_t c_pos; //char position in the text
    int8_t *string;
    int8_t ccnt; //char count

    s_pos = -1;
    c_pos = 0;
    string = (int8_t *) malloc(strlen(str)); /* doesn't allocate storage for terminating zero */
    while (!feof(f)) /* This loop always reads extra stuff. Not fatal in this case, but wrong programming practice */
    {
        if (c_pos == 0) {
            for (ccnt = 1; ccnt <= strlen(str); ccnt++) {
                if (!feof(f)) {
                    string[ccnt - 1] = getc(f);
                }
            }
        }

        /* note that string is not zero-terminated, so behavior of strcmp is undefined */
        if (c_pos != 0) {
            if (!feof(f)) {
                for (ccnt = 0; ccnt <= strlen(str) - 2; ccnt++) {
                    string[ccnt] = string[ccnt + 1];
                }
                string[strlen(str) - 1] = getc(f);
            }
        }

        if (strcmp(string, str) == 0) {
            s_pos = c_pos;
            break;
        }
        c_pos++;
    }
    return(s_pos);
}

/*
void dumphex (unsigned char *Buffer)
{
	int i,j,k,idx;
	for (k=0;k<(2*(1<<SZ_SECTOR));k++)
	{
		for(i=0;i<16;i++)
		{
			printf("%04hx :",(int)k*256+i*16);


			for(j=0;j<8;j++)
				printf("%02hX ",(int)Buffer[k*256+i*16+j]);


			printf("- ");

			for(j=0;j<8;j++)
				printf("%02hX ",(int)Buffer[k*256+i*16+j+8]);

			for(j=0;j<16;j++)
			{
				if (((int)Buffer[k*256+i*16+j]<32)||((int)Buffer[k*256+i*16+j]>=128)) printf(".");
				else printf("%c",Buffer[k*256+i*16+j]);
			}
			printf("\n");
		}

	}
	printf("\n");
}
*/
