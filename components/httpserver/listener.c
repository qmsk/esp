#include <httpserver/server.h>
#include "server.h"

#include <logging.h>

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
