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
