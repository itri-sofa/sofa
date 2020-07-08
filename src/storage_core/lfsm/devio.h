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
#ifndef _DEVIO_H_
#define _DEVIO_H_

struct lfsm_dev_struct;

#define SG_ATA_16_LEN           16
#define SCSI_SENSE_BUFFERSIZE   96
#define SG_ATA_PROTO_PIO_IN (4<<1)
#define SG_ATA_PROTO_DMA        (6 << 1)
#define ATA_USING_LBA (1 << 6)
#define ATA_OP_DSM (0x06)
#define ATA_OP_IDENT (0xEC)
#define ATA_OP_SMART (0xB0)
#define ATA_FEATURE_SMART_READ_DATA (0xD0)

#define SG_ATA_LBA48        1
#define SG_ATA_16               0x85
#define SZ_TRIM_SECTOR_MAX (1<<15)

enum {
    SG_CDB2_TLEN_NODATA = 0 << 0,
    SG_CDB2_TLEN_FEAT = 1 << 0,
    SG_CDB2_TLEN_NSECT = 2 << 0,

    SG_CDB2_TLEN_BYTES = 0 << 2,
    SG_CDB2_TLEN_SECTORS = 1 << 2,

    SG_CDB2_TDIR_TO_DEV = 0 << 3,
    SG_CDB2_TDIR_FROM_DEV = 1 << 3,

    SG_CDB2_CHECK_COND = 1 << 5,
};

typedef struct lfsm_devio_callback {
    atomic_t result;
    wait_queue_head_t io_queue;
    int32_t id_disk;            // used in devio trim
    union {
        struct request *rq;
        struct bio *pBio;
    };
} devio_fn_t;

#pragma pack(1)
struct ata_smart_attribute {
    uint8_t id;
    uint16_t flags;
    uint8_t curr;
    uint8_t worst;
    uint8_t raw[6];
    uint8_t reserv;
};
#pragma pack()

extern bool devio_write_pages(sector64 addr, int8_t * buffer,
                              sector64 * ar_spare_data, int32_t cn_hard_pages);
extern bool devio_write_page(sector64 addr, int8_t * buffer,
                             sector64 spare_data, devio_fn_t * pCb);
extern bool devio_query_lpn(int8_t * buffer, sector64 addr, int32_t len);
extern bool devio_read_pages(sector64 pbno, int8_t * buffer, int32_t cn_fpage,
                             devio_fn_t * pCb);
extern bool devio_read_hpages_batch(sector64 pbno_in_sects, int8_t * buffer, int32_t cn_hpage); // For read in the batch
extern bool devio_read_hpages(sector64 pbno_in_sects, int8_t * buffer,
                              int32_t cn_fpage);
extern bool devio_reread_pages(sector64 pbno, int8_t * buffer,
                               int32_t cn_fpage);
extern bool devio_reread_pages_after_fail(sector64 pbno, int8_t * buffer,
                                          int32_t cn_fpage);

extern bool devio_raw_rw_page(sector64 addr, struct block_device *bdev,
                              int8_t * buffer, int32_t fpage_len, uint64_t rw);
extern bool bio_vec_hpage_rw(struct bio_vec *page_vec, int8_t * buffer,
                             int32_t op);

extern bool bio_vec_data_rw(struct bio_vec *page_vec, int8_t * buffer,
                            int32_t op);
extern int32_t devio_compare(struct bio_vec *iovec, sector64 tar_pbno);
extern int32_t devio_erase(sector64 lsn, int32_t size_in_eus,
                           struct block_device *bdev);
extern int32_t devio_get_identify(int8_t * nm_dev, uint32_t * pret_key);

extern int32_t devio_get_spin_hour(int8_t * nm_dev, int32_t * pret_spin_hour);

extern int32_t devio_erase_async(sector64 start_position, int32_t size_in_eus,
                                 devio_fn_t * pcb);
extern int32_t devio_erase_async_bh(devio_fn_t * pcb);

extern void end_bio_devio_general(struct bio *bio);
extern bool devio_specwrite(int32_t id_disk, int32_t type, void *pcmd);
extern int32_t devio_make_request(struct bio *bio, bool isDirectMakeReq);
extern bool devio_async_write_bh(devio_fn_t * pCb);
extern int32_t devio_async_read_pages(sector64 pbno, int32_t cn_fpage,
                                      devio_fn_t * cb);
extern bool devio_async_read_pages_bh(int32_t cn_fpages, int8_t * buffer,
                                      devio_fn_t * pcb);
extern int32_t devio_async_read_hpages(sector64 pbno, int32_t cn_hpage,
                                       devio_fn_t * pcb);
extern bool devio_async_read_hpages_bh(int32_t cn_hpages, int8_t * buffer,
                                       devio_fn_t * pcb);
extern int32_t devio_copy_fpage(sector64 src_pbno, sector64 dest_pbno1,
                                sector64 dest_pbno2, int32_t cn_fpage);

extern int32_t devio_write_pages_safe(sector64 addr,
                                      int8_t * buffer, sector64 * ar_spare_data,
                                      int32_t cn_flash_pages);
extern void init_devio_cb(devio_fn_t * cb);

#define CN_FPAGE_PER_BATCH 8192

#endif
