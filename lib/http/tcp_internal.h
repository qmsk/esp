#ifndef TCP_INTERNAL_H
#define TCP_INTERNAL_H

#include "stream.h"

#include <stddef.h>

struct tcp {
    int sock;

    struct timeval read_timeout, write_timeout;

    struct stream *read, *write;
};

/*
 * Initialize a new TCP connection for use with its read/write streams.
 */
int tcp_create (struct tcp **tcpp, int sock, size_t stream_size);

#endif
