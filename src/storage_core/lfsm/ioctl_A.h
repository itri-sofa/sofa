/*
 * SPDX-License-Identifier: GPL-2.0-or-later OR Apache-2.0
 * Copyright (C) 2015-2020 Industrial Technology Research Institute
 *
 *
 *
 *  
 */
#ifndef __IOCTL_A_H
#define __IOCTL_A_H

#define SFTL_IOCTL_PRINT_DEBUG _IO( 0xac, 0)
#define SFTL_IOCTL_SIMU_IO _IO( 0xac, 1)
#define SFTL_IOCTL_SUBMIT_IO _IO( 0xac, 2)
#define SFTL_IOCTL_INIT_FIN _IO( 0xac, 3)
#define SFTL_IOCTL_SUBMIT_IO_PAYLOAD _IO( 0xac, 4)
#define SFTL_IOCTL_SCANDEV _IO( 0xac, 5)
#define SFTL_IOCTL_DUMPCALL _IO(0xac, 6)

#define SFTL_IOCTL_SYS_DUMPALL _IO(0xac, 7)

#define SFTL_IOCTL_CHANGE_DEVNAME_DUMPALL _IO(0xac, 8)
#define SFTL_IOCTL_TRIM _IO(0xac,9)
#define SFTL_IOCTL_GET_LFSM_PROPERTY _IO(0xac,10)

#define SFTL_IOCTL_COPY_TO_KERNEL _IO(0xac,11)
#define SFTL_IOCTL_CHECK _IO(0xac,12)

typedef struct {
    int32_t m_nSum;             /* Number of writes */
    int32_t m_nSize;            /* Per-Req size in sector */
    int32_t m_nMd;              /* Delay in millisecond */
    int32_t m_nBatchSize;
    int32_t m_nType;            /* 0, sequential; 1, random */
    int32_t m_nRw;              /* 0, read request; 1, write request, 2, check the correctness of the read/write pair */
    char *m_pPayload;
    int32_t m_nPayloadLen;
    sector64 m_start_sector;
    int8_t data[512];
} sftl_ioctl_simu_w_t;

typedef struct {
    int8_t action[16];
    int8_t nm_dev[16];
} sftl_ioctl_scandev_t;

typedef struct {
    int8_t *action;
    int32_t num_item;
} sftl_ioctl_sys_dump_t;

typedef struct {
    int8_t *nm_dev;
    int32_t disk_num;
} sftl_change_dev_name_t;

typedef struct {
    int64_t lpn;
    int64_t cn_pages;
} sftl_ioctl_trim_t;

enum eerr_code {
    ENO_META_DATA = 1,
    ECCPAGE_BUT_TESTBIT1 = 2,
    ELPNO_EXCEED_SIZE = 3,
    ERET_BMT_LOOKUP = 4,
    ETESTBIT0 = 5,
    ETESTBIT1 = 6,
    EPBNO_WRONG = 7,
    EEU_HPEUID = 8,
    EEU_HEPUID_NO_MATCH = 9,
    EEU_VALID_COUNT = 10,
    EACTIVE_EU_MDNODE_NONEXIST = 11,
    EACTIVE_EU_MDNODE_INVALID = 12,
    ENONACTIVE_EU_MDNODE_EXIST = 13,
    ENONACTIVE_EU_MDNODE_VALID = 14,
    ETEMPER = 15,
    ESTATE = 16,
    EFREE_LIST_NUMBER_MISMATCH = 17,
    EHLIST_NUMBER_MISMATCH = 18,
    EEU_LOST = 19,
    EEU_DUPLICATE = 20,
    ESUPERMAP_READ_FAIL = 20,
    ESPM_DISK_FAIL = 21,
    ESPM_NONEMPTY = 22,
    ESYS_MAP_DIFFERENT = 23,
    ESYS_SPARE_DIFFERENT = 24,
    ESYS_SPARE_READ_ERR = 25,
    ESYS_HPEU_TABLE = 26
};

enum eop_check {
    OP_LPN_PBNO_MAPPING = 1,
    OP_EU_PBNO_MAPPING = 2,
    OP_EU_IDHPEU = 3,
    OP_HPEU_TAB = 4,
    OP_EU_MDNODE = 5,
    OP_EU_VALID_COUNT = 6,
    OP_EU_STATE = 7,
    OP_ACTIVE_EU_FROTIER = 8,
    OP_CHECK_SYS_TABLE = 9,
    OP_CHECK_FREE_BIOC = 10,
    OP_CHECK_FREE_BIOC_SIMPLE = 11,
    OP_SYSTABLE_HPEU = 12
};

enum op_write_count {
    OP_WCOUNT_INIT = 1,
    OP_WCOUNT_MAX = 2,
};

struct sftl_lfsm_properties {
    int32_t cn_total_eus;
    int32_t cn_fpage_per_seu;
    int32_t sz_lfsm_in_page;
    int32_t cn_item_hpeutab;
    int32_t cn_ssds;
    int32_t cn_bioc;
};

struct sftl_ioctl_lfsm_properties {
    struct sftl_lfsm_properties *pprop;
};

typedef struct sftl_err_item {
    int32_t err_code;
    int32_t id_eu;
    sector64 lpno;
    sector64 org_pbn;
    sector64 bmt_pbn;
    int32_t testbit;
    int32_t ret_bmt_lookup;
    int32_t cn_eu_inhpeu;
    int32_t id_hpeu;
    int32_t state_eu;
    int32_t id_disk;
    int32_t id_group;
    int32_t idx_page;
    int32_t id_bioc;
    struct bio_container *pbioc;
    int32_t cn_bioc;
} sftl_err_item_t;

typedef struct sftl_err_msg {
    int32_t cn_err;
    sftl_err_item_t ar_item[0];
} sftl_err_msg_t;

typedef struct sftl_ioctl_check {
    enum eop_check op_check;
    sector64 id_start;
    sector64 cn_check;

    int32_t sz_err_msg;
    sftl_err_msg_t *perr_msg;
} sftl_ioctl_chk_t;

typedef struct sftl_copy_to_kernel {
    int8_t *puser;
    int8_t *pkernel;
    int32_t cn;
} sftl_cp2kern_t;

typedef struct sftl_write_count {
    enum op_write_count op;
    int32_t id;
    sector64 wcount;
} sftl_wcount_t;

#endif
