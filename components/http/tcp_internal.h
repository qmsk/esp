#ifndef TCP_INTERNAL_H
#define TCP_INTERNAL_H

#include "http/stream.h"

#include <stddef.h>
#include <sys/socket.h>

struct tcp {
    int sock;

    struct timeval read_timeout, write_timeout;

    struct stream *read, *write;
};

/*
 * Initialize an empty TCP connection for use with `tcp_reset(...)`
 */
int tcp_new (struct tcp **tcpp, size_t stream_size);

/*
 * Initialize a new TCP connection for use with its read/write streams.
 *
 * Does not close sock on errors.
 */
int tcp_create (struct tcp **tcpp, int sock, size_t stream_size);

/*
 * Re-Initialize TCP connection for use with new socket.
 *
 * Also resets read/write streams.
 */
void tcp_reset (struct tcp *tcpp, int sock);

#endif
