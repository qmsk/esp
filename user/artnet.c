#include "artnet.h"

#include <lwip/sockets.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <string.h>

#include "logging.h"

#define ARTNET_TASK_STACK 512
#define ARTNET_PORT 6454
#define ARTNET_BUF 512

const uint8_t artnet_id[] = { 'A', 'r', 't', '-', 'N', 'e', 't', '\0'};
const uint16_t artnet_version = 14;

enum artnet_opcode {
  ARTNET_OP_POLL        = 0x2000,
  ARTNET_OP_POLL_REPLY  = 0x2100,
  ARTNET_OP_DMX         = 0x5000,
};

struct __attribute__((packed)) artnet_packet_header {
  uint8_t id[8];
  uint16_t opcode;
  uint16_t version;
};

struct __attribute__((packed)) artnet_packet_poll {
  struct artnet_packet_header header;

  uint8_t ttm;
  uint8_t priority;
};
struct __attribute__((packed)) artnet_packet_dmx {
  struct artnet_packet_header header;

  uint8_t sequence;
  uint8_t physical;
  uint8_t sub_uni;
  uint8_t net;
  uint16_t length;

  uint8_t data[0];
};

union artnet_packet {
  struct artnet_packet_header header;
  struct artnet_packet_poll poll;
  struct artnet_packet_dmx dmx;
};

struct artnet {
  xTaskHandle task;

  int socket;

  union artnet_packet packet;
} artnet;

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

int artnet_parse_header(struct artnet *artnet, struct artnet_packet_header *header, int len)
{
  if (len < sizeof(*header)) {
    LOG_WARN("short packet");
    return -1;
  }

  header->version = ntohs(header->version);

  if (memcmp(header->id, artnet_id, sizeof(artnet_id))) {
    LOG_WARN("invalid id");
    return -1;
  }

  if (header->version < artnet_version) {
    LOG_WARN("invalid version: %u", header->version);
    return -1;
  }

  LOG_INFO("id=%.8s opcode=%04x version=%u",
    header->id,
    header->opcode,
    header->version
  );

  return 0;
}

int artnet_op_poll(struct artnet *artnet, struct artnet_packet_poll *poll, int len)
{
  if (len < sizeof(*poll)) {
    LOG_WARN("short p<cket");
    return -1;
  }

  LOG_INFO("ttm=%02x priority=%u", poll->ttm, poll->priority);

  return 0;
}

int artnet_op_dmx(struct artnet *artnet, struct artnet_packet_dmx *dmx, int len)
{
  if (len < sizeof(*dmx)) {
    LOG_WARN("short packet header");
    return -1;
  }

  dmx->length = ntohs(dmx->length);

  if (len < sizeof(*dmx) + dmx->length) {
    LOG_WARN("short packet payload");
    return -1;
  }

  LOG_INFO("seq=%u phy=%u addr=%u.%u len=%u",
    dmx->sequence,
    dmx->physical,
    dmx->sub_uni,
    dmx->net,
    dmx->length
  );

  return 0;
}

int artnet_packet(struct artnet *artnet, union artnet_packet *packet, int len)
{
  switch (packet->header.opcode) {
  case ARTNET_OP_POLL:
    return artnet_op_poll(artnet, &packet->poll, len);

  case ARTNET_OP_DMX:
  return artnet_op_dmx(artnet, &packet->dmx, len);

  default:
    LOG_WARN("unknown opcode: %04x", packet->header.opcode);
    return 0;
  }
}

int artnet_recv(struct artnet *artnet)
{
  int ret, err;

  if ((ret = recv(artnet->socket, &artnet->packet, sizeof(artnet->packet), 0)) < 0) {
    LOG_ERROR("recv");
    return -1;
  } else if ((err = artnet_parse_header(artnet, &artnet->packet.header, ret))) {
    return err;
  } else {
    return artnet_packet(artnet, &artnet->packet, ret);
  }
}

void artnet_task(void *arg)
{
  struct artnet *artnet = arg;

  LOG_DEBUG("artnet=%p", artnet);

  for (;;) {
    artnet_recv(artnet);
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
