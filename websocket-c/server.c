/*
 * Copyright (c) 2024, Junior Rantila <junior.rantila@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "./server.h"

char const* websocket_request_path(char const* request, uint64_t request_size)
{
    (void)request;
    (void)request_size;
    return 0;
}

int websocket_perform_handshake(char const* request, uint64_t request_size, int socket)
{
    (void)request;
    (void)request_size;
    (void)socket;
    return -1;
}
