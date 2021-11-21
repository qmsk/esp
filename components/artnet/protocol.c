#include "artnet.h"

#include <logging.h>
#include <string.h>

static const uint8_t artnet_id[8] = ARTNET_ID;

int artnet_header_parse(struct artnet_packet_header *header, size_t len)
{
  if (len < sizeof(*header)) {
    LOG_WARN("short packet: len=%u", len);
    return 1;
  }

  if (memcmp(header->id, artnet_id, sizeof(artnet_id))) {
    LOG_WARN("invalid id");
    return 1;
  }

  uint16_t version = artnet_unpack_u16hl(header->version);

  if (version < ARTNET_VERSION) {
    LOG_WARN("invalid version: %u", version);
    return 1;
  }

  LOG_DEBUG("id=%.8s opcode=%04x version=%u",
    header->id,
    artnet_unpack_u16lh(header->opcode),
    version
  );

  return 0;
}

int artnet_send_poll_reply(struct artnet *artnet, struct artnet_sendrecv *send)
{
  struct artnet_packet_poll_reply *reply = &send->packet->poll_reply;
  int err;

  // prepare header fields
  send->len = sizeof(send->packet->poll_reply);

  memcpy(reply->id, artnet_id, sizeof(reply->id));
  reply->opcode = ARTNET_OP_POLL_REPLY;

  // prepare constant fields
  memcpy(reply->ip_address, artnet->options.ip_address, 4);

  reply->port_number = artnet_pack_u16lh(artnet->options.port);
  reply->net_switch = (artnet->options.address & 0x7F00) >> 8;
  reply->sub_switch = (artnet->options.address & 0x00F0) >> 0;

  strncpy((char *) reply->short_name, artnet->options.short_name, sizeof(reply->short_name) - 1);
  strncpy((char *) reply->long_name, artnet->options.long_name, sizeof(reply->long_name) - 1);

  memcpy(reply->mac, artnet->options.mac_address, 6);

  reply->status2 = ARTNET_STATUS2_ARTNET3_SUPPORT | ARTNET_STATUS2_DHCP_SUPPORT;

  // per-bind fields
  for (unsigned index = 0; ; index++) {
    unsigned num_ports = 0;

    memset(reply->port_types, 0, sizeof(reply->port_types));
    memset(reply->good_output, 0, sizeof(reply->good_output));
    memset(reply->sw_out, 0, sizeof(reply->sw_out));

    for (struct artnet_output *output = artnet->output_ports; output < artnet->output_ports + artnet->output_count; output++) {
      if (output->options.index != index) {
        continue;
      }

      if ((output->options.address & 0x7F00) != artnet->options.address) {
        LOG_WARN("skip output address=%04x does not match artnet address=%04x", output->options.address, artnet->options.address);
        continue;
      }

      if (output->options.port >= 4) {
        LOG_WARN("skip output port=%d index=%u", output->options.port, output->options.index);
        continue;
      }

      reply->port_types[output->options.port] = output->type | ARTNET_PORT_TYPE_OUTPUT;
      reply->good_output[output->options.port] = ARTNET_OUTPUT_TRANSMITTING;
      reply->sw_out[output->options.port] = (output->options.address & 0x000F);
      num_ports++;
    }

    reply->num_ports = artnet_pack_u16hl(num_ports);
    reply->bind_index = index + 1;

    if (index > 0 && num_ports == 0) {
      // no more ports
      break;
    }

    if ((err = artnet_send(artnet->socket, send))) {
      LOG_ERROR("artnet_send");
      return err;
    }
  }

  return 0;
}

int artnet_sendrecv_poll(struct artnet *artnet, struct artnet_sendrecv *sendrecv)
{
  struct artnet_packet_poll *poll = &sendrecv->packet->poll;

  if (sendrecv->len < sizeof(sendrecv->packet->poll)) {
    LOG_WARN("short packet");
    return 1;
  }

#ifdef DEBUG
  LOG_DEBUG("ttm=%02x priority=%u", poll->ttm, poll->priority); // TODO: log sender?
#else
  (void) poll;
#endif

  // keep addr for reply
  return artnet_send_poll_reply(artnet, sendrecv);
}

// node in synchronous DMX mode?
static bool artnet_sync_state (struct artnet *artnet)
{
  if (artnet->sync_tick) {
    return xTaskGetTickCount() - artnet->sync_tick < ARTNET_SYNC_TICKS;
  } else {
    return false;
  }
}

int artnet_recv_dmx(struct artnet *artnet, const struct artnet_sendrecv *recv)
{
  struct artnet_packet_dmx *dmx = &recv->packet->dmx;

  if (recv->len < sizeof(recv->packet->dmx)) {
    LOG_WARN("short packet header");
    return 1;
  }

  // headers
  uint16_t addr = (dmx->net << 8) | (dmx->sub_uni);
  uint8_t phy = dmx->physical;
  uint8_t seq = dmx->sequence;
  uint16_t len = artnet_unpack_u16hl(dmx->length);

  if (recv->len < sizeof(*dmx) + len) {
    LOG_WARN("short packet payload");
    return 1;
  }

  if (len > ARTNET_DMX_SIZE) {
    LOG_WARN("long data length");
    return 1;
  }

#ifdef DEBUG
  LOG_DEBUG("seq=%u phy=%u addr=%04x len=%u",
    seq,
    phy,
    addr,
    len
  );
#else
  (void) phy;
#endif

  // copy
  artnet->dmx.sync_mode = artnet_sync_state(artnet);
  artnet->dmx.seq = seq;
  artnet->dmx.len = len;

  memcpy(artnet->dmx.data, dmx->data, len);

  return artnet_outputs_dmx(artnet, addr, &artnet->dmx);
}

int artnet_recv_sync(struct artnet *artnet, const struct artnet_sendrecv *sendrecv)
{
  struct artnet_packet_sync *sync = &sendrecv->packet->sync;

  if (sendrecv->len < sizeof(sendrecv->packet->sync)) {
    LOG_WARN("short packet");
    return 1;
  }

  (void) sync;

  artnet->sync_tick = xTaskGetTickCount();

  return artnet_outputs_sync(artnet);
}

int artnet_sendrecv(struct artnet *artnet, struct artnet_sendrecv *sendrecv)
{
  int err;

  if ((err = artnet_header_parse(&sendrecv->packet->header, sendrecv->len))) {
    return err;
  }

  switch (artnet_unpack_u16lh(sendrecv->packet->header.opcode)) {
  case ARTNET_OP_POLL:
    stats_counter_increment(&artnet->stats.recv_poll);

    return artnet_sendrecv_poll(artnet, sendrecv);

  case ARTNET_OP_DMX:
    stats_counter_increment(&artnet->stats.recv_dmx);

    return artnet_recv_dmx(artnet, sendrecv);

  case ARTNET_OP_SYNC:
    stats_counter_increment(&artnet->stats.recv_sync);

    return artnet_recv_sync(artnet, sendrecv);

  default:
    LOG_WARN("unknown opcode: %04x", (unsigned) sendrecv->packet->header.opcode.lh);

    stats_counter_increment(&artnet->stats.recv_unknown);

    return 0;
  }
}
