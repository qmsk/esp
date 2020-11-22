#include "dmx_config.h"

struct dmx_config dmx_config = {
  .gpio = DMX_CONFIG_GPIO,
  .artnet_universe = DMX_CONFIG_ARTNET_UNIVERSE,
};

const struct configtab dmx_configtab[] = {
  { CONFIG_TYPE_UINT16, "gpio",
    .value  = { .uint16 = &dmx_config.gpio },
  },
  { CONFIG_TYPE_UINT16, "artnet_universe",
    .value  = { .uint16 = &dmx_config.artnet_universe },
  },
  {}
};
