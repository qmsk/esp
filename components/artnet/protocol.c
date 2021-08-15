#include "artnet.h"

#include <logging.h>
#include <string.h>

static const uint8_t artnet_id[8] = ARTNET_ID;

int artnet_header_parse(struct artnet_packet_header *header, size_t len)
{
  if (len < sizeof(*header)) {
    LOG_WARN("short packet: len=%u", len);
    return -1;
  }

  if (memcmp(header->id, artnet_id, sizeof(artnet_id))) {
    LOG_WARN("invalid id");
    return -1;
  }

  uint16_t version = artnet_unpack_u16hl(header->version);

  if (version < ARTNET_VERSION) {
    LOG_WARN("invalid version: %u", version);
    return -1;
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

  send->len = sizeof(send->packet->poll_reply);

  memcpy(reply->id, artnet_id, sizeof(reply->id));
  reply->opcode = ARTNET_OP_POLL_REPLY;

  memcpy(reply->ip_address, artnet->options.ip_address, 4);

  reply->port_number = artnet_pack_u16lh(artnet->options.port);
  reply->net_switch = (artnet->options.address & 0x7F00) >> 8;
  reply->sub_switch = (artnet->options.address & 0x00F0) >> 0;

  strncpy((char *) reply->short_name, artnet->options.short_name, sizeof(reply->short_name) - 1);
  strncpy((char *) reply->long_name, artnet->options.long_name, sizeof(reply->long_name) - 1);

  reply->num_ports = artnet_pack_u16hl(artnet->output_count);

  for (int port = 0; port < artnet->output_count; port++) {
    struct artnet_output *output = &artnet->output_ports[port];

    if ((output->address & 0x7F00) != artnet->options.address) {
      LOG_WARN("skip output address=%04x does not match artnet address=%04x", output->address, artnet->options.address);
      continue;
    }

    reply->port_types[port] = output->type | ARTNET_PORT_TYPE_OUTPUT;
    reply->good_output[port] = ARTNET_OUTPUT_TRANSMITTING;
    reply->sw_out[port] = (output->address & 0x000F);
  }

  memcpy(reply->mac, artnet->options.mac_address, 6);

  reply->status2 = ARTNET_STATUS2_ARTNET3_SUPPORT | ARTNET_STATUS2_DHCP_SUPPORT;

  return artnet_send(artnet->socket, send);
}

int artnet_sendrecv_poll(struct artnet *artnet, struct artnet_sendrecv *sendrecv)
{
  struct artnet_packet_poll *poll = &sendrecv->packet->poll;

  if (sendrecv->len < sizeof(sendrecv->packet->poll)) {
    LOG_WARN("short packet");
    return -1;
  }

#ifdef DEBUG
  LOG_DEBUG("ttm=%02x priority=%u", poll->ttm, poll->priority); // TODO: log sender?
#else
  (void) poll;
#endif

  // keep addr for reply
  return artnet_send_poll_reply(artnet, sendrecv);
}

int artnet_recv_dmx(struct artnet *artnet, const struct artnet_sendrecv *recv)
{
  struct artnet_packet_dmx_headers *dmx_headers = &recv->packet->dmx_headers;

  if (recv->len < sizeof(recv->packet->dmx_headers)) {
    LOG_WARN("short packet header");
    return -1;
  }

  // headers
  uint16_t addr = (dmx_headers->net << 8) | (dmx_headers->sub_uni);
  uint8_t phy = dmx_headers->physical;
  uint8_t seq = dmx_headers->sequence;
  uint16_t dmx_len = artnet_unpack_u16hl(dmx_headers->length);

  if (recv->len < sizeof(*dmx_headers) + dmx_len) {
    LOG_WARN("short packet payload");
    return -1;
  }

#ifdef DEBUG
  LOG_DEBUG("seq=%u phy=%u addr=%04x len=%u",
    seq,
    phy,
    addr,
    dmx_len
  );
#else
  (void) phy;
#endif

  // payload
  struct artnet_packet_dmx_payload *dmx_payload = &recv->packet->dmx_payload;

  dmx_payload->dmx.len = dmx_len;

  // output
  struct artnet_output *output;

  if (artnet_find_output(artnet, addr, &output)) {
    LOG_DEBUG("unknown output addr=%04x", addr);
    return 0;
  }

  return artnet_output_dmx(output, &dmx_payload->dmx, seq);
}

int artnet_sendrecv(struct artnet *artnet, struct artnet_sendrecv *sendrecv)
{
  if (artnet_header_parse(&sendrecv->packet->header, sendrecv->len)) {
    return 0;
  }

  switch (artnet_unpack_u16lh(sendrecv->packet->header.opcode)) {
  case ARTNET_OP_POLL:
    return artnet_sendrecv_poll(artnet, sendrecv);

  case ARTNET_OP_DMX:
    return artnet_recv_dmx(artnet, sendrecv);

  default:
    LOG_WARN("unknown opcode: %04x", (unsigned) sendrecv->packet->header.opcode.lh);
    return 0;
  }
}
