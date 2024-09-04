/*
 * Copyright (c) 2024, Junior Rantila <junior.rantila@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <websocket-c/websocket.h>
#include <websocket-c/client.h>

#include <unistd.h>
#include <stdio.h>

int main(void) {
    int socket = websocket_connect("ws://localhost:3000/ws");
    if (socket < 0) {
        perror("could not connect to websocket");
        return -1;
    }

    int should_continue = 1;
    while (should_continue) {
        if (!websocket_frame_ready(socket)) {
            usleep(10000);
            continue;
        }
        WebSocketFrame frame = { 0 };
        if (websocket_recv_frame(socket, &frame) < 0) {
            break;
        }

        switch (frame.kind) {
        case WebSocketFrameKind_Close:
            should_continue = 0;
            break;
        case WebSocketFrameKind_Text:
            printf("Got message: %.*s\n", (int)frame.text.length, frame.text.characters);
            break;
        default: break;
        }

        websocket_destroy_frame(&frame);
    }

    websocket_close(socket);
    return 0;
}
