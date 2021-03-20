#include "artnet.h"

#include <logging.h>

#include <errno.h>
#include <string.h>
#include <lwip/sockets.h>

int artnet_listen(int *sockp, uint16_t port)
{
  struct sockaddr_in bind_addr = {
    .sin_family = AF_INET,
    .sin_port = htons(port),
    .sin_addr = { INADDR_ANY },
  };
  int sock;

  if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
    LOG_ERROR("socket: %s", strerror(errno));
    return -1;
  }

  if (bind(sock, (struct sockaddr *) &bind_addr, sizeof(bind_addr)) < 0) {
    LOG_ERROR("bind: %s", strerror(errno));
    close(sock);
    return -1;
  }

  LOG_INFO("port=%u: socket=%d", port, sock);

  *sockp = sock;

  return 0;
}

int artnet_send(int sock, const struct artnet_sendrecv *send)
{
  LOG_DEBUG("op=%04x len=%u", send->packet->header.opcode, send->len);

  if (sendto(sock, send->packet, send->len, 0, &send->addr, send->addrlen) < 0) {
    LOG_ERROR("send: %s", strerror(errno));
    return -1;
  }

  return 0;
}

int artnet_recv(int sock, struct artnet_sendrecv *recv)
{
  int ret;

  if ((ret = recvfrom(sock, recv->packet, sizeof(*recv->packet), 0, &recv->addr, &recv->addrlen)) < 0) {
    LOG_ERROR("recv: %s", strerror(errno));
    return -1;
  } else {
    recv->len = ret;
  }

  LOG_DEBUG("op=%04x len=%u", recv->packet->header.opcode, recv->len);

  return 0;
}
