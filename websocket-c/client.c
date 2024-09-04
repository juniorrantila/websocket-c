/*
 * Copyright (c) 2024, Junior Rantila <junior.rantila@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "./client.h"

#include <stdint.h>
#include <string.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct {
    char const* protocol;
    char const* domain;
    char const* port;
    char const* slug;
} URL;
static int parse_url(char const*, URL* url);
static void free_url(URL*);

typedef struct {
    char const** headers;
    uint64_t headers_length;
    int status_code;
} HttpResponse;

static int recv_http_response(int socket, HttpResponse*);
static void free_http_response(HttpResponse*);

static int request_protocol_switch(int socket, char const* slug);

int websocket_connect(char const* raw_url)
{
    URL url = { 0 };
    if (parse_url(raw_url, &url) < 0) {
        return -1;
    }
    if (strcmp(url.protocol, "ws") != 0) {
        free_url(&url);
        return errno=ENOTSUP, -1;
    }

    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    struct addrinfo* res = 0;
    if (getaddrinfo(url.domain, url.port, &hints, &res) < 0) {
        goto fi_1;
    }

    int fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (fd < 0) {
        goto fi_2;
    }

    if (connect(fd, res->ai_addr, res->ai_addrlen) < 0) {
        goto fi_3;
    }

    if (request_protocol_switch(fd, url.slug) < 0) {
        goto fi_3;
    }

    freeaddrinfo(res);
    free_url(&url);
    return fd;
fi_3:
    close(fd);
fi_2:
    freeaddrinfo(res);
fi_1:
    free_url(&url);
    return -1;
}

static int request_protocol_switch(int socket, char const* slug)
{
    (void)socket;
    (void)slug;
    char const* sec_websocket_key = "";
    char const* expected_sec_websocket_accept = "";
    (void)expected_sec_websocket_accept;

    char* message = 0;
    int message_len = asprintf(&message,
            "GET %s HTTP/1.1\r\nConnection: Upgrade\r\nUpgrade: websocket\r\nSec-WebSocket-Key: %s\r\n\r\n",
            slug, sec_websocket_key);
    if (message == 0) {
        return -1;
    }

    if (send(socket, message, message_len, 0) != message_len) {
        goto fi_1;
    }

    HttpResponse response = { 0 };
    if (recv_http_response(socket, &response) < 0) {
        goto fi_1;
    }

    free_http_response(&response);
    free(message);
    return 0;
fi_1:
    free(message);
    return -1;
}

static int parse_url(char const* raw_url, URL* url_out)
{
    (void)raw_url;
    (void)url_out;
    return -1;
}

static void free_url(URL* url)
{
    free((void*)url->protocol);
    free((void*)url->domain);
    free((void*)url->port);
    free((void*)url->slug);
    memset(url, 0, sizeof(*url));
}

static int recv_http_response(int socket, HttpResponse* response_out)
{
    (void)socket;
    (void)response_out;
    return -1;
}

static void free_http_response(HttpResponse* response)
{
    for (uint64_t i = 0; i < response->headers_length; i++) {
        free((void*)response->headers[i]);
    }
    free(response->headers);
    memset(response, 0, sizeof(*response));
}
