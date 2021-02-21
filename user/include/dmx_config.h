#ifndef __DMX_CONFIG_H__
#define __DMX_CONFIG_H__

#include <lib/config.h>

#define DMX_CONFIG_GPIO 4
#define DMX_CONFIG_ARTNET_UNIVERSE 1

struct dmx_config {
  bool enabled;
  uint16_t gpio; // output enable, must be low during boot to supress GPIO2/U1TX debug output
  uint16_t artnet_universe;
};

extern struct dmx_config dmx_config;

extern const struct configtab dmx_configtab[];

#endif
