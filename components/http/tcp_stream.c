#include <http/tcp.h>
#include <http/stream.h>
#include "tcp_stream.h"
#include "sock.h"

#include <logging.h>

#include <errno.h>
#include <string.h>
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
    struct tcp_stream *tcp_stream = ctx;
    int err;

    if (tcp_stream->sock < 0) {
      LOG_ERROR("closed");
      return -1;
    }

    while ((err = sock_read(tcp_stream->sock, buf, sizep)) > 0) {
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
    struct tcp_stream *tcp_stream = ctx;
    int err;

    if (tcp_stream->sock < 0) {
      LOG_ERROR("closed");
      return -1;
    }

    while ((err = sock_write(tcp_stream->sock, buf, sizep)) > 0) {
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
    struct tcp_stream *tcp_stream = ctx;
    int err;

    if (!*sizep) {
        // XXX: choose a random default
        *sizep = TCP_STREAM_SIZE;
    }

    if (tcp_stream->sock < 0) {
      LOG_ERROR("closed");
      return -1;
    }

    while ((err = sock_sendfile(tcp_stream->sock, fd, sizep)) > 0) {
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

int tcp_stream_new (struct tcp_stream **tcp_streamp, size_t stream_size)
{
  struct tcp_stream *tcp_stream = NULL;

  if (!(tcp_stream = calloc(1, sizeof(*tcp_stream)))) {
    LOG_ERROR("calloc");
    return -1;
  }

  tcp_stream->sock = -1;

  if (stream_create(&tcp_stream_type, &tcp_stream->read, stream_size, tcp_stream)) {
    LOG_ERROR("stream_create read");
    goto error;
  }

  if (stream_create(&tcp_stream_type, &tcp_stream->write, stream_size, tcp_stream)) {
    LOG_ERROR("stream_create write");
    goto error;
  }

  *tcp_streamp = tcp_stream;

  return 0;

error:
  if (tcp_stream) {
    tcp_stream_destroy(tcp_stream);
  }

  return -1;
}

int tcp_stream_create (struct tcp_stream **tcp_streamp, int sock, size_t stream_size)
{
  int err;

  if ((err = tcp_stream_new(tcp_streamp, stream_size))) {
    LOG_ERROR("tcp_stream_new");
    return err;
  }

  (*tcp_streamp)->sock = sock;

  return 0;
}

void tcp_stream_reset (struct tcp_stream *tcp_stream, int sock)
{
  if (tcp_stream->sock >= 0) {
    LOG_WARN("force-closing sock=%d", tcp_stream->sock);
    close(tcp_stream->sock);
  }

  tcp_stream->sock = sock;

  stream_reset(tcp_stream->read);
  stream_reset(tcp_stream->write);
}

int tcp_stream_sock (struct tcp_stream *tcp_stream)
{
    return tcp_stream->sock;
}

struct stream * tcp_read_stream (struct tcp_stream *tcp_stream)
{
    return tcp_stream->read;
}

struct stream * tcp_write_stream (struct tcp_stream *tcp_stream)
{
    return tcp_stream->write;
}

// TODO: implement for sock_read/write
void tcp_stream_read_timeout (struct tcp_stream *tcp_stream, const struct timeval *timeout)
{
    tcp_stream->read_timeout = *timeout;
}

// TODO: implement for sock_read/write
void tcp_stream_write_timeout (struct tcp_stream *tcp_stream, const struct timeval *timeout)
{
    tcp_stream->write_timeout = *timeout;
}

int tcp_stream_close (struct tcp_stream *tcp_stream)
{
  if (tcp_stream->sock < 0) {
    return 0;
  }

  if (close(tcp_stream->sock)) {
    tcp_stream->sock = -1; // do not ever re-close!
    LOG_WARN("close: %s", strerror(errno));
    return -1;
  }

  tcp_stream->sock = -1;

  return 0;
}

void tcp_stream_destroy (struct tcp_stream *tcp_stream)
{
    if (tcp_stream->write)
        stream_destroy(tcp_stream->write);

    if (tcp_stream->read)
        stream_destroy(tcp_stream->read);

    if (tcp_stream->sock >= 0)
        close(tcp_stream->sock);

    free(tcp_stream);
}
