/*
 * Copyright (c) 2024, Junior Rantila <junior.rantila@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    WebSocketFrameKind_Invalid = 0,
    WebSocketFrameKind_Text,
    WebSocketFrameKind_Binary,
    WebSocketFrameKind_Ping,
    WebSocketFrameKind_Pong,
    WebSocketFrameKind_Close,
} WebSocketFrameKind;

typedef struct {
    char const* characters;
    uint64_t length;
} WebSocketTextFrame;

typedef struct {
    void const* bytes;
    uint64_t length;
} WebSocketBinaryFrame;

typedef struct {
    void const* payload;
    uint64_t length;
} WebSocketPingFrame;

typedef struct {
    void const* payload;
    uint64_t length;
} WebSocketPongFrame;

typedef struct {
    uint16_t reason;
} WebSocketCloseFrame;

typedef struct {
    union {
        WebSocketTextFrame text;
        WebSocketBinaryFrame binary;
        WebSocketPingFrame ping;
        WebSocketPongFrame pong;
        WebSocketCloseFrame close;
    };
    WebSocketFrameKind kind;
} WebSocketFrame;

typedef enum {
    WebSocketCloseReason_Normal = 1000,
    WebSocketCloseReason_GoingAway = 1001,
    WebSocketCloseReason_ProtocolError = 1002,
    WebSocketCloseReason_CannotAccept = 1003,
    WebSocketCloseReason_InvalidDate = 1007,
    WebSocketCloseReason_PolicyViolation = 1008,
    WebSocketCloseReason_TooBig = 1009,
    WebSocketCloseReason_InsufficientExtensions = 1010,
    WebSocketCloseReason_InternalServerError = 1011,
} WebSocketCloseReason;

bool websocket_frame_ready(int socket);
int websocket_recv_frame(int socket, WebSocketFrame* frame);
void websocket_destroy_frame(WebSocketFrame* frame);

int websocket_send_text(int socket, char const* characters, uint64_t characters_length);
int websocket_send_binary(int socket, void const* bytes, uint64_t bytes_length);
int websocket_send_ping(int socket, void const* payload, uint64_t payload_byte_length);
int websocket_send_pong(int socket, void const* payload, uint64_t payload_byte_length);
int websocket_send_close(int socket, uint16_t reason);

#ifdef __cplusplus
}
#endif
