/*
 * SPDX-License-Identifier: GPL-2.0-or-later OR Apache-2.0
 * Copyright (C) 2015-2020 Industrial Technology Research Institute
 *
 *
 *
 *  
 */

#ifndef DEVIO_PRIVATE_H_
#define DEVIO_PRIVATE_H_

#if LFSM_ENABLE_TRIM == 1
static int32_t devio_trim_async(struct block_device *bdev, sector64 lba_trim,
                                int32_t size_in_eus, devio_fn_t * pcb,
                                rq_end_io_fn * rq_end_io);
#endif

#if LFSM_ENABLE_TRIM == 2
static int32_t devio_discard_async(struct block_device *bdev,
                                   sector64 lba_discard, int32_t size_in_eus,
                                   devio_fn_t * pcb, bio_end_io_t * bi_end_io);
#endif

#if LFSM_ENABLE_TRIM == 3       // ## Peter added for nvme trim
static int32_t devio_discard_async_NVME(struct block_device *bdev,
                                        sector64 lba_discard,
                                        int32_t size_in_eus, devio_fn_t * pcb,
                                        bio_end_io_t * bi_end_io);
#endif

#if LFSM_ENABLE_TRIM == 4       // ## Peter added for sgio unmap of scsi ssd
static int32_t devio_trim_by_sgio_unmap(struct block_device *bdev,
                                        sector64 lba_trim, int32_t size_in_eus,
                                        devio_fn_t * pcb,
                                        rq_end_io_fn * rq_end_io);
#endif

#endif /* DEVIO_PRIVATE_H_ */
