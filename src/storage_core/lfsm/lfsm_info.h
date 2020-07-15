/*
 * SPDX-License-Identifier: GPL-2.0-or-later OR Apache-2.0
 * Copyright (C) 2015-2020 Industrial Technology Research Institute
 *
 *
 *
 *  
 */
#ifndef LFSM_PROCFS_12312848123
#define LFSM_PROCFS_12312848123

typedef struct SProcLFSM {
    struct proc_dir_entry *dir_main;
    struct proc_dir_entry *dir_ect;
    struct proc_dir_entry *dir_rddcy;
    struct proc_dir_entry *dir_heap;
    struct proc_dir_entry *dir_hlist;
    struct proc_dir_entry *dir_perfMon;
} SProcLFSM_t;

void release_procfs(SProcLFSM_t * pproc);
void init_procfs(SProcLFSM_t * pproc);

#endif
