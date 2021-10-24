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
    struct tcp_stream *tcp_stream;
    struct http *http;

    // per-request
    struct http_request request;
    struct http_response response;

    int err;
};

/** server.c */
/*
 * Read, process and respond to one request.
 *
 * @param request initialized http_request with http set
 * @param response initialized http_response with http set
 *
 * Return <0 on internal error, 0 on success with persistent connection, 1 on success with cconnection-close.
 */
int http_server_request (struct http_server *server, struct http_request *request, struct http_response *response, http_handler_func handler, void *ctx);

/** connection.c */
int http_connection_create (struct http_server *server, struct tcp_stream *tcp_stream, struct http_connection **connectionp);
