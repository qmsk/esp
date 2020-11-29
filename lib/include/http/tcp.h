#ifndef TCP_H
#define TCP_H

#include <stddef.h>
#include <sys/socket.h>

struct tcp;
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
int tcp_server_accept (struct tcp_server *server, struct tcp **tcpp, size_t stream_size, int flags);

/*
 * Release all resources.
 */
void tcp_server_destroy (struct tcp_server *server);

/*
 * Connect to a server..
 */
int tcp_client (struct tcp **tcpp, const char *host, const char *port, size_t stream_size);

/*
 * TCP connection interface.
 */
int tcp_sock (struct tcp *tcp);
struct stream * tcp_read_stream (struct tcp *tcp);
struct stream * tcp_write_stream (struct tcp *tcp);

/*
 * Set idle timeout for read operations.
 */
void tcp_read_timeout (struct tcp *tcp, const struct timeval *timeout);
void tcp_write_timeout (struct tcp *tcp, const struct timeval *timeout);

void tcp_destroy (struct tcp *tcp);

#endif
