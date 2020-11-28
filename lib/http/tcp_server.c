#include "tcp_internal.h"
#include "sock.h"

#include "http/tcp.h"
#include "http/stream.h"

#include "logging.h"

#include <errno.h>
#include <netdb.h>
#include <stdbool.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

struct tcp_server {
    int sock;
};

int tcp_listen (int *sockp, const char *host, const char *port, int backlog)
{
    int err;
    struct addrinfo hints = {
#ifdef HAVE_AI_PASSIVE
        .ai_flags       = AI_PASSIVE,
#endif
        .ai_family      = AF_UNSPEC,
        .ai_socktype    = SOCK_STREAM,
        .ai_protocol    = 0,
    };
    struct addrinfo *addrs, *addr;
    int sock = -1;

    // translate empty string to NULL
    if (!host || !*host)
        host = NULL;

    if ((err = getaddrinfo(host, port, &hints, &addrs))) {
#ifdef HAVE_GAI_STRERROR
        LOG_ERROR("getaddrinfo %s:%s: %s", host, port, gai_strerror(err));
#else
        LOG_ERROR("getaddrinfo %s:%s: %d", host, port, err);
#endif
        return -1;
    }

    for (addr = addrs; addr; addr = addr->ai_next) {
        if ((sock = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol)) < 0) {
            LOG_WARN("socket(%d, %d, %d): %s", addr->ai_family, addr->ai_socktype, addr->ai_protocol, strerror(errno));
            continue;
        }

        LOG_DEBUG("sock=%d bind %s", sock, sockaddr_str(addr->ai_addr, addr->ai_addrlen));

        // bind to listen address/port
        if ((err = bind(sock, addr->ai_addr, addr->ai_addrlen)) < 0) {
            LOG_WARN("bind: %s", strerror(errno));
            close(sock);
            sock = -1;
            continue;
        }

        break;
    }

    freeaddrinfo(addrs);

    if (sock < 0)
        return -1;


    LOG_INFO("%s", sockname_str(sock));
    LOG_DEBUG("sock=%d listen backlog=%d", sock, backlog);

    // mark as listening
    if ((err = listen(sock, backlog))) {
        LOG_ERROR("listen: %s", strerror(errno));
        close(sock);
        sock = -1;
    }

    if (sock < 0)
        return -1;

    *sockp = sock;

    return 0;
}

int tcp_server (struct tcp_server **serverp, const char *host, const char *port, int backlog, int flags)
{
    struct tcp_server *server;
    int err;

    if (!(server = calloc(1, sizeof(*server)))) {
        LOG_ERROR("calloc");
        return -1;
    }

    LOG_DEBUG("server=%p listen host=%s port=%s", server, host, port);

    if ((err = tcp_listen(&server->sock, host, port, backlog))) {
        LOG_ERROR("tcp_listen %s:%s", host, port);
        goto error;
    }

    if (flags & TCP_NONBLOCKING) {
      LOG_DEBUG("sock_nonblocking sock=%d", server->sock);

      if ((err = sock_nonblocking(server->sock))) {
          LOG_ERROR("sock_nonblocking");
          goto error;
      }
    }

    LOG_DEBUG("server=%p sock=%d", server, server->sock);

    // ok
    *serverp = server;
    return 0;

error:
    free(server);
    return err;
}

int tcp_server_accept (struct tcp_server *server, struct tcp **tcpp, size_t stream_size, int flags)
{
    int err;
    int sock;

    LOG_DEBUG("server=%p ssock=%d accept", server, server->sock);

    while ((err = sock_accept(server->sock, &sock)) != 0) {
        // handle various error cases
        if (err < 0 && (errno == EMFILE || errno == ENFILE)) {
            // TODO: sleep and retry?
            LOG_WARN("temporary accept failure");
            return err;

        } else if (err < 0) {
            LOG_ERROR("sock_accept");
            return -1;

        } else {
            LOG_ERROR("sock_accept: yield");
            return -1;
        }
    }

    LOG_DEBUG("server=%p ssock=%d accept: sock=%d", server->sock, sock);
    LOG_INFO("%s accept %s", sockname_str(sock), sockpeer_str(sock));

    if (flags & TCP_NONBLOCKING) {
      LOG_DEBUG("sock_nonblocking sock=%d", sock);

      if ((err = sock_nonblocking(sock))) {
          LOG_ERROR("sock_nonblocking");
          close(sock);
          return -1;
      }
    }

    if (tcp_create(tcpp, sock, stream_size)) {
        LOG_ERROR("tcp_create");
        return -1;
    }

    return 0;
}

void tcp_server_destroy (struct tcp_server *server)
{
    if (server->sock >= 0)
        close(server->sock);

    free(server);
}
