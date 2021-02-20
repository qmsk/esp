#ifndef __P9813_CONFIG_H__
#define __P9813_CONFIG_H__

#include <lib/config.h>

struct p9813_config {
  bool enabled;
  uint16_t count;
  uint16_t artnet_universe;
  uint16_t gpio; // output power, active high
};

extern struct p9813_config p9813_config;

extern const struct configtab p9813_configtab[];

#endif
