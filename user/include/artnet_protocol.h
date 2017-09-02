#ifndef __ARTNET_PROTOCOL_H__
#define __ARTNET_PROTOCOL_H__

#define ARTNET_PORT 6454
#define ARTNET_ID { 'A', 'r', 't', '-', 'N', 'e', 't', '\0'}
#define ARTNET_VERSION 14

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

struct __attribute__((packed)) artnet_packet_poll_reply {
  uint8_t id[8];
  uint16_t opcode;

  uint32_t ip_address;
  uint16_t port;
  uint16_t version_info;
  uint8_t net_switch;
  uint8_t sub_switch;
  uint16_t oem;
  uint8_t ubea_version;
  uint8_t status1;
  uint16_t esta_man;
  uint8_t short_name[18];
  uint8_t long_name[64];
  uint8_t node_report[64];
  uint16_t num_ports;
  uint8_t port_types[4];
  uint8_t good_input[4];
  uint8_t good_output[4];
  uint8_t sw_in[4];
  uint8_t sw_out[4];
  uint8_t sw_video;
  uint8_t sw_macro;
  uint8_t sw_remote;
  uint8_t spare1;
  uint8_t spare2;
  uint8_t spare3;
  uint8_t style;
  uint8_t mac[8];
  uint32_t bind_ip;
  uint8_t bind_index;
  uint8_t status2;
  uint8_t filler[26];
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
  struct artnet_packet_poll_reply poll_reply;
  struct artnet_packet_dmx dmx;
};

#endif
