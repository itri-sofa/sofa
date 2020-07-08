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

#ifndef SOCK_UTIL_H_
#define SOCK_UTIL_H_

int32_t socket_create(int32_t domain, int32_t type, int32_t protocol);
int32_t socket_setopt(int32_t sk_fd, int32_t level, int32_t optname,
        void *optval, int32_t optlen);
int32_t socket_rcv(int32_t sk_fd, int8_t *rx_buf, int32_t rx_len);
int32_t socket_send(int32_t sk_fd, int8_t *send_buffer, int32_t tx_len);
int32_t socket_connect(int8_t *ipaddr, int32_t port);
int32_t socket_close(int32_t sk_fd);

#endif /* SOCK_UTIL_H_ */
