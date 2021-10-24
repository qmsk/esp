#pragma once

#include "http/stream.h"
#include "http/tcp.h"

#include <stddef.h>
#include <sys/socket.h>

struct tcp_stream {
    int sock;

    struct timeval read_timeout, write_timeout;

    struct stream *read, *write;
};

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
