#pragma once

#include <httpserver/server.h>
#include <http/http.h>
#include <http/tcp.h>

#include "request.h"
#include "response.h"

#include <sys/queue.h>

struct http_server {
    int listen_backlog;
    size_t stream_size;

    /* Listen tasks */
    TAILQ_HEAD(server_listeners, http_listener) listeners;
};

struct http_listener {
    struct http_server *server;
    struct tcp_server *tcp_server;

    // TODO
    TAILQ_ENTRY(http_listener) server_listeners;
};

struct http_connection {
    struct http_server *server;

    // owned
    struct tcp_stream *tcp_stream;
    struct http *http;

    // per-request
    struct http_request request;
    struct http_response response;
};
