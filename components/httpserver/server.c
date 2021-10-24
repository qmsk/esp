#include <httpserver/server.h>
#include <http/http.h>
#include <http/tcp.h>

#include "request.h"
#include "response.h"

#include <logging.h>

// string.h strdup
#ifndef _XOPEN_SOURCE
 #define _XOPEN_SOURCE 500
#endif

#include <stdlib.h>
#include <string.h>
#include <strings.h>
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

/* Idle timeout used for client reads; reset on every read operation */
static const struct timeval server_read_timeout = { .tv_sec = 10 };

/* Idle timeout used for client write buffering; reset on every write operation */
static const struct timeval server_write_timeout = { .tv_sec = 10 };

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

/*
 * Read, process and respond to one request.
 *
 * @param request initialized http_request with http set
 * @param response initialized http_response with http set
 *
 * Return <0 on internal error, 0 on success with persistent connection, 1 on success with cconnection-close.
 */
static int http_server_request (struct http_server *server, struct http_request *request, struct http_response *response, http_handler_func handler, void *ctx)
{
    enum http_status status = 0, response_status;
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

    if (!status && !http_response_get_status(response)) {
        LOG_WARN("no response status sent or returned, assuming default HTTP 200 OK");
        status = 200;
    }

    LOG_DEBUG("response status=%d", status);

    if (!status) {

    } else if ((response_status = http_response_get_status(response))) {
        LOG_WARN("ignoring response status=%d as response already has status=%d set", status, response_status);
    } else if ((err = http_response_start(response, status, NULL))) {
        LOG_WARN("http_response_start");
        return -1;
    } else {
        LOG_DEBUG("sent response status=%d", status);
    }

    // returns >0 if connection closed
    return http_response_close(response) || http_request_closed(request);
}

static int http_connection_create (struct http_server *server, struct tcp_stream *tcp_stream, struct http_connection **connectionp)
{
    struct http_connection *connection;
    int err = 0;

    if (!(connection = calloc(1, sizeof(*connection)))) {
        LOG_ERROR("calloc");
        return -1;
    }

    connection->server = server;
    connection->tcp_stream = tcp_stream;

    if ((err = http_create(&connection->http, tcp_read_stream(tcp_stream), tcp_write_stream(tcp_stream)))) {
        LOG_ERROR("http_create");
        goto error;
    }

    *connectionp = connection;

    return 0;

error:
    if (connection->http)
        http_destroy(connection->http);

    free(connection);

    return -1;
}

/*
 * Handle one client.
 */
int http_connection_serve (struct http_connection *connection, const struct http_hooks *hooks, http_handler_func handler, void *ctx)
{
    int err;

    LOG_DEBUG("connection=%p", connection);

    // set idle timeouts
    tcp_stream_read_timeout(connection->tcp_stream, &server_read_timeout);
    tcp_stream_write_timeout(connection->tcp_stream, &server_write_timeout);

    // reset request/response state...
    connection->request = (struct http_request) {
      .http     = connection->http,
      .response = &connection->response,
      .hooks    = hooks,
    };
    connection->response = (struct http_response) {
      .http     = connection->http,
      .request  = &connection->request,
      .hooks    = hooks,
    };

    if ((err = http_server_request(connection->server, &connection->request, &connection->response, handler, ctx)) < 0) {
        LOG_WARN("http_server_request");
    } else if (err) {
        LOG_DEBUG("end of client requests");
    }

    return err;
}

void http_connection_destroy (struct http_connection *connection)
{
  if (connection->http)
      http_destroy(connection->http);

  // TODO: clean close vs reset?
  tcp_stream_destroy(connection->tcp_stream);

  free(connection);
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

int http_listener_accept (struct http_listener *listener, struct http_connection **connectionp)
{
    struct tcp_stream *tcp_stream;
    int err;

    if ((err = tcp_stream_new(&tcp_stream, listener->server->stream_size))) {
      LOG_ERROR("tcp_stream_new");
      return -1;
    }

    if ((err = tcp_server_accept(listener->tcp_server, tcp_stream, 0))) {
        LOG_ERROR("tcp_server_accept");
        goto error;
    }

    LOG_DEBUG("tcp_stream=%p", tcp_stream);

    if ((err = http_connection_create(listener->server, tcp_stream, connectionp))) {
        LOG_ERROR("http_server_connection");
        goto error;
    }

    return 0;

error:
    tcp_stream_destroy(tcp_stream);

    return err;
}

void http_listener_destroy (struct http_listener *listener)
{
    tcp_server_destroy(listener->tcp_server);
    free(listener);
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
