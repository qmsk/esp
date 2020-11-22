#include "config.h"
#include "user_config.h"

struct user_config user_config = {
  .version        = USER_CONFIG_VERSION,
  .wifi_ssid      = USER_CONFIG_WIFI_SSID,
  .wifi_password  = USER_CONFIG_WIFI_PASSWORD,
  .artnet = {
    .universe = ARTNET_CONFIG_UNIVERSE,
  },
  .dmx = {
    .gpio = DMX_CONFIG_GPIO,
    .artnet_universe = DMX_CONFIG_ARTNET_UNIVERSE,
  },
};

const struct config_tab user_configtab[] = {
  { CONFIG_TYPE_UINT16, "version",
    .readonly = true,
    .value  = { .uint16 = &user_config.version },
  },
  { CONFIG_TYPE_STRING, "wifi.ssid",
    .size   = sizeof(user_config.wifi_ssid),
    .value  = { .string = user_config.wifi_ssid },
  },
  { CONFIG_TYPE_STRING, "wifi.password",
    .size   = sizeof(user_config.wifi_password),
    .secret = true,
    .value  = { .string = user_config.wifi_password },
  },
  { CONFIG_TYPE_UINT16, "artnet.universe",
    .size   = sizeof(user_config.artnet.universe),
    .value  = { .uint16 = &user_config.artnet.universe },
  },
  { CONFIG_TYPE_UINT16, "dmx.gpio",
    .value  = { .uint16 = &user_config.dmx.gpio },
  },
  { CONFIG_TYPE_UINT16, "dmx.artnet_universe",
    .value  = { .uint16 = &user_config.dmx.artnet_universe },
  },
  { CONFIG_TYPE_UINT16, "p9813.count",
    .value  = { .uint16 = &user_config.p9813.count },
  },
  { CONFIG_TYPE_UINT16, "p9813.artnet_universe",
    .value  = { .uint16 = &user_config.p9813.artnet_universe },
  },
  { CONFIG_TYPE_UINT16, "p9813.gpio",
    .value  = { .uint16 = &user_config.p9813.gpio },
  },
  {}
};
