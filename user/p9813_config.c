#include "p9813_config.h"

struct p9813_config p9813_config = {

};

const struct configtab p9813_configtab[] = {
  { CONFIG_TYPE_UINT16, "count",
    .value  = { .uint16 = &p9813_config.count },
  },
  { CONFIG_TYPE_UINT16, "artnet_universe",
    .value  = { .uint16 = &p9813_config.artnet_universe },
  },
  { CONFIG_TYPE_UINT16, "gpio",
    .value  = { .uint16 = &p9813_config.gpio },
  },
  {}
};
