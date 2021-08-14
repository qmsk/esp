#include "wifi.h"
#include "wifi_internal.h"

#include <logging.h>

#include <string.h>

struct wifi_config wifi_config = {};

const struct configtab wifi_configtab[] = {
  { CONFIG_TYPE_BOOL, "enabled",
    .bool_type = { .value = &wifi_config.enabled },
  },
  { CONFIG_TYPE_STRING, "ssid",
    .string_type = { .value = wifi_config.ssid, .size = sizeof(wifi_config.ssid) },
  },
  { CONFIG_TYPE_STRING, "password",
    .string_type = { .value = wifi_config.password, .size   = sizeof(wifi_config.password) },
    .secret = true,
  },
  {}
};

int init_wifi_config(const struct wifi_config *config)
{
  wifi_config_t wifi_config = {};

  if (config->enabled) {
    LOG_INFO("enabled");

    strncpy((char *) wifi_config.sta.ssid, config->ssid, sizeof(wifi_config.sta.ssid));
    strncpy((char *) wifi_config.sta.password, config->password, sizeof(wifi_config.sta.password));

    if (config->password[0]) {
      wifi_config.sta.threshold.authmode = WIFI_AUTHMODE_THRESHOLD;
    }

    if (wifi_connect(&wifi_config)) {
      LOG_ERROR("wifi_connect");
      return -1;
    }
  } else {
    LOG_INFO("disabled");
  }

  return 0;
}
