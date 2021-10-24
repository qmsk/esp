#include "http/tcp.h"
#include "sock.h"
#include "tcp_internal.h"

#include "logging.h"

#include <errno.h>
#include <netdb.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

/*
 * Open a TCP socket connected to the given addr.
 */
int tcp_connect_async (int *sockp, struct addrinfo *addr)
{
    struct event *event = NULL;
    int sock;
    int err;

    if ((sock = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol)) < 0) {
        LOG_WARN("socket(%d, %d, %d): %s", addr->ai_family, addr->ai_socktype, addr->ai_protocol, strerror(errno));
        return 1;
    }

    err = sock_connect(sock, addr->ai_addr, addr->ai_addrlen);

    if (err > 0 && event) {
        LOG_ERROR("sock_connect: yield");
        goto error;

        err = sock_error(sock);
    }

    if (err) {
        LOG_WARN("sock_connect");
        goto error;
    }

    *sockp = sock;

error:
    if (err)
        close(sock);

    return err;
}

int tcp_connect (int *sockp, const char *host, const char *port)
{
    int err;
    struct addrinfo hints = {
        .ai_flags        = 0,
        .ai_family        = AF_UNSPEC,
        .ai_socktype    = SOCK_STREAM,
        .ai_protocol    = 0,
    };
    struct addrinfo *addrs, *addr;

    if ((err = getaddrinfo(host, port, &hints, &addrs))) {
#ifdef HAVE_GAI_STRERROR
        LOG_ERROR("getaddrinfo %s:%s: %s", host, port, gai_strerror(err));
#else
        LOG_ERROR("getaddrinfo %s:%s: %d", host, port, err);
#endif
        return -1;
    }

    // pre-set err in case of empty addrs
    err = 1;

    for (addr = addrs; addr; addr = addr->ai_next) {
        LOG_INFO("%s:%s: %s...", host, port, sockaddr_str(addr->ai_addr, addr->ai_addrlen));

        if ((err = tcp_connect_async(sockp, addr))) {
            LOG_ERROR("%s:%s: %s", host, port, sockaddr_str(addr->ai_addr, addr->ai_addrlen));
            continue;
        }

        LOG_INFO("%s:%s: %s <- %s", host, port, sockpeer_str(*sockp), sockname_str(*sockp));

        break;
    }

    freeaddrinfo(addrs);

    return err;
}

int tcp_client (struct tcp **tcpp, const char *host, const char *port, size_t stream_size)
{
    int sock;
    int err;

    if (tcp_connect(&sock, host, port)) {
        LOG_WARN("tcp_connect: %s:%s", host, port);
        return -1;
    }

    LOG_DEBUG("%d", sock);

    if ((err = tcp_create(tcpp, sock, stream_size))) {
      LOG_ERROR("tcp_create");
      close(sock);
      return err;
    }

    return 0;
}
