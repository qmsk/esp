#include "wifi_config.h"

struct wifi_config wifi_config = {

};

const struct configtab wifi_configtab[] = {
  { CONFIG_TYPE_STRING, "ssid",
    .size   = sizeof(wifi_config.ssid),
    .value  = { .string = wifi_config.ssid },
  },
  { CONFIG_TYPE_STRING, "password",
    .size   = sizeof(wifi_config.password),
    .secret = true,
    .value  = { .string = wifi_config.password },
  },
  {}
};
