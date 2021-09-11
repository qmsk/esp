#include "wifi.h"
#include "wifi_internal.h"

#include <logging.h>

#include <string.h>

struct wifi_config wifi_config = {};

#define WIFI_HOSTNAME_FMT "qmsk-esp-%02x%02x%02x"

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
  { CONFIG_TYPE_STRING, "hostname",
    .string_type = { .value = wifi_config.hostname, .size = sizeof(wifi_config.hostname) },
  },
  {}
};

int init_wifi_hostname(const struct wifi_config *config)
{
  char hostname[TCPIP_HOSTNAME_MAX_SIZE];
  uint8_t mac[6];
  esp_err_t err;

  if (config->hostname[0]) {
    LOG_INFO("Using config hostname: %s", config->hostname);

    strlcpy(hostname, config->hostname, sizeof(hostname));

  } else if ((err = esp_wifi_get_mac(WIFI_MODE_STA, mac))) {
    LOG_ERROR("esp_wifi_get_mac WIFI_MODE_STA: %s", esp_err_to_name(err));
    return -1;

  } else {
    LOG_INFO("Using default hostname: " WIFI_HOSTNAME_FMT, mac[3], mac[4], mac[5]);

    snprintf(hostname, sizeof(hostname), WIFI_HOSTNAME_FMT, mac[3], mac[4], mac[5]);
  }

  if ((err = tcpip_adapter_set_hostname(TCPIP_ADAPTER_IF_STA, hostname))) {
    LOG_ERROR("tcpip_adapter_set_hostname: %s", esp_err_to_name(err));
    return -1;
  }

  return 0;
}

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
