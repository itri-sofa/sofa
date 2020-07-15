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
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <syslog.h>
#include <errno.h>
#include <netinet/in.h>
#include <netdb.h>
#include <net/if.h>
#include <string.h>
#include <pthread.h>
#include "../common/sofa_mm.h"
#include "userD_sock_srv_private.h"
#include "userD_sock_srv.h"
#include "userD_req_mgr.h"
#include "userD_main.h"

static int32_t socket_send(int32_t sk_fd, int8_t * tx_buf, int32_t tx_len)
{
    int8_t *send_ptr;
    int32_t local_tx_len, res, ret;

    send_ptr = tx_buf;
    local_tx_len = 0;
    res = 0;
    ret = 0;
    while (local_tx_len < tx_len) {
        res = write(sk_fd, send_ptr, tx_len - local_tx_len);

        if (res < 0) {
            if (errno != EINTR) {
                syslog(LOG_ERR, "[SOFA] USERD INFO socket send fail errno %d\n",
                       errno);
                break;
            }
        }

        local_tx_len += res;
        send_ptr = send_ptr + res;
    }

    //syslog(LOG_INFO, "local tx %d tx len %d res %d\n",
    //        local_tx_len, tx_len, (local_tx_len == tx_len ? 0 : -1));
    return (local_tx_len == tx_len ? 0 : -1);
}

static int32_t socket_rcv(int32_t sk_fd, int8_t * rx_buf, int32_t rx_len)
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
//                syslog(LOG_ERR,
//                        "[SOFA] USERD INFO recv data fail errno %d\n", errno);
                break;
            }
        } else if (res == 0) {
            //syslog(LOG_ERR, "[SOFA] USERD INFO rcv 0 bytes and errno %d\n", errno);
            return -1;
        }

        local_rx_len = local_rx_len + res;
        recv_ptr = recv_ptr + res;
    }

    return ((rx_len == local_rx_len) ? 0 : -1);
}

static int32_t _add_new_client(userD_sksrv_t * mysrv, int32_t sk_fd)
{
    int32_t i, ret;

    ret = -1;

    pthread_mutex_lock(&mysrv->sock_fd_lock);
    for (i = 0; i < MAX_SOCK_CLIENT; i++) {
        if (mysrv->client_sock_fd[i] == -1) {
            mysrv->client_sock_fd[i] = sk_fd;
            ret = i;
            break;
        }
    }

    pthread_mutex_unlock(&mysrv->sock_fd_lock);

    return ret;
}

int32_t _create_srv_sock(userD_sksrv_t * mysrv)
{
    struct sockaddr_in myaddr;
    int32_t ret, on;

    ret = -1;
    on = 0;

    /* create socket */
    mysrv->sock_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (mysrv->sock_fd < 0) {
        syslog(LOG_ERR,
               "[SOFA] USERD ERROR User Daemon fail create socket ret: %d\n",
               mysrv->sock_fd);
        return ret;
    }

    do {
        ret =
            setsockopt(mysrv->sock_fd, SOL_SOCKET, SO_REUSEADDR,
                       (int8_t *) & on, sizeof(on));

        if (ret < 0) {
            syslog(LOG_ERR, "[SOFA] USERD ERROR setsockopt error : %d\n", ret);
            break;
        }

        /* initialize structure dest */
        bzero(&myaddr, sizeof(struct sockaddr_in));
        myaddr.sin_family = AF_INET;
        myaddr.sin_port = htons(mysrv->sock_port);

        /* this line is different from client */
        myaddr.sin_addr.s_addr = INADDR_ANY;

        /* assign a port number to socket */
        ret =
            bind(mysrv->sock_fd, (struct sockaddr *)&myaddr,
                 sizeof(struct sockaddr_in));

        if (ret < 0) {
            syslog(LOG_ERR, "[SOFA] USERD ERROR User Daemon fail bind port "
                   "ret: %d errno %d\n", ret, errno);
            break;
        }

        /* make it listen to socket with max 20 connections */
        ret = listen(mysrv->sock_fd, MAX_SOCK_CLIENT);

        if (ret < 0) {
            syslog(LOG_ERR,
                   "[SOFA] USERD ERROR User fail listen port return code: %d\n",
                   ret);
            break;
        }

        ret = 0;
    } while (0);

    if (ret) {
        syslog(LOG_INFO, "[SOFA] USERD ERROR socket fd is %d stop\n",
               mysrv->sock_fd);
        shutdown(mysrv->sock_fd, SHUT_RDWR);
        close(mysrv->sock_fd);
        mysrv->sock_fd = -1;
    }

    return ret;
}

int32_t _create_sock_srv(userD_sksrv_t * mysrv)
{
    int32_t retry_cnt, ret;

    retry_cnt = 0;
    ret = 0;
    while (retry_cnt <= MAX_RETRY_BUILD_SKSRV) {
        if (_create_srv_sock(mysrv)) {
            sleep(RETRY_PERIOD);
            retry_cnt++;
            continue;
        }

        break;
    }

    if (retry_cnt > MAX_RETRY_BUILD_SKSRV) {
        syslog(LOG_ERR, "[SOFA] USERD ERROR fail to create socket server\n");
        return -1;
    }

    return ret;
}

void _init_sock_srv(userD_sksrv_t * mysrv, int32_t port_num)
{
    int32_t i;

    mysrv->running = 0;
    mysrv->sock_fd = -1;
    mysrv->sock_port = port_num;
    for (i = 0; i < MAX_SOCK_CLIENT; i++) {
        mysrv->client_sock_fd[i] = -1;
    }

    pthread_mutex_init(&mysrv->sock_fd_lock, NULL);
}

static void _req_to_hostOrder(req_header_t * req_hder)
{
    req_hder->mNum_s = ntohl(req_hder->mNum_s);
    req_hder->opCode = ntohs(req_hder->opCode);
    req_hder->subopCode = ntohs(req_hder->subopCode);
    req_hder->reqID = ntohl(req_hder->reqID);
    req_hder->size = ntohl(req_hder->size);
}

static void _send_resp(int32_t skfd, req_header_t * req_hder,
                       resp_header_t * resp, int8_t * buff)
{
    uint32_t mnum_e;
    int16_t opcode, resp_hdsize, resp_data_size;

    opcode = req_hder->opCode;
    resp_data_size = resp->size;

    resp->mNum_s = htonl(MNUM_S);
    resp->opCode = htons(req_hder->opCode);
    resp->subopCode = htons(req_hder->subopCode);
    resp->reqID = htonl(req_hder->reqID);
    resp->result = htons(resp->result);
    resp->size = htonl(resp->size);

    resp_hdsize = sizeof(resp_header_t);
    if (socket_send(skfd, (int8_t *) resp, resp_hdsize)) {
        syslog(LOG_INFO, "[SOFA] USERD INFO send resp header fail\n");
        return;
    }
    //syslog(LOG_INFO, "[SOFA] USERD INFO send resp para = %d\n", resp_data_size);
    if (resp_data_size > 0) {
        if (socket_send(skfd, buff, resp_data_size)) {
            syslog(LOG_INFO, "[SOFA] USERD INFO send resp para fail\n");
            return;
        }
    }

    mnum_e = htonl(MNUM_E);

    if (socket_send(skfd, (int8_t *) & mnum_e, sizeof(uint32_t))) {
        syslog(LOG_INFO, "[SOFA] USERD INFO send resp end fail\n");
        return;
    }
}

/*
 * return value:
 * -1 : conn broken
 *  1 : wrong packet format
 *  0 : success
 */
static int32_t _rcv_req(int32_t skfd, req_header_t * req)
{
    int32_t retCode;
    uint32_t mnum_e;

    do {
        retCode = socket_rcv(skfd, (int8_t *) req, sizeof(req_header_t));

        if (retCode < 0) {
//            syslog(LOG_INFO,
//                    "[SOFA] USERD INFO recv data error retCode %d\n", retCode);
            break;
        }

        _req_to_hostOrder(req);

        if (req->size <= 0 || req->mNum_s != MNUM_S) {
            syslog(LOG_INFO,
                   "[SOFA] USERD INFO wrong request header size <= 0 (%d) or "
                   "wrong header start\n", req->size);
            socket_rcv(skfd, (int8_t *) & mnum_e, sizeof(int32_t));
            retCode = EILSEQ;
            continue;
        }
    } while (0);

    return retCode;
}

static void reset_client_fd(int32_t * sock_fd)
{
    pthread_mutex_lock(&sofa_sksrv.sock_fd_lock);
    *sock_fd = -1;
    pthread_mutex_unlock(&sofa_sksrv.sock_fd_lock);
}

static void fn_cmd_handler(void *data)
{
    req_header_t req_hder;
    resp_header_t resp_hder;
    int8_t *buffer, *resp_buff;
    int32_t clientfd, retCode, mnum_e;

    if (data == NULL) {
        syslog(LOG_ERR, "[SOFA] USERD WARN the NULL ptr of sock fd data\n");
        return;
    }

    clientfd = *((int32_t *) data);

    while (1) {
        memset(&req_hder, 0, sizeof(req_header_t));
        retCode = _rcv_req(clientfd, &req_hder);

        if (retCode) {
//            syslog(LOG_INFO,
//                    "[SOFA] USERD INFO recv request header error retCode %d\n",
//                    retCode);

            if (retCode < 0) {
                break;
            } else {
                continue;
            }
        }

        buffer = sofa_malloc(sizeof(int8_t) * req_hder.size);
        if (buffer == NULL) {
            syslog(LOG_ERR,
                   "[SOFA] USERD WARN fail alloc mem for req with size (%d)\n",
                   req_hder.size);
            socket_rcv(clientfd, (int8_t *) & mnum_e, sizeof(int32_t));
            //TODO: we should reply caller with error
            continue;
        }

        if (socket_rcv(clientfd, buffer, req_hder.size) < 0) {
            syslog(LOG_INFO, "[SOFA] USERD INFO fail recv req para\n");
            sofa_free(buffer);
            continue;
        }

        if (socket_rcv(clientfd, (int8_t *) & mnum_e, sizeof(int32_t)) < 0) {
            syslog(LOG_INFO, "[SOFA] USERD INFO fail recv req end\n");
            sofa_free(buffer);
            continue;
        }

        if (MNUM_E != ntohl(mnum_e)) {
            syslog(LOG_INFO, "[SOFA] USERD INTO wrong end of request\n");
            sofa_free(buffer);
            continue;
        }

        resp_hder.result = handle_req(fd_chrDev, req_hder.opCode,
                                      req_hder.subopCode, buffer,
                                      &resp_hder.size, &resp_buff);

        _send_resp(clientfd, &req_hder, &resp_hder, resp_buff);

        if (buffer != NULL) {
            sofa_free(buffer);
            buffer = NULL;
        }

        if (resp_buff != NULL) {
            sofa_free(resp_buff);
            resp_buff = NULL;
        }
    }

    close(clientfd);
    reset_client_fd((int32_t *) data);
}

int32_t create_socket_server(int32_t port_num)
{
    struct sockaddr_in client_addr;
    pthread_attr_t attr;
    pthread_t id;
    int32_t ret, clientfd, index_gfd, addrlen;

    syslog(LOG_INFO,
           "[SOFA] USERD INFO user daemon create socket server at port %d\n",
           port_num);
    _init_sock_srv(&sofa_sksrv, port_num);

    if ((ret = _create_sock_srv(&sofa_sksrv))) {
        return ret;
    }

    sofa_sksrv.running = 1;
    addrlen = sizeof(client_addr);

    while (sofa_sksrv.running) {
        clientfd = 0;
        index_gfd = -1;
        memset(&client_addr, 0, sizeof(struct sockaddr));

        /* Wait and accept connection */
        clientfd =
            accept(sofa_sksrv.sock_fd, (struct sockaddr *)&client_addr,
                   &addrlen);

        if (clientfd < 0) {
            syslog(LOG_ERR,
                   "[SOFA] USERD WARN fail to accept new conn %d running %d\n",
                   clientfd, sofa_sksrv.running);
            continue;
        }

        index_gfd = _add_new_client(&sofa_sksrv, clientfd);

        if (index_gfd == -1) {
            syslog(LOG_ERR,
                   "[SOFA] USERD WARN fail find empty slot client close conn\n");
            if (clientfd >= 0) {
                close(clientfd);
            }

            continue;
        }

        pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

        ret = pthread_create(&id, &attr, (void *)fn_cmd_handler,
                             (void *)(&(sofa_sksrv.client_sock_fd[index_gfd])));

        if (ret != 0) {
            syslog(LOG_ERR,
                   "[SOFA] USERD WARN Socket server create pthread err!\n");
            close(clientfd);
        }

        pthread_attr_destroy(&attr);
    }

    return ret;
}

void stop_socket_server(void)
{
    sofa_sksrv.running = 0;

    shutdown(sofa_sksrv.sock_fd, SHUT_RDWR);
    close(sofa_sksrv.sock_fd);
}
