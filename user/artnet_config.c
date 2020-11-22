#include "artnet_config.h"

struct artnet_config artnet_config = {
  .universe = ARTNET_CONFIG_UNIVERSE,
};

const struct configtab artnet_configtab[] = {
  { CONFIG_TYPE_UINT16, "universe",
    .size   = sizeof(artnet_config.universe),
    .value  = { .uint16 = &artnet_config.universe },
  },
  {}
};
