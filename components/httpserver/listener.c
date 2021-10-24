#include <httpserver/server.h>
#include "server.h"

#include <logging.h>

int http_listener_accept (struct http_listener *listener, struct http_connection *connection)
{
  int err;

  if ((err = tcp_server_accept(listener->tcp_server, connection->tcp_stream, 0))) {
    LOG_ERROR("tcp_server_accept");
    return err;
  }

  return 0;
}

void http_listener_destroy (struct http_listener *listener)
{
  if (listener->tcp_server) {
    tcp_server_destroy(listener->tcp_server);
  }

  free(listener);
}
