/*
 * Copyright (c) 2015-2020 Industrial Technology Research Institute.
 *
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 *
 *
 *
 *
 *
 *
 *  
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <errno.h>
#include <syslog.h>
#include <unistd.h>
#include "sock_util.h"

int32_t socket_create (int32_t domain, int32_t type, int32_t protocol)
{
    int32_t sk_fd;

    sk_fd = -1;

    sk_fd = socket(domain, type, protocol);

    if (sk_fd < 0) {
        syslog(LOG_INFO, "SOFA TOOL fail create socket\n");
    }

    return sk_fd;
}


int32_t socket_setopt (int32_t sk_fd, int32_t level, int32_t optname,
        void *optval, int32_t optlen)
{
    int32_t ret;

    ret = -1;
    ret = setsockopt(sk_fd, level, optname, (int8_t *)optval, optlen);

    if (ret < 0) {
        syslog(LOG_INFO, "SOFA TOOL fail set socket option %d\n", optname);
    }
    return ret;
}

int32_t socket_connect (int8_t *ipaddr, int32_t port)
{
    struct sockaddr_in address;
    int8_t sockfd;
    int32_t ret;

    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    address.sin_addr.s_addr = inet_addr(ipaddr);
    bzero(&(address.sin_zero), 8);

    sockfd = socket_create(AF_INET, SOCK_STREAM, 0);

    if (sockfd < 0) {
        syslog(LOG_INFO, "SOFA TOOL fail create socket\n");
        return -1;
    }

    ret = connect(sockfd, (struct sockaddr*)&address, sizeof(struct sockaddr));
    if (ret < 0) {
        syslog(LOG_INFO, "SOFA TOOL fail connect %s:%d\n", ipaddr, port);
        close(sockfd);
        sockfd = -1;
    }

    return sockfd;
}

int32_t socket_send (int32_t sk_fd, int8_t *send_buffer, int32_t tx_len) {
    int8_t *send_ptr;
    int32_t local_tx_len, res, ret;

    send_ptr = send_buffer;
    local_tx_len = 0;
    res = 0;
    ret = 0;

    while (local_tx_len < tx_len) {
        res = write(sk_fd, send_ptr, tx_len - local_tx_len);

        if (res < 0) {
            if (errno != EINTR) {
                syslog(LOG_ERR, "SOFA TOOL socket_sendto ERROR: errno %d\n",
                        errno);
                break;
            }
        }

        local_tx_len += res;
        send_ptr = send_ptr + res;
    }

    return (local_tx_len == tx_len ? 0 : -1);
}

int32_t socket_rcv (int32_t sk_fd, int8_t *rx_buf, int32_t rx_len)
{
    int8_t *recv_ptr;
    int32_t local_rx_len, res;

    recv_ptr = rx_buf;
    local_rx_len = 0;
    res = 0;

    while (local_rx_len < rx_len) {
        res = recv(sk_fd, recv_ptr, rx_len - local_rx_len, MSG_WAITALL);
        if (res < 0) {
            if (errno != EINTR && errno != EAGAIN && errno != EWOULDBLOCK) {
                syslog(LOG_ERR, "DMSC INFO rcv data ERROR: errno %d\n", errno);
                break;
            }
        } else if (res == 0) {
            syslog(LOG_ERR, "socket_recvfrom receive 0 bytes and errno %d\n",
                    errno);
            return 0;
        }

        local_rx_len = local_rx_len + res;
        recv_ptr = recv_ptr + res;
    }

    return ((rx_len == local_rx_len) ? 0 : -1);
}

int32_t socket_close (int32_t sk_fd) {
    int32_t ret;

    ret = shutdown(sk_fd, SHUT_RDWR);
    ret = close(sk_fd);

    return ret;
}
