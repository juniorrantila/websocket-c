/*
 * Copyright (c) 2024, Junior Rantila <junior.rantila@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "./websocket.h"

#include <sys/socket.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    uint8_t opcode;
    bool is_final;
} WebSocketHeader;

enum Opcode {
    Opcode_Continuation = 0x00,
    Opcode_Text = 0x01,
    Opcode_Binary = 0x02,
    Opcode_Close = 0x08,
    Opcode_Ping = 0x09,
    Opcode_Pong = 0x0A,
};

static int recv_header(int socket, WebSocketHeader* header_out);
static int recv_payload_length(int socket, uint64_t* length_out);
static int recv_masking_key(int socket, uint8_t key[static 4]);
static int send_length(int socket, uint64_t length);
static int send_frame(int socket, WebSocketFrame frame);

bool websocket_frame_ready(int socket)
{
    return recv(socket, 0, 1, MSG_PEEK) == 1;
}

int websocket_recv_frame(int socket, WebSocketFrame* frame)
{
    WebSocketHeader header = { 0 };
    if (recv_header(socket, &header) < 0) {
        return -1;
    }
    
    uint64_t payload_length = 0;
    if (recv_payload_length(socket, &payload_length) < 0) {
        return -1;
    }
    
    uint8_t masking_key[4];
    if (recv_masking_key(socket, masking_key) < 0) {
        return -1;
    }

    if (header.opcode == Opcode_Continuation) {
        // FIXME: Implement continuation.
        return errno=ENOTSUP, -1;
    }

    uint8_t* payload = (uint8_t*)malloc(payload_length);
    if (payload == 0) {
        return -1;
    }
    if (recv(socket, payload, payload_length, 0) != (int64_t)payload_length) {
        free(payload);
        return -1;
    }
    for (uint64_t i = 0; i < payload_length; i++) {
        payload[i] = payload[i] ^ masking_key[i % 4];
    }

    if (header.opcode == Opcode_Text) {
        *frame = (WebSocketFrame) {
            .text = {
                .characters = (char const*)payload,
                .length = payload_length
            },
            .kind = WebSocketFrameKind_Text
        };
        return 0;
    }

    if (header.opcode == Opcode_Binary) {
        *frame = (WebSocketFrame) {
            .binary = {
                .bytes = payload,
                .length = payload_length
            },
            .kind = WebSocketFrameKind_Binary
        };
        return 0;
    }

    if (!header.is_final) {
        free(payload);
        return errno=ENOTSUP, -1;
    }

    if (payload_length > 125) {
        free(payload);
        return errno=ENOTSUP, -1;
    }

    if (header.opcode == Opcode_Close) {
        uint16_t reason = *(uint16_t*)payload;
        free(payload);
        *frame = (WebSocketFrame) {
            .close = {
                .reason = ntohs(reason)
            },
            .kind = WebSocketFrameKind_Close
        };
        return 0;
    }

    if (header.opcode == Opcode_Ping) {
        *frame = (WebSocketFrame) {
            .ping = {
                .payload = payload,
                .length = payload_length
            },
            .kind = WebSocketFrameKind_Ping
        };
        return 0;
    }

    if (header.opcode == Opcode_Pong) {
        *frame = (WebSocketFrame) {
            .pong = {
                .payload = payload,
                .length = payload_length
            },
            .kind = WebSocketFrameKind_Pong
        };
        return 0;
    }

    return errno=ENOTSUP, -1;
}

int websocket_send_text(int socket, char const* characters, uint64_t characters_length)
{
    return send_frame(socket, (WebSocketFrame) {
         .text = {
            .characters = characters,
            .length = characters_length,
         },
        .kind = WebSocketFrameKind_Text
    });
}

int websocket_send_binary(int socket, void const* bytes, uint64_t bytes_length)
{
    return send_frame(socket, (WebSocketFrame) {
         .binary = {
            .bytes = bytes,
            .length = bytes_length,
         },
        .kind = WebSocketFrameKind_Binary
    });
}

int websocket_send_ping(int socket, void const* payload, uint64_t payload_byte_length)
{
    return send_frame(socket, (WebSocketFrame) {
         .ping = {
            .payload = payload,
            .length = payload_byte_length,
         },
        .kind = WebSocketFrameKind_Ping
    });
}

int websocket_send_pong(int socket, void const* payload, uint64_t payload_byte_length)
{
    return send_frame(socket, (WebSocketFrame) {
         .pong = {
            .payload = payload,
            .length = payload_byte_length,
         },
        .kind = WebSocketFrameKind_Pong
    });
}

int websocket_send_close(int socket, uint16_t reason)
{
    return send_frame(socket, (WebSocketFrame) {
         .close = {
            .reason = reason,
         },
        .kind = WebSocketFrameKind_Close
    });
}

void websocket_destroy_frame(WebSocketFrame* frame)
{
    switch (frame->kind) {
    case WebSocketFrameKind_Binary:
        if (frame->binary.bytes) {
            free((void*)frame->binary.bytes);
        }
        break;
    case WebSocketFrameKind_Text:
        if (frame->text.characters) {
            free((void*)frame->text.characters);
        }
        break;
    case WebSocketFrameKind_Ping:
        if (frame->ping.payload) {
            free((void*)frame->ping.payload);
        }
        break;
    case WebSocketFrameKind_Pong:
        if (frame->pong.payload) {
            free((void*)frame->pong.payload);
        }
        break;
    case WebSocketFrameKind_Invalid:
    case WebSocketFrameKind_Close:
        break;
    }
    memset(frame, 0, sizeof(*frame));
}

static int recv_header(int socket, WebSocketHeader* header_out)
{
    uint8_t lead = 0;
    if (recv(socket, &lead, 1, 0) != 1) {
        return -1;
    }

    // The reserve bits MUST be 0 unless dictated by extensions and as
    // we do not implement any extensions that specify a non-zero value,
    // the connection will be closed if any of the reserve bits are set.
    if (lead & 0x70) {
        return -1;
    }

    *header_out = (WebSocketHeader) {
        .opcode = lead & 0x7f,
        .is_final = (lead & 0x80) > 0,
    };
    return 0;
}

static int recv_payload_length(int socket, uint64_t* length_out)
{
    uint8_t initial_length = 0;
    if (recv(socket, &initial_length, sizeof(initial_length), 0) != sizeof(initial_length)) {
        return -1;
    }

    // This will almost certainly be 1 when coming from the client
    // as is defined in the specification.
    bool masked = initial_length & 0x80;

    // Per section 5.1 of the specification
    if (!masked) {
        return -1;
    }

    uint64_t payload_length = initial_length & 0x7f;

    if (payload_length == 126) {
        uint16_t new_payload_length = 0;
        if (recv(socket, &new_payload_length, sizeof(new_payload_length), 0) != sizeof(new_payload_length)) {
            return -1;
        }
        return *length_out = ntohs(new_payload_length), 0;
    }

    if (payload_length == 127) {
        uint64_t new_payload_length = 0;
        if (recv(socket, &new_payload_length, sizeof(new_payload_length), 0) != sizeof(new_payload_length)) {
            return -1;
        }
        return *length_out = ntohll(new_payload_length), 0;
    }

    return *length_out = payload_length, 0;
}

static int recv_masking_key(int socket, uint8_t key[static 4])
{
    if (recv(socket, key, 4, 0) != 4) {
        return -1;
    }
    return 0;
}

static int send_length(int socket, uint64_t length)
{
    bool is_masked = false;

    if (length <= 125) {
        uint8_t first_byte = (is_masked << 7) | length;
        if (send(socket, &first_byte, sizeof(first_byte), 0) != sizeof(first_byte)) {
            return -1;
        }
        return 0;
    }

    if (length <= 65535) {
        uint8_t first_byte = (is_masked << 7) | 126;
        if (send(socket, &first_byte, sizeof(first_byte), 0) != sizeof(first_byte)) {
            return -1;
        }

        uint16_t mask = (1 << 16) - 1; // Generate a bit mask for only the last 16 bits.
        uint16_t payload_length = htons(length & mask);
        if (send(socket, &payload_length, sizeof(payload_length), 0) != sizeof(payload_length)) {
            return -1;
        }
        return 0;
    }

    uint8_t first_byte = (is_masked << 7) | 127;
    if (send(socket, &first_byte, sizeof(first_byte), 0) != sizeof(first_byte)) {
        return -1;
    }

    uint64_t payload_length = htonll(length);
    if (send(socket, &payload_length, sizeof(payload_length), 0) != sizeof(payload_length)) {
        return -1;
    }
    return 0;
}

static int send_frame(int socket, WebSocketFrame frame)
{
    if (frame.kind == WebSocketFrameKind_Invalid) {
        return errno=EINVAL, -1;
    }

    bool is_finished = true;
    uint8_t fin_and_reserved = (is_finished << 3) | 0;

    uint8_t opcode = 0;
    void const* payload = 0;
    uint64_t payload_length = 0;
    switch (frame.kind) {
    case WebSocketFrameKind_Invalid:
        break;
    case WebSocketFrameKind_Text:
        opcode = Opcode_Text;
        payload = frame.text.characters;
        payload_length = frame.text.length;
        break;
    case WebSocketFrameKind_Binary:
        opcode = Opcode_Binary;
        payload = frame.binary.bytes;
        payload_length = frame.binary.length;
        break;
    case WebSocketFrameKind_Ping:
        opcode = Opcode_Ping;
        payload = frame.ping.payload;
        payload_length = frame.ping.length;
        break;
    case WebSocketFrameKind_Pong:
        opcode = Opcode_Pong;
        payload = frame.pong.payload;
        payload_length = frame.pong.length;
        break;
    case WebSocketFrameKind_Close:
        opcode = Opcode_Close;
        payload = &frame.close.reason;
        payload_length = sizeof(frame.close.reason);
        break;
    }

    uint8_t lead = (fin_and_reserved << 4) | opcode;
    if (send(socket, &lead, sizeof(lead), 0) != sizeof(lead)) {
        return -1;
    }

    if (send_length(socket, payload_length) < 0) {
        return -1;
    }

    if (payload_length == 0) {
        return 0;
    }

    if (send(socket, payload, payload_length, 0) < 0) {
        return -1;
    }

    return 0;
}
