#include <httpserver/server.h>
#include "server.h"

#include <logging.h>

// string.h strdup
#ifndef _XOPEN_SOURCE
 #define _XOPEN_SOURCE 500
#endif

#include <stdlib.h>
#include <string.h>
#include <strings.h>

int http_server_create (struct http_server **serverp, int listen_backlog, size_t stream_size)
{
    struct http_server *server = NULL;

    if (!(server = calloc(1, sizeof(*server)))) {
        LOG_ERROR("calloc");
        return -1;
    }

    server->listen_backlog = listen_backlog;
    server->stream_size = stream_size;

    TAILQ_INIT(&server->listeners);

    *serverp = server;

    return 0;
}

int http_server_request (struct http_server *server, struct http_request *request, struct http_response *response, http_handler_func handler, void *ctx)
{
    enum http_status status = 0;
    int err;

    LOG_DEBUG("server=%p request=%p response=%p handler=%p ctx=%p", server, request, response, handler, ctx);

    // request
    if ((err = http_request_read(request)) < 0) {
        LOG_WARN("http_request_read");
        return -1;

    } else if (err == 1) {
        // EOF
        return 1;

    } else if (err > 0) {
        status = err;
        LOG_INFO("invalid HTTP request, return status=%d", status);
        goto response;
    }

    // handler
    if ((err = handler(request, response, ctx)) < 0) {
        LOG_WARN("http-handler");
        return err;
    } else if (err > 0) {
        LOG_DEBUG("handler returned %d", err);
        status = err;
    }

response:
    // finalize request
    if (http_request_close(request) < 0) {
      LOG_WARN("http_request_close");
      return -1;
    }

    unsigned response_status = http_response_get_status(response);

    if (!status && !response_status) {
        LOG_DEBUG("no response status sent or returned, assuming default HTTP 200 OK");
        status = 200;
    }

    LOG_DEBUG("return status=%d vs response status=%d", status, response_status);

    if (response_status == status) {
        LOG_DEBUG("keep response status=%d", status);
    } else if (response_status && status != response_status) {
        LOG_WARN("ignoring return status=%d as response already has status=%d set", status, response_status);
    } else if ((err = http_response_start(response, status, NULL))) {
        LOG_ERROR("http_response_start");
        return err;
    } else {
        LOG_DEBUG("sent response status=%d", status);
    }

    // returns >0 if connection closed
    return http_response_close(response) || http_request_closed(request);
}

int http_server_listen (struct http_server *server, const char *host, const char *port, struct http_listener **listenerp)
{
    struct http_listener *listener;
    int err;

    if (!(listener = calloc(1, sizeof(*listener)))) {
        LOG_ERROR("calloc");
        return -1;
    }

    listener->server = server;

    if ((err = tcp_server(&listener->tcp_server, host, port, server->listen_backlog, 0))) {
        LOG_ERROR("tcp_server");
        goto error;
    }

    TAILQ_INSERT_TAIL(&server->listeners, listener, server_listeners);

    *listenerp = listener;

    return 0;

error:
    if (listener->tcp_server)
        tcp_server_destroy(listener->tcp_server);

    free(listener);

    return err;
}

void http_server_destroy (struct http_server *server)
{
    // listeners
    struct http_listener *l;

    while ((l = TAILQ_FIRST(&server->listeners))) {
        TAILQ_REMOVE(&server->listeners, l, server_listeners);

        // TODO: kill TCP server to interrupt accept()
    }

    free(server);
}
