#include <httpserver/server.h>
#include "server.h"

#include <logging.h>

/* Idle timeout used for client reads; reset on every read operation */
static const struct timeval server_read_timeout = { .tv_sec = 10 };

/* Idle timeout used for client write buffering; reset on every write operation */
static const struct timeval server_write_timeout = { .tv_sec = 10 };

int http_connection_new (struct http_server *server, struct http_connection **connectionp)
{
    struct http_connection *connection;
    int err = 0;

    if (!(connection = calloc(1, sizeof(*connection)))) {
        LOG_ERROR("calloc");
        return -1;
    }

    connection->server = server;

    if ((err = tcp_stream_new(&connection->tcp_stream, server->stream_size))) {
        LOG_ERROR("tcp_stream_new");
        goto error;
    }

    if ((err = http_create(&connection->http, tcp_read_stream(connection->tcp_stream), tcp_write_stream(connection->tcp_stream)))) {
        LOG_ERROR("http_create");
        goto error;
    }

    *connectionp = connection;

    return 0;

error:
    if (connection->http)
        http_destroy(connection->http);

    if (connection->tcp_stream)
        tcp_stream_destroy(connection->tcp_stream);

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

int http_connection_close (struct http_connection *connection)
{
  int err;

  http_reset(connection->http);

  if ((err = tcp_stream_close(connection->tcp_stream))) {
    LOG_WARN("tcp_stream_close");
    return err;
  }

  return 0;
}

void http_connection_destroy (struct http_connection *connection)
{
  if (connection->http)
      http_destroy(connection->http);

  if (connection->tcp_stream)
    tcp_stream_destroy(connection->tcp_stream);

  free(connection);
}
