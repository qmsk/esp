#include "tcp.h"
#include "tcp_internal.h"
#include "sock.h"
#include "stream.h"

#include "logging.h"

#include <unistd.h>

const struct timeval * maybe_timeout (const struct timeval *timeout)
{
    if (timeout->tv_sec || timeout->tv_usec)
        return timeout;
    else
        return NULL;
}

int tcp_stream_read (char *buf, size_t *sizep, void *ctx)
{
    struct tcp *tcp = ctx;
    int err;

    while ((err = sock_read(tcp->sock, buf, sizep)) > 0) {
      LOG_ERROR("sock_read: yield");
      return -1;
    }

    if (err) {
        LOG_ERROR("sock_read");
        return -1;
    }

    if (!*sizep) {
        LOG_DEBUG("eof");
        return 1;
    }

    return 0;
}

int tcp_stream_write (const char *buf, size_t *sizep, void *ctx)
{
    struct tcp *tcp = ctx;
    int err;

    while ((err = sock_write(tcp->sock, buf, sizep)) > 0) {
      LOG_ERROR("sock_write: yield");
      return -1;
    }

    if (err) {
        LOG_ERROR("sock_write");
        return -1;
    }

    if (!*sizep) {
        LOG_DEBUG("eof");
        return 1;
    }

    return 0;
}

#ifdef HAVE_SENDFILE
int tcp_stream_sendfile (int fd, size_t *sizep, void *ctx)
{
    struct tcp *tcp = ctx;
    int err;

    if (!*sizep) {
        // XXX: choose a random default
        *sizep = TCP_STREAM_SIZE;
    }

    while ((err = sock_sendfile(tcp->sock, fd, sizep)) > 0) {
      LOG_ERROR("sock_sendfile: yield");
      return -1;
    }

    if (err) {
        LOG_ERROR("sock_write");
        return -1;
    }

    if (!*sizep) {
        LOG_DEBUG("eof");
        return 1;
    }

    return 0;
}
#endif

static const struct stream_type tcp_stream_type = {
    .read       = tcp_stream_read,
    .write      = tcp_stream_write,
#ifdef HAVE_SENDFILE
    .sendfile   = tcp_stream_sendfile,
#endif
};

int tcp_create (struct tcp **tcpp, int sock, size_t stream_size)
{
    struct tcp *tcp = NULL;

    if (!(tcp = calloc(1, sizeof(*tcp)))) {
        LOG_ERROR("calloc");
        goto error;
    }

    tcp->sock = sock;

    if (stream_create(&tcp_stream_type, &tcp->read, stream_size, tcp)) {
        LOG_ERROR("stream_create read");
        goto error;
    }

    if (stream_create(&tcp_stream_type, &tcp->write, stream_size, tcp)) {
        LOG_ERROR("stream_create write");
        goto error;
    }

    *tcpp = tcp;

    return 0;

error:
    if (tcp) {
        tcp_destroy(tcp);
    } else {
        close(sock);
    }

    return -1;
}

int tcp_sock (struct tcp *tcp)
{
    return tcp->sock;
}

struct stream * tcp_read_stream (struct tcp *tcp)
{
    return tcp->read;
}

struct stream * tcp_write_stream (struct tcp *tcp)
{
    return tcp->write;
}

// TODO: implement for sock_read/write
void tcp_read_timeout (struct tcp *tcp, const struct timeval *timeout)
{
    tcp->read_timeout = *timeout;
}

// TODO: implement for sock_read/write
void tcp_write_timeout (struct tcp *tcp, const struct timeval *timeout)
{
    tcp->write_timeout = *timeout;
}

void tcp_destroy (struct tcp *tcp)
{
    if (tcp->write)
        stream_destroy(tcp->write);

    if (tcp->read)
        stream_destroy(tcp->read);

    if (tcp->sock >= 0)
        close(tcp->sock);

    free(tcp);
}
