/*
 * Copyright (c) 2015-2025 Industrial Technology Research Institute.
 *
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */
#ifndef SPEC_CMD_H
#define SPEC_CMD_H

#include "lfsm_core.h"
#include "batchread.h"

// must be consist with firmware code
#define LFSM_ERASE           1
#define LFSM_COPY            2
#define LFSM_MARK_BAD        3
#define LFSM_READ        10
#define LFSM_WRITE        11
#define LFSM_READ_BATCH_PRELOAD    4

#define ECC_0_4            0x01
#define ECC_4_24        0x02
#define ECC_UNRECOVERABLE    0x04

#define WR_ERROR        0x0100

#define RD_ERROR        0x0200
#define ERASE_ERROR        0x010000

#define MARK_BAD_ERROR        0x020000

#define LFSM_ERR_WR_ERROR         0x000100
#define LFSM_ERR_WR_RETRY         0x000200
#define LFSM_ERR_ERASE_ERROR      0x010000
#define LFSM_ERR_MARK_BAD_ERROR   0x020000

#define LFSM_NO_FAIL              0x000000
#define LFSM_READ_FAIL            0x000101

#define         ADDR_MODE        47 // weafon

#define         MAPPING_TO_HDD_ORDER  48    // AN
#define         MAPPING_TO_HDD_BIT   ((unsigned long long)1<<MAPPING_TO_HDD_ORDER)
#define         IS_HDD_LBA(X) ((X&MAPPING_TO_HDD_BIT)>0)
#define         LFSM_SPECIAL_CMD    ((unsigned long long)1<<ADDR_MODE)
#define         LFSM_BOOT_QUERY        ((unsigned long long)1<<(ADDR_MODE-1))
#define         LFSM_EU_BIT            ((unsigned long long)1<<(ADDR_MODE-2))
#define         LFSM_BREAD_BIT        ((unsigned long long)1<<(ADDR_MODE-3))
#define         LFSM_SPECIAL_MASK    ((sector64)((LFSM_SPECIAL_CMD)|(LFSM_EU_BIT)|(LFSM_BOOT_QUERY)))
#define         LFSM_READ_BATCH_RECEIVE ((LFSM_SPECIAL_CMD)|(LFSM_EU_BIT)|(LFSM_BREAD_BIT))
#define         LFSM_LPN_QUERY          ((LFSM_SPECIAL_CMD)|(LFSM_EU_BIT)|(LFSM_BOOT_QUERY))

#define         CMD_BOOT_QUERY        -1
#define LPN_QUERY_HEADER 32

struct erase {
    sector64 start_position;
    int32_t size_in_eus;
};

struct copy {
    sector64 source;
    sector64 dest;
    int32_t run_length_in_pages;
};

struct mark_as_bad {
    sector64 eu_position;
    int32_t runl_length_in_eus;
};

struct special_cmd {
    int8_t signature[4];
    int32_t cmd_type;
    union {
        struct erase erase_cmd;
        struct copy copy_cmd;
        struct mark_as_bad mark_bad_cmd;
        struct sbrm_cmd brm_cmd;
    };
};
//MAG:
struct error_info {
    int32_t failure_code; /**LBN/ LEUN depending upon the command we are dealing with**/
    int32_t logical_num;
};

struct bad_block_info {
    int32_t num_bad_items;
    struct error_info errors[64];
};

#define DEVIO_RESULT_FAILURE -1

#define DEVIO_RESULT_WORKING 1
#define DEVIO_RESULT_SUCCESS 2

extern int32_t compose_special_command_bio(struct diskgrp *pgrp,
                                           int32_t cmd_type, void *cmd,
                                           struct bio *page_bio);
extern int32_t build_mark_bad_command(lfsm_dev_t * td, sector64 eu_position,
                                      int run_length_in_eus);
extern int32_t build_copy_command(lfsm_dev_t * td, sector64 source,
                                  sector64 dest, int32_t run_length_in_pages,
                                  bool isPipeline);
extern int32_t build_erase_command(sector64 start_position,
                                   int32_t size_in_eus);

//int boot_BBM(lfsm_dev_t *td);
extern int32_t BBM_load_all(struct diskgrp *pgrp);
extern int32_t BBM_load(struct diskgrp *pgrp, int32_t id_disk);

extern int32_t special_read_enquiry(sector64 bi_sector,
                                    int32_t query_num, int32_t org_cmd_type,
                                    int32_t BlkUnit,
                                    struct bad_block_info *bad_blk_info);

extern int32_t mark_as_bad(sector64 eu_position, int32_t run_length_in_eus);
extern int32_t build_lpn_query(lfsm_dev_t * td, sector64 target_position,
                               struct page *ret_page);
bool read_updatelog_page(sector64 pos, struct bio_container *ul_read_bio);

extern void hexdump(int8_t * data, int32_t len);

#endif
