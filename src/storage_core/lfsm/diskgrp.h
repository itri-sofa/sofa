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
#ifndef DISKGRP_INC_12093810928312
#define DISKGRP_INC_12093810928312

#define PHYMEDIA_CCMASSD 0
#define PHYMEDIA_HDD    1
#define MAXLEN_DEV_NAME 16
#define MAXLEN_IO_SCHE_NAME 16

struct lfsm_dev_struct;
struct lfsm_subdevice;

typedef struct subdisk_info {
    struct block_device *bdev;  /* Linux block device data structure,
                                   NULL normally, hold bdev when some threads call get_drive */
    sector64 disk_size;         /* disk size in sectors, align with EU size
                                   total size of a disk - size of 1 EU (_get_align_capacity) */
    atomic_t cn_err_read;       /* counting of read error */
    int32_t actived;            // %2=1: enable %2=0: disable
    int32_t cn_open_fail;       /* counting of open device error */
    int8_t nm_dev[MAXLEN_DEV_NAME]; /* device name */
    spinlock_t lock;            /* device lock */
    int32_t cn_bio;             // # of thread who are in my_make_request for the drive
} subdisk_info_t;

typedef struct diskgrp {
    int32_t phymedia;           /* type of disks, SSD (PHYMEDIA_CCMASSD) or HD (PHYMEDIA_HDD) */
    int8_t nm_io_scheduler[MAXLEN_IO_SCHE_NAME];    /* name of disk scheduler */
    int32_t cn_drives;          /* number of disk open success */
    int32_t cn_max_drives;      /* max number of working ssds in lfsm (not including spare) */
    subdisk_info_t *ar_drives;  /* logical representation of each disk */
    struct lfsm_subdevice *ar_subdev;   /* treat each disk as a lfsm sub device */
    sector64 cn_sectors_per_dev;    /* value of disk_size in subdisk_info_t */
    sector64 capacity;          /* total cap of disk group in sector (sum of all disk) */
    atomic_t *ar_cn_selected;   /* array size = # of pgroup */
    sector64 cn_sectors_per_dev_assigned;
} diskgrp_t;

extern diskgrp_t grp_ssds;

extern int32_t diskgrp_check_and_complete_disknum(diskgrp_t * pgrp);
extern int32_t diskgrp_open_dev(struct lfsm_dev_struct *td, int32_t id_disk,
                                bool assign_empty_slot);
extern void diskgrp_init(diskgrp_t * pgrp, int32_t max_drives, int32_t phymedia,
                         int32_t cn_pgroup, int8_t * nm_io_scheduler);
extern void diskgrp_destory(diskgrp_t * pgrp);
extern bool diskgrp_logerr(diskgrp_t * pgrp, int32_t id_disk);
extern bool diskgrp_isfaildrive(diskgrp_t * pgrp, int32_t id_disk);
extern bool diskgrp_isfaildrive_by_pbno(sector64 pbno);
extern struct block_device *diskgrp_getbdev(diskgrp_t * pgrp, int32_t id_dev);

extern void diskgrp_cleanlog(diskgrp_t * pgrp, int32_t id_disk);
extern void diskgrp_switch(diskgrp_t * pgrp, int32_t id_disk, bool bswitch);
extern struct block_device *diskgrp_get_drive(diskgrp_t * pgrp, int32_t id_dev);
extern void diskgrp_put_drive(diskgrp_t * pgrp, int32_t id_dev);

extern int32_t diskgrp_detach(diskgrp_t * pgrp, int32_t id_faildisk);
extern void diskgrp_attach(diskgrp_t * pgrp, int32_t idx_faildisk,
                           int32_t id_disk);

extern int32_t diskgrp_get_erase_upbound(sector64 lsn);

#endif // DISKGRP_INC_12093810928312
