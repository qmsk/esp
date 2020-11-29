#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include "handler.h"

/*
 * HTTP Server.
 */
struct http_server;
struct http_listener;
struct http_connection;

/*
 * Initialize a new server.
 *
 * @param listen_backlog TCP listen() backlog
 * @param stream_size TCP read/write stream buffer size; defines maximum request POST payload, response print size
 */
int http_server_create (struct http_server **serverp, int listen_backlog, size_t stream_size);

/*
 * Add a custom header to all responses.
 *
 * The given header/value are copied, and need not remain valid for the lifetime of the server.
 */
/*int http_server_add_header (struct http_server *server, const char *name, const char *value); TODO */

/*
 * Listen on given host/port
 */
int http_server_listen (struct http_server *server, const char *host, const char *port, struct http_listener **listenerp);

/*
 * Accept one client connection on listener.
 *
 * The caller must arrange for http_connection_serve() to be called on the connection.
 *
 * Returns 0 on successful connection, -1 on error.
 */
int http_listener_accept (struct http_listener *listener, struct http_connection **connectionp);

/*
 * Process one client request on connection.
 *
 * Return <0 on internal error, 0 on success with persistent connection, 1 on success with cconnection-close.
 */
int http_connection_serve (struct http_connection *connection, http_handler_func handler, void *ctx);

/* Cleanup connection */
void http_connection_destroy (struct http_connection *connection);

/* Cleanup listener */
void http_listener_destroy (struct http_listener *listener);

/*
 * Release all resources for server.
 */
void http_server_destroy (struct http_server *server);


#endif
