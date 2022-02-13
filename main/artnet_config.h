#pragma once

struct artnet_config {
  bool enabled;

  uint16_t net, subnet;
};

extern struct artnet_config artnet_config;
