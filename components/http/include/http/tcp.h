#ifndef TCP_H
#define TCP_H

#include <stddef.h>
#include <sys/socket.h>

struct tcp_stream;
struct tcp_server;

enum {
    TCP_NONBLOCKING = 1,
};

/*
 * Open a TCP server socket, listening on the given host/port.
 *
 * host may be given as NULL to listen on all addresses.
 */
int tcp_listen (int *sockp, const char *host, const char *port, int backlog);

/*
 * Open a TCP client socket, connected to the given host/port.
 */
int tcp_connect (int *sockp, const char *host, const char *port);

/*
 * Run a server for accepting connections..
 */
int tcp_server (struct tcp_server **serverp, const char *host, const char *port, int backlog, int flags);

/*
 * Accept a new incoming request.
 * *
 * TODO: Return >0 on temporary per-client errors?
 */
int tcp_server_accept (struct tcp_server *server, struct tcp_stream **tcp_streamp, size_t stream_size, int flags);

/*
 * Release all resources.
 */
void tcp_server_destroy (struct tcp_server *server);

/*
 * Connect to a server..
 */
int tcp_client (struct tcp_stream **tcp_streamp, const char *host, const char *port, size_t stream_size);


/** TCP stream interface. */

 /*
  * Initialize an empty TCP connection for use with `tcp_reset(...)`
  */
int tcp_stream_new (struct tcp_stream **tcp_streamp, size_t stream_size);

 /*
  * Initialize a new TCP connection for use with its read/write streams.
  *
  * Does not close sock on errors.
  */
int tcp_stream_create (struct tcp_stream **tcp_streamp, int sock, size_t stream_size);

 /*
  * Re-Initialize TCP connection for use with new socket.
  *
  * Also resets read/write streams.
  */
void tcp_stream_reset (struct tcp_stream *tcp_stream, int sock);

int tcp_stream_sock (struct tcp_stream *tcp_stream);

struct stream * tcp_read_stream (struct tcp_stream *tcp_stream);
struct stream * tcp_write_stream (struct tcp_stream *tcp_stream);

/*
 * Set idle timeout for read operations.
 */
void tcp_stream_read_timeout (struct tcp_stream *tcp_stream, const struct timeval *timeout);
void tcp_stream_write_timeout (struct tcp_stream *tcp_stream, const struct timeval *timeout);

/*
 * Close TCP socket.
 */
 int tcp_stream_close (struct tcp_stream *tcp_stream);

/*
 * Close TCP socket, release stream buffers and resources.
 */
void tcp_stream_destroy (struct tcp_stream *tcp_stream);

#endif
