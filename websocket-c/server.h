/*
 * Copyright (c) 2024, Junior Rantila <junior.rantila@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once
#include <stdint.h>

char const* websocket_request_path(char const* request, uint64_t request_size);
int websocket_perform_handshake(char const* request, uint64_t request_size, int socket);
