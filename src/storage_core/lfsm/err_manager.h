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
#ifndef _ERRMGR_H_
#define _ERRMGR_H_

extern void errmgr_wait_falselist_clean(void);
extern void errmgr_bad_process(sector64 vic_pbno_start, page_t vic_lpn_start,
                               page_t vic_lpn_end, lfsm_dev_t * td,
                               bool isMetadata, bool to_remove_pin);
extern void move_to_FalseList(struct HListGC *h, struct EUProperty *vic_eu);

#endif // _ERRMGR_H_
