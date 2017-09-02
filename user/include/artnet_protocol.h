#ifndef __ARTNET_PROTOCOL_H__
#define __ARTNET_PROTOCOL_H__

#include "artnet.h"
#include "artnet_dmx.h"
#include <lwip/def.h>

#define ARTNET_PORT 6454
#define ARTNET_ID { 'A', 'r', 't', '-', 'N', 'e', 't', '\0'}
#define ARTNET_VERSION 14

enum artnet_opcode {
  ARTNET_OP_POLL        = 0x2000,
  ARTNET_OP_POLL_REPLY  = 0x2100,
  ARTNET_OP_DMX         = 0x5000,
};

enum artnet_port_type {
  ARTNET_PORT_TYPE_DMX      = 0x00,

  ARTNET_PORT_TYPE_OUTPUT   = 1 << 7,
  ARTNET_PORT_TYPE_INPUT    = 1 << 6,
};

enum artnet_output_status {
  ARTNET_OUTPUT_TRANSMITTING = 1 << 7,
};

enum artnet_status2 {
  ARTNET_STATUS2_ARTNET3_SUPPORT  = 1 << 3,
  ARTNET_STATUS2_DHCP_SUPPORT     = 1 << 2,
  ARTNET_STATUS2_DHCP_ENABLE      = 1 << 1,
  ARTNET_STATUS2_WEB_SUPPORT      = 1 << 0,
};

typedef struct __attribute__((packed)) { uint16_t lh; } artnet_u16lh;
typedef struct __attribute__((packed)) { uint16_t hl; } artnet_u16hl;

struct __attribute__((packed)) artnet_packet_header {
  uint8_t id[8];
  artnet_u16lh opcode;
  artnet_u16hl version;
};

struct __attribute__((packed)) artnet_packet_poll {
  struct artnet_packet_header header;

  uint8_t ttm;
  uint8_t priority;
};

struct __attribute__((packed)) artnet_packet_poll_reply {
  uint8_t id[8];
  uint16_t opcode;

  uint8_t ip_address[4];
  artnet_u16lh port_number;
  artnet_u16hl version_info;
  uint8_t net_switch;
  uint8_t sub_switch;
  artnet_u16hl oem;
  uint8_t ubea_version;
  uint8_t status1;
  artnet_u16lh esta_man;
  uint8_t short_name[18];
  uint8_t long_name[64];
  uint8_t node_report[64];
  artnet_u16hl num_ports;
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

union artnet_packet_dmx {
  struct __attribute__((packed)) artnet_packet_dmx_headers {
    struct artnet_packet_header header;

    uint8_t sequence;
    uint8_t physical;
    uint8_t sub_uni;
    uint8_t net;

    artnet_u16hl length;
  } headers;

  // align payload.dmx.data with the end of headers
  struct __attribute__((packed)) {
    struct artnet_packet_header header;

    uint8_t alignment[4];

    // dmx.len overlaps with headers.length
    struct artnet_dmx dmx;
  } payload;
};

union artnet_packet {
  struct artnet_packet_header header;
  struct artnet_packet_poll poll;
  struct artnet_packet_poll_reply poll_reply;
  union artnet_packet_dmx dmx;
};

inline uint16_t artnet_unpack_u16hl(artnet_u16hl f) { return ntohs(f.hl); }
inline uint16_t artnet_unpack_u16lh(artnet_u16lh f) { return f.lh; }

inline artnet_u16hl artnet_pack_u16hl(uint16_t u16) { return (artnet_u16hl){ htons(u16) }; }
inline artnet_u16lh artnet_pack_u16lh(uint16_t u16) { return (artnet_u16lh){ u16 }; }

#endif
