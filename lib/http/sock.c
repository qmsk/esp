#include "sock.h"
#include "logging.h"

#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>

#ifdef HAVE_SENDFILE
  #include <sys/sendfile.h>
#endif

int sockaddr_buf (char *buf, size_t buflen, const struct sockaddr *sa, socklen_t salen)
{
#ifdef HAVE_GETNAMEINFO
    char host[NI_MAXHOST];
    char serv[NI_MAXSERV];
    int err;

    if ((err = getnameinfo(sa, salen, host, sizeof(host), serv, sizeof(serv), NI_NUMERICHOST | NI_NUMERICSERV))) {
        LOG_WARN("getnameinfo: %s", gai_strerror(err));
        return -1;
    }

    snprintf(buf, buflen, "%s:%s", host, serv);

    return 0;
#else
    in_port_t port;
    const struct sockaddr_in *sa_inet;
    const struct sockaddr_in6 *sa_inet6;

    switch (sa->sa_family) {
      case AF_INET:
        sa_inet = (void *) sa;
        port = ntohs(sa_inet->sin_port);

        if (!inet_ntoa_r(sa_inet->sin_addr, buf, buflen)) {
          LOG_WARN("inet_ntop: %s", strerror(errno));
          return -1;
        }

        break;

      case AF_INET6:
        sa_inet6 = (void *) sa;
        port = ntohs(sa_inet6->sin6_port);

        if (!inet6_ntoa_r(sa_inet6->sin6_addr, buf, buflen)) {
          LOG_WARN("inet_ntop: %s", strerror(errno));
          return -1;
        }

        break;

      default:
        LOG_WARN("unknown sa_family=%d", sa->sa_family);

        return -1;
    }

    // append to end of string
    char *tail = buf;
    size_t taillen = buflen;

    while (*tail && taillen > 0) {
      tail++;
      taillen--;
    }

    if (snprintf(tail, taillen, ":%d", port) >= taillen) {
      LOG_WARN("snprintf overflow");
      return -1;
    }

    return 0;
#endif
}

const char *sockaddr_str (const struct sockaddr *sa, socklen_t salen)
{
    static char buf[SOCKADDR_MAX];

    if (sockaddr_buf(buf, sizeof(buf), sa, salen)) {
        return NULL;
    }

    return buf;
}

const char * sockname_str (int sock)
{
    static char buf[SOCKADDR_MAX];

    struct sockaddr_storage addr;
    socklen_t addrlen = sizeof(addr);

    if (getsockname(sock, (struct sockaddr *) &addr, &addrlen)) {
        LOG_WARN("getsockname(%d): %s", sock, strerror(errno));
        return NULL;
    }

    if (sockaddr_buf(buf, sizeof(buf), (struct sockaddr *) &addr, addrlen)) {
        return NULL;
    }

    return buf;
}

const char * sockpeer_str (int sock)
{
    static char buf[SOCKADDR_MAX];

    struct sockaddr_storage addr;
    socklen_t addrlen = sizeof(addr);

    if (getpeername(sock, (struct sockaddr *) &addr, &addrlen)) {
        LOG_WARN("getpeername(%d): %s", sock, strerror(errno));
        return NULL;
    }

    if (sockaddr_buf(buf, sizeof(buf), (struct sockaddr *) &addr, addrlen)) {
        return NULL;
    }

    return buf;

}

int sock_nonblocking (int sock)
{
    int flags;

    if ((flags = fcntl(sock, F_GETFL, 0)) < 0) {
        LOG_ERROR("fcntl(%d, F_GETFL, 0): %s", sock, strerror(errno));
        return -1;
    }

    if (fcntl(sock, F_SETFL, flags | O_NONBLOCK) < 0) {
        LOG_ERROR("fcntl(%d, F_SETFL, 0x%04x | O_NONBLOCK): %s", sock, flags, strerror(errno));
        return -1;
    }

    return 0;
}

int sock_connect (int sock, void *addr, size_t addrlen)
{
    int ret;

    ret = connect(sock, addr, addrlen);

    if (ret >= 0) {
        return 0;

    } else if (errno == EINPROGRESS) {
        return 1;

    } else {
        LOG_ERROR("connect: %s", strerror(errno));
        return -1;
    }
}

int sock_error (int sock)
{
    int out;
    socklen_t outlen = sizeof(out);

    if (getsockopt(sock, SOL_SOCKET, SO_ERROR, &out, &outlen) < 0) {
        LOG_ERROR("getsockopt: %s", strerror(errno));
        return -1;
    }

    if (outlen != sizeof(out)) {
        LOG_ERROR("getsockopt: outlen does not match");
        return -1;
    }

    LOG_DEBUG("%d", out);

    return out;
}

int sock_accept (int ssock, int *sockp)
{
    int sock;

    sock = accept(ssock, NULL, NULL);

    LOG_DEBUG("accept(%d): %d", ssock, sock);

    if (sock >= 0) {
        *sockp = sock;
        return 0;

    } else if (errno == EAGAIN || errno == EWOULDBLOCK) {
        return 1;

    } else {
        LOG_ERROR("accept: %s", strerror(errno));
        return -1;
    }
}

int sock_read (int sock, char *buf, size_t *sizep)
{
    int ret = read(sock, buf, *sizep);

    if (ret >= 0) {
        *sizep = ret;
        return 0;

    } else if (errno == EAGAIN || errno == EWOULDBLOCK) {
        return 1;

    } else {
        LOG_ERROR("read: %s", strerror(errno));
        return -1;
    }
}

int sock_write (int sock, const char *buf, size_t *sizep)
{
    int ret = write(sock, buf, *sizep);

    if (ret >= 0) {
        *sizep = ret;
        return 0;

    } else if (errno == EAGAIN || errno == EWOULDBLOCK) {
        return 1;

    } else {
        LOG_ERROR("write: %s", strerror(errno));
        return -1;
    }
}

#ifdef HAVE_SENDFILE
int sock_sendfile (int sock, int fd, size_t *sizep)
{
    int ret = sendfile(sock, fd, NULL, *sizep);

    if (ret > 0) {
        *sizep = ret;
        return 0;

    } else if (!ret) {
        LOG_DEBUG("eof");
        *sizep = 0;
        return 0;

    } else if (errno == EAGAIN || errno == EWOULDBLOCK) {
        return 1;

    } else {
        LOG_ERROR("sendfile: %s", strerror(errno));
        return -1;
    }
}
#endif
