#ifndef __ARTNET_CONFIG_H__
#define __ARTNET_CONFIG_H__

#include <lib/config.h>

#define ARTNET_CONFIG_UNIVERSE 0

struct artnet_config {
  uint16_t universe;
};

extern struct artnet_config artnet_config;

extern const struct configtab artnet_configtab[];

#endif
