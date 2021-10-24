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
