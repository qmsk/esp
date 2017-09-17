#include "artnet.h"
#include "artnet_config.h"
#include "artnet_protocol.h"
#include "artnet_dmx.h"

#include <lwip/sockets.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <string.h>

#include "logging.h"

#define ARTNET_TASK_STACK 512
#define ARTNET_PORTS 4

static const char *artnet_product = "https://github.com/SpComb/esp-projects";

struct artnet_output {
  uint16_t addr;
  enum artnet_port_type type;

  xQueueHandle queue;

  uint16_t seq;
};

struct artnet {
  struct artnet_config config;
  const struct user_info *user_info;

  xTaskHandle task;

  uint output_count;
  struct artnet_output output_ports[ARTNET_PORTS];

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

int artnet_init(struct artnet *artnet, const struct user_info *user_info)
{
  artnet->user_info = user_info;
  artnet->socket = -1;

  return 0;
}

int artnet_listen(struct artnet *artnet)
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

int artnet_setup(struct artnet *artnet, const struct artnet_config *config)
{
  struct sockname sockname;
  int err;

  artnet->config = *config;

  if (artnet->socket < 0) {
    if ((err = artnet_listen(artnet))) {
      return err;
    }
  }

  if ((err = artnet_sockname(artnet, &sockname))) {
    return -1;
  } else {
    LOG_INFO("listen UDP %s:%u at universe=%u", sockname.host, sockname.port, config->universe);
  }

  return 0;
}

int artnet_start_output(struct artnet *artnet, uint16_t addr, xQueueHandle queue)
{
  if (artnet->output_count >= ARTNET_PORTS) {
    LOG_ERROR("too many outputs");
    return -1;
  }

  if ((addr & 0xFFF0) != artnet->config.universe) {
    LOG_ERROR("port address=%u mismatch with artnet.universe=%u", addr, artnet->config.universe);
    return -1;
  }

  LOG_INFO("port=%u addr=%u", artnet->output_count, addr);

  artnet->output_ports[artnet->output_count] = (struct artnet_output){
    .addr  = addr,
    .type  = ARTNET_PORT_TYPE_DMX,
    .queue = queue,
  };
  artnet->output_count++; // XXX: atomic for concurrent artnet_find_output()?


  return 0;
}

int artnet_find_output(struct artnet *artnet, uint16_t addr, struct artnet_output **outputp)
{
  for (int port = 0; port < artnet->output_count; port++) {
    struct artnet_output *output = &artnet->output_ports[port];

    if (output->addr == addr) {
      *outputp = output;
      return 0;
    }
  }

  return -1;
}

int artnet_output_dmx(struct artnet_output *output, struct artnet_dmx *dmx, uint16_t seq)
{
  if (seq == 0) {
    // reset
    output->seq = 0;

  } else if (seq <= output->seq && output->seq - seq < 128) {
    LOG_WARN("skip addr=%d seq=%d < %d", output->addr, seq, output->seq);
    return 0;

  } else {
    // advance or wraparound
    output->seq = seq;
  }

  if (!xQueueOverwrite(output->queue, dmx)) {
    LOG_WARN("addr=%u seq=%d xQueueOverwrite", output->addr, seq);
  }

  return 0;
}

int artnet_send(struct artnet *artnet, const struct artnet_sendrecv send)
{
  if (sendto(artnet->socket, send.packet, send.len, 0, &send.addr, send.addrlen) < 0) {
    LOG_ERROR("send");
    return -1;
  }

  LOG_DEBUG("op=%04x len=%u", send.packet->header.opcode, send.len);

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

  LOG_DEBUG("id=%.8s opcode=%04x version=%u",
    header->id,
    header->opcode,
    version
  );

  return 0;
}

int artnet_poll_reply(struct artnet *artnet, struct artnet_packet_poll_reply *reply)
{
  memcpy(&reply->ip_address, &artnet->user_info->ip, 4);
  memcpy(reply->mac, artnet->user_info->mac, 6);

  reply->port_number = artnet_pack_u16lh(ARTNET_PORT);
  reply->net_switch = (artnet->config.universe & 0x7F00) >> 8;
  reply->sub_switch = (artnet->config.universe & 0x00F0) >> 0;
  reply->status2 = ARTNET_STATUS2_ARTNET3_SUPPORT | ARTNET_STATUS2_DHCP_SUPPORT;

  snprintf((char *) reply->short_name, sizeof(reply->short_name), "%s", artnet->user_info->hostname);
  snprintf((char *) reply->long_name, sizeof(reply->long_name), "%s: %s", artnet_product, artnet->user_info->hostname);

  reply->num_ports = artnet_pack_u16hl(artnet->output_count);

  for (int port = 0; port < artnet->output_count; port++) {
    struct artnet_output *output = &artnet->output_ports[port];

    reply->port_types[port] = output->type | ARTNET_PORT_TYPE_OUTPUT;
    reply->good_output[port] = ARTNET_OUTPUT_TRANSMITTING;
    reply->sw_out[port] = output->addr & 0x0F;
  }

  return 0;
}

int artnet_op_poll(struct artnet *artnet, const struct artnet_sendrecv recv)
{
  int err;

  if (recv.len < sizeof(recv.packet->poll)) {
    LOG_WARN("short packet");
    return -1;
  }

  struct artnet_packet_poll *poll = &recv.packet->poll; (void) poll; // XXX: unused

  LOG_DEBUG("ttm=%02x priority=%u", poll->ttm, poll->priority); // TODO: log sender?

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
  if (recv.len < sizeof(recv.packet->dmx.headers)) {
    LOG_WARN("short packet header");
    return -1;
  }

  // struct view of packet headers
  struct artnet_packet_dmx_headers *headers = &recv.packet->dmx.headers;
  uint16_t addr = (headers->net << 8) | (headers->sub_uni);
  uint16_t seq = headers->sequence;
  uint16_t dmx_len = artnet_unpack_u16hl(headers->length);

  if ((addr & 0xFFF0) != artnet->config.universe) {
    LOG_DEBUG("ignore addr=%u", addr);
    return 0;
  }

  if (recv.len < sizeof(*headers) + dmx_len) {
    LOG_WARN("short packet payload");
    return -1;
  }

  LOG_DEBUG("seq=%u phy=%u addr=%u len=%u",
    seq,
    headers->physical,
    addr,
    dmx_len
  );

  struct artnet_output *output;

  if (artnet_find_output(artnet, addr, &output)) {
    LOG_WARN("invalid output port addr");
    return -1;
  }

  // struct view of packet payload
  struct artnet_dmx *dmx = &recv.packet->dmx.payload.dmx;

  dmx->len = dmx_len;

  LOG_DEBUG("packet=%p headers=%p dmx=%p dmx.data=%p",
    recv.packet,
    headers,
    dmx,
    dmx->data
  );

  return artnet_output_dmx(output, dmx, seq);
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

int init_artnet(const struct user_config *config, const struct user_info *user_info)
{
  int err;

  if ((err = artnet_init(&artnet, user_info))) {
    return err;
  }

  if ((err = artnet_setup(&artnet, &config->artnet))) {
    return err;
  }

  if ((err = xTaskCreate(&artnet_task, (signed char *) "artnet", ARTNET_TASK_STACK, &artnet, tskIDLE_PRIORITY + 2, &artnet.task)) <= 0) {
    LOG_ERROR("xTaskCreate");
    return -1;
  } else {
    LOG_INFO("artnet task=%p", artnet.task);
  }

  return 0;
}

int start_artnet_output(uint16_t addr, xQueueHandle queue)
{
  return artnet_start_output(&artnet, addr, queue);
}
