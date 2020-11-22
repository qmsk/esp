#include "config.h"
#include "user_config.h"
#include "wifi_config.h"

struct user_config user_config = {
  .version        = USER_CONFIG_VERSION,
  .artnet = {
    .universe = ARTNET_CONFIG_UNIVERSE,
  },
  .dmx = {
    .gpio = DMX_CONFIG_GPIO,
    .artnet_universe = DMX_CONFIG_ARTNET_UNIVERSE,
  },
};

const struct configtab wifi_configtab[] = {
  { CONFIG_TYPE_STRING, "ssid",
    .size   = sizeof(user_config.wifi.ssid),
    .value  = { .string = user_config.wifi.ssid },
  },
  { CONFIG_TYPE_STRING, "password",
    .size   = sizeof(user_config.wifi.password),
    .secret = true,
    .value  = { .string = user_config.wifi.password },
  },
  {}
};

const struct configtab artnet_configtab[] = {
  { CONFIG_TYPE_UINT16, "universe",
    .size   = sizeof(user_config.artnet.universe),
    .value  = { .uint16 = &user_config.artnet.universe },
  },
  {}
};

const struct configtab dmx_configtab[] = {
  { CONFIG_TYPE_UINT16, "gpio",
    .value  = { .uint16 = &user_config.dmx.gpio },
  },
  { CONFIG_TYPE_UINT16, "artnet_universe",
    .value  = { .uint16 = &user_config.dmx.artnet_universe },
  },
  {}
};

const struct configtab p9813_configtab[] = {
  { CONFIG_TYPE_UINT16, "count",
    .value  = { .uint16 = &user_config.p9813.count },
  },
  { CONFIG_TYPE_UINT16, "artnet_universe",
    .value  = { .uint16 = &user_config.p9813.artnet_universe },
  },
  { CONFIG_TYPE_UINT16, "gpio",
    .value  = { .uint16 = &user_config.p9813.gpio },
  },
  {}
};

const struct configmod user_configmods[] = {
  { "wifi",   wifi_configtab },
  { "artnet", artnet_configtab },
  { "dmx",    dmx_configtab },
  { "p9813",  p9813_configtab },
  {}
};

const struct config user_configmeta = {
  .modules = user_configmods,
};
