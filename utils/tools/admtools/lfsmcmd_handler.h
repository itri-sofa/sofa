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

#ifndef LFSMCMD_HANDLER_H_
#define LFSMCMD_HANDLER_H_

extern int32_t gen_lfsm_packet_buffer(user_cmd_t *ucmd, int8_t **buff);
extern void show_lfsmcmd_result(user_cmd_t *ucmd);
extern void free_lfsmcmd_resp(user_cmd_t *ucmd);
extern void _show_getstatus(void);
extern void _show_getproperties(void);
extern void _show_activate(void);
extern void _show_checkSOFA(void);
extern void _show_setgcratio(uint32_t on_ratio, uint32_t off_ratio);
extern void _show_setgcreserve(uint32_t reserve_value);
#endif /* LFSMCMD_HANDLER_H_ */
