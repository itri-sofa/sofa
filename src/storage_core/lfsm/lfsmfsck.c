/*
 * SPDX-License-Identifier: GPL-2.0-or-later OR Apache-2.0
 * Copyright (C) 2015-2020 Industrial Technology Research Institute
 *
 *
 *
 *  
 */
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <error.h>
#include <linux/types.h>
#include "common/common.h"
#include <string.h>

#include "ioctl_A.h"

int ioctl_submit(char *name, unsigned long cmd, void *data)
{

    int ret, fd;
    fd = open(name, O_RDWR | O_SYNC);

    if (fd == -1) {
        perror("open error");
        return -1;
    }

    ret = ioctl(fd, cmd, data);

    close(fd);
    return ret;
}

int fsck_para_check(int argc, char **argv)
{
    if (argc != 3) {
        printf
            ("please input 2 arg    : lfsmfsck lpn|eu_pbno|eu_idhpeu cn_batch\n");
        return -1;
    }
    if (atoi(argv[2]) <= 0) {
        printf("cn_batch < 0 %d\n", atoi(argv[2]));
        return -1;
    }

    return 0;
}

int lfsm_fsck_exec(int id_start, int cn_total, int cn_batch,
                   enum eop_check op_check, int cn_err_msg)
{
    sftl_ioctl_chk_t sftl_check;
    sftl_err_item_t *pitem;
    int sz_mem;
    int cn_err, i;
    int cn_total_err = 0;

    sz_mem = sizeof(sftl_err_msg_t) + sizeof(sftl_err_item_t) * (cn_err_msg);
    if ((sftl_check.perr_msg = malloc(sz_mem)) == NULL) {
        printf("Cannot alloc memory\n");
        return -1;
    }
    sftl_check.id_start = id_start;
    sftl_check.op_check = op_check;
    sftl_check.sz_err_msg = sz_mem;

    int cn_res = cn_total;

    while (cn_res > 0) {
        if (cn_res > cn_batch)
            sftl_check.cn_check = cn_batch;
        else
            sftl_check.cn_check = cn_res;

        cn_err = ioctl_submit("/dev/lfsm", SFTL_IOCTL_CHECK, &sftl_check);

        printf("Check id eu from:%lu cn_eu:%lu msg_cn_err:%d ret:%d \n",
               (long unsigned int)sftl_check.id_start,
               (long unsigned int)sftl_check.cn_check,
               sftl_check.perr_msg->cn_err, cn_err);

        if (sftl_check.perr_msg->cn_err > 0) {
            printf("found_error \n");

            cn_total_err += sftl_check.perr_msg->cn_err;

            for (i = 0; i < sftl_check.perr_msg->cn_err; i++) {
                pitem = &sftl_check.perr_msg->ar_item[i];
                printf
                    ("err:%d id_eu:%d lpno:%lld org_pbn:%lld bmt_pbn:%lld testbit:%d bmt_llkup %d \n",
                     pitem->err_code, pitem->id_eu, pitem->lpno, pitem->org_pbn,
                     pitem->bmt_pbn, pitem->testbit, pitem->ret_bmt_lookup);
            }

            break;
        }
        sftl_check.id_start += sftl_check.cn_check;

        cn_res -= sftl_check.cn_check;
    }

    printf(" Total Error %d\n", cn_total_err);

    free(sftl_check.perr_msg);

}

int main(int argc, char **argv)
{
    struct sftl_ioctl_lfsm_properties sftl_prop;
    struct sftl_lfsm_properties prop;
    sftl_ioctl_chk_t sftl_check;
    int cn_batch = 10;
    int sz_mem, cn_res, i, id_start_eu = 0;
    sftl_prop.pprop = &prop;
    int ret;
    int idx_sel;
    char *cmd;

    if (fsck_para_check < 0)
        exit(0);

    ioctl_submit("/dev/lfsm", SFTL_IOCTL_GET_LFSM_PROPERTY, &sftl_prop);
    printf
        ("Get lfsm properties total SEU %d , fpage_per_seu %d , size lfsm in page %d cn_ssd %d bioc_num %d \n",
         sftl_prop.pprop->cn_total_eus, sftl_prop.pprop->cn_fpage_per_seu,
         sftl_prop.pprop->sz_lfsm_in_page, sftl_prop.pprop->cn_ssds,
         sftl_prop.pprop->cn_bioc);

    cmd = argv[1];
    cn_batch = atoi(argv[2]);

    if (strcmp(cmd, "eu_pbno") == 0) {

        lfsm_fsck_exec(0, sftl_prop.pprop->cn_total_eus, cn_batch, OP_EU_PBNO_MAPPING,  // check operation
                       (sftl_prop.pprop->cn_fpage_per_seu) * cn_batch); // err_msg_number

    } else if (strcmp(cmd, "lpno") == 0) {
        lfsm_fsck_exec(0, sftl_prop.pprop->sz_lfsm_in_page, cn_batch, OP_LPN_PBNO_MAPPING,  // check operation
                       cn_batch);   // err_msg_number

    } else if (strcmp(cmd, "eu_idhpeu") == 0) {
        lfsm_fsck_exec(0, sftl_prop.pprop->cn_total_eus, cn_batch, OP_EU_IDHPEU,    // check operation
                       cn_batch);   // err_msg_number

    } else if (strcmp(cmd, "tab_idhpeu") == 0) {
        lfsm_fsck_exec(0, sftl_prop.pprop->cn_item_hpeutab, cn_batch, OP_HPEU_TAB,  // check operation
                       cn_batch);   // err_msg_number
    } else if (strcmp(cmd, "eu_mdnode") == 0) {
        lfsm_fsck_exec(0, sftl_prop.pprop->cn_total_eus, cn_batch, OP_EU_MDNODE,    // check operation
                       cn_batch);   // err_msg_number
    } else if (strcmp(cmd, "eu_validcount") == 0) {
        lfsm_fsck_exec(0, sftl_prop.pprop->cn_total_eus, cn_batch, OP_EU_VALID_COUNT,   // check operation
                       cn_batch);   // err_msg_number
    } else if (strcmp(cmd, "eu_state") == 0) {
        lfsm_fsck_exec(0, sftl_prop.pprop->cn_ssds, cn_batch, OP_EU_STATE,  // check operation
                       cn_batch * (sftl_prop.pprop->cn_total_eus) / (sftl_prop.pprop->cn_ssds));    // err_msg_number
    }

    else if (strcmp(cmd, "eu_active") == 0) // AN : eu_active : for this item, only need to check one time
    {
        lfsm_fsck_exec(0, 1, cn_batch, OP_ACTIVE_EU_FROTIER,    // check operation
                       (sftl_prop.pprop->cn_ssds) * 3); // err_msg_number
    } else if (strcmp(cmd, "systable") == 0)    // AN : system_table : for this item, only need to check one time
    {
        lfsm_fsck_exec(0, 1, cn_batch, OP_CHECK_SYS_TABLE,  // check operation
                       (sftl_prop.pprop->cn_ssds) * 10);    // err_msg_number
    } else if (strcmp(cmd, "freelist") == 0)    // must run  udevadm control --stop-exec-queue before check
    {
        lfsm_fsck_exec(0, sftl_prop.pprop->cn_bioc, cn_batch, OP_CHECK_FREE_BIOC,   // check operation
                       cn_batch);   // err_msg_number
    } else if (strcmp(cmd, "freelist_simple") == 0) //AN : freelist : for this item, only need to check one time
    {                           // must run  udevadm control --stop-exec-queue before check
        lfsm_fsck_exec(0, 1, cn_batch, OP_CHECK_FREE_BIOC_SIMPLE,   // check operation
                       cn_batch);   // err_msg_number
    }

    else if (strcmp(cmd, "systable_hpeu") == 0) //AN :  for this item, only need to check one time
    {                           // must run  udevadm control --stop-exec-queue before check
        lfsm_fsck_exec(0, 1, cn_batch, OP_SYSTABLE_HPEU,    // check operation
                       cn_batch);   // err_msg_number
    } else
        printf("Cannot find cmd %s \n", cmd);
}
