/*
 * Copyright (c) 2024, Junior Rantila <junior.rantila@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <websocket-c/websocket.h>
#include <websocket-c/server.h>

#include <stdio.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>

static int create_server(void);
static int recv_http_request(int client_fd, char const**, uint64_t*);

static void handle_request(int socket)
{
    char const* request = 0;
    uint64_t request_length = 0;
    if (recv_http_request(socket, &request, &request_length) < 0) {
        perror("could not read request");
        return;
    }

    char const* path = websocket_request_path(request, request_length);
    if (strcmp(path, "/ws") != 0) {
        return;
    }

    if (websocket_perform_handshake(request, request_length, socket) < 0) {
        perror("could not perform handshake");
        return;
    }

    int should_continue = 1;
    while (should_continue) {
        if (!websocket_frame_ready(socket)) {
            websocket_send_text(socket, "Hello", 5);
            usleep(10000);
            continue;
        }

        WebSocketFrame frame = { 0 };
        if (websocket_recv_frame(socket, &frame) < 0) {
            break;
        }

        if (frame.kind == WebSocketFrameKind_Close) {
            should_continue = 0;
        }

        websocket_destroy_frame(&frame);
    }
}

int main(void) {
    int server_fd = create_server();
    if (server_fd < 0) {
        perror("could not create server");
    }

    while (true) {
        int client_fd = accept(server_fd, 0, 0);
        int child_pid = fork();
        if (child_pid > 0) {
            continue;
        }

        handle_request(client_fd);
        close(client_fd);
        return 0;
    }

    return 0;
}

static int create_server(void)
{
    return -1;
}

static int recv_http_request(int client_fd, char const** request_out, uint64_t* length_out)
{
    (void)client_fd;
    (void)request_out;
    (void)length_out;
    return -1;
}
