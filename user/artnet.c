#include "artnet.h"
#include "artnet_protocol.h"

#include <lwip/sockets.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <string.h>

#include "logging.h"

#define ARTNET_TASK_STACK 512
#define ARTNET_BUF 512

static const char *artnet_product = "https://github.com/SpComb/esp-projects";

struct artnet {
  xTaskHandle task;

  int socket;

  union artnet_packet packet;
} artnet;

struct artnet_sendrecv {
  struct sockaddr addr;
  socklen_t addrlen;

  union artnet_packet *packet;
  size_t len;
};

struct sockname {
  sa_family_t family;
  const char *host;
  uint port;

  char buf[32];
};

int artnet_init(struct artnet *artnet)
{
  struct sockaddr_in bind_addr = {
    .sin_family = AF_INET,
    .sin_port = htons(ARTNET_PORT),
    .sin_addr = { INADDR_ANY },
  };

  if ((artnet->socket = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
    LOG_ERROR("socket");
    return -1;
  }

  if (bind(artnet->socket, (struct sockaddr *) &bind_addr, sizeof(bind_addr)) < 0) {
    LOG_ERROR("bind");
    return -1;
  }

  LOG_INFO("socket=%d", artnet->socket);

  return 0;
}

int artnet_sockname(struct artnet *artnet, struct sockname *sockname)
{
  struct sockaddr sockaddr;
  socklen_t socklen = sizeof(sockaddr);

  if (getsockname(artnet->socket, &sockaddr, &socklen) < 0) {
    LOG_ERROR("getsockname");
    return -1;
  }

  switch (sockaddr.sa_family) {
    case AF_INET:
      sockname->host = inet_ntoa_r(((struct sockaddr_in *) &sockaddr)->sin_addr, sockname->buf, sizeof(sockname->buf));
      sockname->port = ntohs(((struct sockaddr_in *) &sockaddr)->sin_port);
      break;

    case AF_INET6:
      sockname->host = inet6_ntoa_r(((struct sockaddr_in6 *) &sockaddr)->sin6_addr, sockname->buf, sizeof(sockname->buf));
      sockname->port = ntohs(((struct sockaddr_in6 *) &sockaddr)->sin6_port);
      break;

    default:
      sockname->host = NULL;
      sockname->port = 0;
      LOG_ERROR("sockaddr.sa_family: %d", sockaddr.sa_family);
      break;
  }

  return 0;
}

int artnet_send(struct artnet *artnet, const struct artnet_sendrecv send)
{
  if (sendto(artnet->socket, send.packet, send.len, 0, &send.addr, send.addrlen) < 0) {
    LOG_ERROR("send");
    return -1;
  }

  LOG_INFO("op=%04x len=%u", send.packet->header.opcode, send.len);

  return 0;
}

int artnet_parse_header(struct artnet *artnet, struct artnet_packet_header *header, size_t len)
{
  uint8_t artnet_id[8] = ARTNET_ID;

  if (len < sizeof(*header)) {
    LOG_WARN("short packet");
    return -1;
  }

  uint16_t version = artnet_unpack_u16hl(header->version);

  if (memcmp(header->id, artnet_id, sizeof(artnet_id))) {
    LOG_WARN("invalid id");
    return -1;
  }

  if (version < ARTNET_VERSION) {
    LOG_WARN("invalid version: %u", version);
    return -1;
  }

  LOG_INFO("id=%.8s opcode=%04x version=%u",
    header->id,
    header->opcode,
    version
  );

  return 0;
}

int artnet_poll_reply(struct artnet *artnet, struct artnet_packet_poll_reply *reply)
{
  reply->port_number = artnet_pack_u16lh(ARTNET_PORT);
  reply->status1 = ARTNET_STATUS2_ARTNET3_SUPPORT | ARTNET_STATUS2_DHCP_SUPPORT;

  snprintf((char *) reply->short_name, sizeof(reply->short_name), "%s", "unknown");
  snprintf((char *) reply->long_name, sizeof(reply->long_name), "%s: %s", artnet_product, "unknown");

  return 0;
}

int artnet_op_poll(struct artnet *artnet, const struct artnet_sendrecv recv)
{
  int err;

  if (recv.len < sizeof(recv.packet->poll)) {
    LOG_WARN("short p<cket");
    return -1;
  }

  struct artnet_packet_poll *poll = &recv.packet->poll;

  LOG_INFO("ttm=%02x priority=%u", poll->ttm, poll->priority);

  struct artnet_sendrecv send = recv; // reply to sender

  send.len = sizeof(send.packet->poll_reply);
  send.packet->poll_reply = (struct artnet_packet_poll_reply) {
    .id     = ARTNET_ID,
    .opcode = ARTNET_OP_POLL_REPLY,
  };

  if ((err = artnet_poll_reply(artnet, &send.packet->poll_reply))) {
    return err;
  }

  return artnet_send(artnet, send);
}

int artnet_op_dmx(struct artnet *artnet, struct artnet_sendrecv recv)
{
  if (recv.len < sizeof(recv.packet->dmx)) {
    LOG_WARN("short packet header");
    return -1;
  }

  struct artnet_packet_dmx *dmx = &recv.packet->dmx;
  uint16_t dmx_len = artnet_unpack_u16hl(dmx->length);

  if (recv.len < sizeof(*dmx) + dmx_len) {
    LOG_WARN("short packet payload");
    return -1;
  }

  LOG_INFO("seq=%u phy=%u addr=%u.%u len=%u",
    dmx->sequence,
    dmx->physical,
    dmx->sub_uni,
    dmx->net,
    dmx_len
  );

  return 0;
}

int artnet_op(struct artnet *artnet, struct artnet_sendrecv recv)
{
  switch (artnet_unpack_u16lh(recv.packet->header.opcode)) {
  case ARTNET_OP_POLL:
    return artnet_op_poll(artnet, recv);

  case ARTNET_OP_DMX:
    return artnet_op_dmx(artnet, recv);

  default:
    LOG_WARN("unknown opcode: %04x", recv.packet->header.opcode);
    return 0;
  }
}

int artnet_recv(struct artnet *artnet, struct artnet_sendrecv *recv)
{
  int ret;

  if ((ret = recvfrom(artnet->socket, recv->packet, sizeof(*recv->packet), 0, &recv->addr, &recv->addrlen)) < 0) {
    LOG_ERROR("recv");
    return -1;
  } else {
    recv->len = ret;
  }

  return artnet_parse_header(artnet, &recv->packet->header, recv->len);
}

void artnet_task(void *arg)
{
  struct artnet *artnet = arg;
  int err;

  LOG_DEBUG("artnet=%p", artnet);

  for (;;) {
    struct artnet_sendrecv recv = {
      .addrlen = sizeof(recv.addr),
      .packet = &artnet->packet,
    };

    if ((err = artnet_recv(artnet, &recv))) {

    } else if ((err = artnet_op(artnet, recv))) {

    }
  }
}

int init_artnet(struct user_config *config)
{
  struct sockname sockname;
  int err;

  if ((err = artnet_init(&artnet))) {
    return err;
  }

  if ((err = artnet_sockname(&artnet, &sockname))) {
    return -1;
  } else {
    LOG_INFO("listen UDP %s:%u", sockname.host, sockname.port);
  }

  if ((err = xTaskCreate(&artnet_task, (signed char *) "artnet", ARTNET_TASK_STACK, &artnet, tskIDLE_PRIORITY + 2, &artnet.task)) <= 0) {
    LOG_ERROR("xTaskCreate");
    return -1;
  }

  return 0;
}
