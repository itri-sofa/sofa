/*
 * SPDX-License-Identifier: GPL-2.0-or-later OR Apache-2.0
 * Copyright (C) 2015-2020 Industrial Technology Research Institute
 *
 *
 *
 *  
 */

#ifndef CONFIG_LFSM_SETTING_H_
#define CONFIG_LFSM_SETTING_H_

#define MAXCN_HOTSWAP_SPARE_DISK          50    //TODO: We should move to hotswap module
#define FLASH_PAGE_PER_BLOCK_ORDER        8 // 8k  modify here
#define SECTORS_PER_FLASH_PAGE_ORDER      3

/* We can have multiple logical EUs per super_EU. */
/* Change the value by assuming each EU is 1025 sectors = 512 kb */
#define SUPER_EU_ORDER                  6   // 2ce : 5; 4ce : 6
#define NUM_EUS_PER_SUPER_EU     (1<<SUPER_EU_ORDER)    // Always Power of 2
#define EU_ORDER                 (FLASH_PAGE_ORDER + FLASH_PAGE_PER_BLOCK_ORDER + SUPER_EU_ORDER )  // 2^19 = 512KB  // CHYI: EU_ORDER is in byte// one EU 64 page = 2^6, EU ORDER how many byte in SEU

/* EU size should be configured accurately depending on the specific SSD */
/* sector size is fixed at 512b
 * if current lfsm define 8k page size, assume 8k page size in firmware
 * all other define is derived from these 2 defines
 * NOTE: 20160908-lego for speedup, we replace / % with bit operation (>> << &)
 *       if we want to change the FLASH_PAGE_ORDER and FLASH_PAGE_SIZE
 *       *****remember to check all corresponding setting
 */
#define FLASH_PAGE_ORDER                (SECTORS_PER_FLASH_PAGE_ORDER+SECTOR_ORDER)
#define FLASH_PAGE_SIZE                     (1 << FLASH_PAGE_ORDER)
#define SECTORS_PER_FLASH_PAGE          (1<<SECTORS_PER_FLASH_PAGE_ORDER)
#define MEM_PER_FLASH_PAGE    (1 <<(SECTORS_PER_FLASH_PAGE_ORDER - SECTORS_PER_PAGE_ORDER))

#define FLASH_PAGE_MASK (~((sector64)(FLASH_PAGE_SIZE-1)))
#define N_FLASH_PAGE_MASK (~((sector64)FLASH_PAGE_MASK))

#define SECTORS_PER_HARD_PAGE_ORDER     (SECTORS_PER_FLASH_PAGE_ORDER)
#define HARD_PAGE_ORDER                 (SECTORS_PER_HARD_PAGE_ORDER+SECTOR_ORDER)
#define HARD_PAGE_SIZE                  (1 << HARD_PAGE_ORDER)
#define SECTORS_PER_HARD_PAGE           (1<<SECTORS_PER_HARD_PAGE_ORDER)
#define MEM_PER_HARD_PAGE    (1 <<(SECTORS_PER_HARD_PAGE_ORDER - SECTORS_PER_PAGE_ORDER))
#define HPAGE_PER_SEU_ORDER (EU_ORDER-HARD_PAGE_ORDER)
#define HPAGE_PER_SEU    (1L<<HPAGE_PER_SEU_ORDER)

#define EU_SIZE                             (1<<EU_ORDER)
#define EU_MASK                             (EU_SIZE-1)
#define PAGE_ORDER                  12  // 2^12 = 4KB
#define N_PAGE_MASK                 (~PAGE_MASK)
#define SECTOR_ORDER                    9   // 2^9   = 512 B
#define N_SECTOR_MASK                       (SECTOR_SIZE-1)
#define SECTORS_PER_PAGE_ORDER              (PAGE_ORDER-SECTOR_ORDER)
#define SECTORS_PER_PAGE                    (1<<SECTORS_PER_PAGE_ORDER) // 1 page = 8 sectors

#define FPAGE_PER_SEU_ORDER (EU_ORDER-FLASH_PAGE_ORDER)
#define FPAGE_PER_SEU    (1L<<FPAGE_PER_SEU_ORDER)

#define SECTORS_PER_SEU_ORDER (EU_ORDER - SECTOR_ORDER)
#define SECTORS_PER_SEU (1 << SECTORS_PER_SEU_ORDER)

#define PAGE_SIZE_ORDER 12

#define MAX_MEM_PAGE_NUM BIO_MAX_PAGES
#define MAX_FLASH_PAGE_NUM (MAX_MEM_PAGE_NUM/MEM_PER_FLASH_PAGE)

#define TEMP_LAYER_NUM_ORDER            2
#define TEMP_LAYER_NUM                 ((1<<TEMP_LAYER_NUM_ORDER)-1)    /* # of temperatures configured */
#define MIN_RESERVE_EU            (TEMP_LAYER_NUM+1)
#define DATALOG_GROUP_NUM   10

#endif /* CONFIG_LFSM_SETTING_H_ */
