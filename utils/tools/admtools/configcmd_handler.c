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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <stdint.h>
#include <syslog.h>
#include <stdbool.h>
#include "user_cmd.h"
#include "sofa_errno.h"

static int32_t  write_cfgitem (FILE *fp_out, int8_t *content, 
        int8_t *cfgName, int8_t *cfgVal, int8_t *cfg_set) {
    int8_t elname[64];
    int8_t elvalue[64];
    int8_t elset[64];

    int8_t *el_s_name = "<name>";
    int8_t *el_e_name = "</name>";
    int8_t *el_s_value = "<value>";
    int8_t *el_e_value = "</value>";
    int8_t *el_s_set = "<setting>";
    int8_t *el_e_set = "</setting>";
    int8_t *name_el_start = NULL;
    int8_t *name_el_end = NULL;
    int8_t *value_el_start = NULL;
    int8_t *value_el_end = NULL;
    int8_t *set_el_start = NULL;
    int8_t *set_el_end = NULL;

    int32_t length = 0;
    int32_t i;

    name_el_start = strstr(content, el_s_name);
    name_el_end   = strstr(content, el_e_name);
    value_el_start = strstr(content, el_s_value);
    value_el_end   = strstr(content, el_e_value);
    set_el_start = strstr(content, el_s_set);
    set_el_end   = strstr(content, el_e_set);

    do {
        if (name_el_start == NULL) {
            break;
        }

        if (name_el_end == NULL) {
            break;
        }

        if (value_el_start == NULL) {
            break;
        }

        if (value_el_end == NULL) {
            break;
        }

        name_el_start = name_el_start + strlen(el_s_name);
        name_el_end = name_el_end - 1;
        strncpy(elname, name_el_start, name_el_end - name_el_start + 1);
        elname[name_el_end - name_el_start + 1] = '\0';

        value_el_start = value_el_start + strlen(el_s_value);
        value_el_end = value_el_end - 1;
        strncpy(elvalue, value_el_start, value_el_end - value_el_start + 1);
        elvalue[value_el_end - value_el_start + 1] = '\0';

        if (strncmp(name_el_start, cfgName, name_el_end - name_el_start + 1) == 0) {
            if (set_el_start == NULL || set_el_end == NULL || cfg_set == NULL) {
                fprintf(fp_out, "%s<property>\n%s<name>%s</name>\n%s<value>%s</value>%s\n",
                        "    ", "        ", cfgName, "        ",
                        cfgVal, value_el_end + strlen(el_e_value) + 1);
            } else {
                fprintf(fp_out,
                        "%s<property>\n%s<name>%s</name>\n%s<value>%s</value>\n"
                        "%s<setting>%s</setting>\n%s\n",
                        "    ", "        ", cfgName, "        ",
                        cfgVal, "        ", cfg_set,
                        set_el_end + strlen(el_e_set) + 1);
            }
            return 1 ;
        } else {
            fprintf(fp_out, "%s<property>\n%s<name>%s</name>\n%s<value>%s</value>%s\n",
                    "    ", "        ", elname, "        ",
                    elvalue, value_el_end + strlen(el_e_value) + 1);
        }
    } while (0);
    return 0;
}

static int32_t replace_cfgVal (int8_t *file_in, int8_t *file_out,
        int8_t *cfg_name, int8_t *cfg_val, int8_t *cfg_set, bool is_append ) {
#define ch_arr_size 1024
    int8_t ch_array[ch_arr_size];
    int8_t *el_s_property = "<property>";
    int8_t *el_e_property = "</property>";
    int8_t ch;
    int32_t index = -1, ret;
    int32_t state = 0;
    FILE *fp_in, *fp_out;
    int32_t is_find = 0 ;

    fp_in = fopen(file_in, "r");
    if (fp_in == NULL) {
        return -ENOENT;
    }

    fp_out = fopen(file_out, "w+");

    memset(ch_array, '\0', sizeof(int8_t)*ch_arr_size);

    ret = 0;

    while ((ch = fgetc(fp_in)) != EOF) {
        if (state==0 && ch == '<') {
            state = 1;
        }

        if (state > 0) {
            index++; //ERROR Handling
            if (index >= ch_arr_size) {
                ret = -1;
                break;
            }
            ch_array[index] = ch;
        }

        if (ch == '>') {
            ch_array[index+1] = '\0';
            if (state == 1 && (strstr(ch_array, el_s_property) != NULL)) {
                state = 2;
            } else if (state == 2 && (strstr(ch_array, el_e_property) != NULL)) {
                if (  write_cfgitem(fp_out, ch_array, cfg_name, cfg_val, cfg_set) ==1){  
                      is_find=1; 
                }
                state = 0;
                index = -1;
                memset(ch_array, '\0', sizeof(int8_t)*ch_arr_size);
            } else if(state == 1) {
                if ( is_append &&  !is_find  && strstr( ch_array , "</configuration>") != NULL){
                    if ( cfg_set  == NULL ){
                        fprintf(fp_out, "%s<property>\n%s<name>%s</name>\n%s<value>%s</value>\n%s</property>\n",
                        "    ", "        ", cfg_name, "        ",
                        cfg_val, "    " );
                    }else{
                        fprintf(fp_out, "%s<property>\n%s<name>%s</name>\n%s<value>%s</value>\n" 
                        "%s<setting>%s</setting>\n%s</property>\n",
                        "    ", "        ", cfg_name, "        ",
                        cfg_val, "        ", cfg_set,
                        "    ");
                    }
                }

                fprintf(fp_out, "%s\n", ch_array);
                index = -1;
                state = 0;
                memset(ch_array, '\0', sizeof(int8_t)*ch_arr_size);
            }
        }
    }

    fclose(fp_in);
    fclose(fp_out);

    return ret;
}

int32_t process_cfgcmd (user_cmd_t *ucmd, bool is_append)
{
    cfg_ufile_cmd_t *mycmd;
    int8_t syscmd[256];
    int8_t *tmp_fn;
    int32_t ret, tmp_fn_len;

    ret = 0;
    mycmd = (cfg_ufile_cmd_t *)ucmd;

    tmp_fn_len = strlen(mycmd->cfgfile) + 8;
    tmp_fn = (int8_t *)malloc(sizeof(int8_t)*tmp_fn_len);
    memset(tmp_fn, '\0', tmp_fn_len);
    sprintf(tmp_fn, "%s%s", mycmd->cfgfile, ".tmp");

    if (replace_cfgVal(mycmd->cfgfile, tmp_fn, mycmd->itemName, mycmd->itemVal,
            mycmd->itemSet , is_append )) {
        ucmd->result = -1;
    } else {
        ucmd->result = 0;
        memset(syscmd, '\0', 256);
        sprintf(syscmd, "/bin/cp -f %s %s", tmp_fn, mycmd->cfgfile);
        system(syscmd);

        memset(syscmd, '\0', 256);
        sprintf(syscmd, "/bin/rm -f %s", tmp_fn);
        system(syscmd);
    }

    if (mycmd->itemSet == NULL) {
        syslog(LOG_INFO, "[SOFA ADM] config cmd %d %d %s %s %s\n",
                mycmd->cmd.opcode, mycmd->cmd.subopcode, mycmd->cfgfile,
                mycmd->itemName, mycmd->itemVal);
    } else {
        syslog(LOG_INFO, "[SOFA ADM] config cmd %d %d %s %s %s %s\n",
                mycmd->cmd.opcode, mycmd->cmd.subopcode, mycmd->cfgfile,
                mycmd->itemName, mycmd->itemVal, mycmd->itemSet);
    }

    return ret;
}

void free_cfgcmd_resp (user_cmd_t *ucmd)
{
    cfg_ufile_cmd_t *mycmd;

    mycmd = (cfg_ufile_cmd_t *)ucmd;
    free(mycmd);
}

void show_cfgcmd_result (user_cmd_t *ucmd)
{
    cfg_ufile_cmd_t *mycmd;

    mycmd = (cfg_ufile_cmd_t *)ucmd;

    if (ucmd->result) {
        if (mycmd->itemSet == NULL) {
            printf("SOFA TOOL update config file %s %s=%s fail ret %d\n",
                    mycmd->cfgfile, mycmd->itemName, mycmd->itemVal,
                    ucmd->result);
        } else {
            printf("SOFA TOOL update config file %s %s=%s %s fail ret %d\n",
                    mycmd->cfgfile, mycmd->itemName, mycmd->itemVal,
                    mycmd->itemSet, ucmd->result);
        }
    } else {
        if (mycmd->itemSet == NULL) {
            printf("SOFA TOOL update config file %s %s=%s success\n",
                    mycmd->cfgfile, mycmd->itemName, mycmd->itemVal);
        } else {
            printf("SOFA TOOL update config file %s %s=%s %s success\n",
                    mycmd->cfgfile, mycmd->itemName, mycmd->itemVal,
                    mycmd->itemSet);
        }
    }

}
