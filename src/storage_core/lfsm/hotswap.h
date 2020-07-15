/*
 * SPDX-License-Identifier: GPL-2.0-or-later OR Apache-2.0
 * Copyright (C) 2015-2020 Industrial Technology Research Institute
 *
 *
 *
 *  
 */

#ifndef    HOT_SWAP_H
#define    HOT_SWAP_H

typedef struct shotswap {
// set part
    int32_t ar_id_spare_disk[MAXCN_HOTSWAP_SPARE_DISK]; /* spare disk pool */
    int32_t ar_actived[MAXCN_HOTSWAP_SPARE_DISK];   // 1: enable 0: disable /* indicate the disk in pool is good or bad */
    sector64 ar_write_count[MAXCN_HOTSWAP_SPARE_DISK];  // Write count for spare disks */
    sector64 ar_write_max[MAXCN_HOTSWAP_SPARE_DISK];    // Write max for spare disks */
    int32_t cn_spare_disk;      /* total num of spare disks */
    spinlock_t lock;            /* protect access of pool TODO: FSS-64 */

    struct task_struct *pthread;
    wait_queue_head_t wq;
    bool fg_enable;             /* if no spare disks, set as false to let thread idle */
    lfsm_dev_t *td;
    int32_t waittime;           /* wait 10 seconrds to open disk (why?) */
    int8_t char_msg[500];       /* message buffer */
} shotswap_t;

extern int32_t hotswap_init(shotswap_t * phs, lfsm_dev_t * td);
extern int32_t hotswap_destroy(shotswap_t * phs);
extern int32_t hotswap_info_read(int8_t * buffer, uint32_t size);

extern int32_t hotswap_euqueue_disk(shotswap_t * phs, int32_t id_disk,
                                    int32_t actived);
extern int32_t hotswap_dequeue_disk(shotswap_t * phs, int32_t id_disk);
extern int32_t hotswap_delete_disk(shotswap_t * phs, int32_t id_disk);
#endif
