#include "wifi.h"
#include "wifi_config.h"
#include "user_event.h"

#include <logging.h>
#include <system_interfaces.h>
#include <system_wifi.h>

#include <string.h>

#define WIFI_CONFIG_CHANNEL_MAX 13

struct wifi_config wifi_config = {
  .mode       = WIFI_MODE_AP,
  .auth_mode  = WIFI_AUTH_WPA2_PSK,
};

#define WIFI_SSID_FMT "qmsk-esp-%02x%02x%02x"
#define WIFI_HOSTNAME_FMT "qmsk-esp-%02x%02x%02x"

const struct config_enum wifi_mode_enum[] = {
  { "OFF",    WIFI_MODE_NULL  },
  { "STA",    WIFI_MODE_STA   },
  { "AP",     WIFI_MODE_AP    },
  { "APSTA",  WIFI_MODE_APSTA },
  {}
};

const struct config_enum wifi_auth_mode_enum[] = {
  { "OPEN",           WIFI_AUTH_OPEN },
  { "WEP",            WIFI_AUTH_WEP },
  { "WPA-PSK",        WIFI_AUTH_WPA_PSK },
  { "WPA2-PSK",       WIFI_AUTH_WPA2_PSK },
  { "WPA-WPA2-PSK",   WIFI_AUTH_WPA_WPA2_PSK },
  { "WPA3-PSK",       WIFI_AUTH_WPA3_PSK },
  { "WPA2-WPA3-PSK",  WIFI_AUTH_WPA2_WPA3_PSK },
  {}
};

const struct configtab wifi_configtab[] = {
  { CONFIG_TYPE_ENUM, "mode",
    .enum_type = { .value = &wifi_config.mode, .values = wifi_mode_enum },
  },
  { CONFIG_TYPE_ENUM, "auth_mode",
    .description = (
      "For STA mode: minimum threshold for AP provided auth level\n"
      "For AP mode: provided auth level\n"
    ),
    .enum_type = { .value = &wifi_config.auth_mode, .values = wifi_auth_mode_enum },
  },
  { CONFIG_TYPE_UINT16, "channel",
    .description = (
      "For STA mode: connect to AP with given SSID\n"
      "For AP mode: start AP on given channel\n"
    ),
    .uint16_type = { .value = &wifi_config.channel, .max = WIFI_CONFIG_CHANNEL_MAX },
  },
  { CONFIG_TYPE_STRING, "ssid",
    .description = (
      "For STA mode: connect to AP with given SSID\n"
      "For AP mode: start AP with given SSID, or use default\n"
    ),
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

/* Wifi must be started */
static int config_wifi_interface(const struct wifi_config *config, wifi_interface_t wifi_if, tcpip_adapter_if_t tcpip_if)
{
  char hostname[TCPIP_HOSTNAME_MAX_SIZE];
  uint8_t mac[6];
  esp_err_t err;

  if (config->hostname[0]) {
    LOG_INFO("Using config hostname: %s", config->hostname);

    strlcpy(hostname, config->hostname, sizeof(hostname));

  } else if ((err = esp_wifi_get_mac(wifi_if, mac))) {
    LOG_ERROR("esp_wifi_get_mac %s: %s", wifi_interface_str(wifi_if), esp_err_to_name(err));
    return -1;

  } else {
    LOG_INFO("Using %s default hostname: " WIFI_HOSTNAME_FMT, wifi_interface_str(wifi_if), mac[3], mac[4], mac[5]);

    snprintf(hostname, sizeof(hostname), WIFI_HOSTNAME_FMT, mac[3], mac[4], mac[5]);
  }

  if ((err = tcpip_adapter_set_hostname(tcpip_if, hostname))) {
    LOG_ERROR("tcpip_adapter_set_hostname %s: %s", tcpip_adapter_if_str(tcpip_if), esp_err_to_name(err));
    return -1;
  }

  return 0;
}

static int config_wifi_off(const struct wifi_config *config)
{
  if (wifi_close()) {
    LOG_ERROR("wifi_close");
    return -1;
  }

  return 0;
}

static int config_wifi_sta(const struct wifi_config *config)
{
  wifi_sta_config_t sta_config = {
    .channel = config->channel,
    .threshold = {
      .authmode = config->auth_mode,
    },
  };
  esp_err_t err;

  strncpy((char *) sta_config.ssid, config->ssid, sizeof(sta_config.ssid));
  strncpy((char *) sta_config.password, config->password, sizeof(sta_config.password));

  if (!config->password[0]) {
    sta_config.threshold.authmode = WIFI_AUTH_OPEN;
  }

  // ignore default AP mode/config
  if ((err = esp_wifi_set_mode(WIFI_MODE_STA))) {
    LOG_ERROR("esp_wifi_set_mode WIFI_MODE_STA: %s", esp_err_to_name(err));
    return -1;
  }

  if (wifi_connect(&sta_config)) {
    LOG_ERROR("wifi_connect");
    return -1;
  }

  if ((err = config_wifi_interface(config, WIFI_IF_STA, TCPIP_ADAPTER_IF_STA))) {
    LOG_ERROR("config_wifi_interface");
    return err;
  }

  return 0;
}

static int config_wifi_ap(const struct wifi_config *config)
{
  uint8_t mac[6];
  wifi_ap_config_t ap_config = {
    .channel = config->channel,
    .authmode = config->auth_mode,
    .max_connection = WIFI_AP_MAX_CONNECTION,
  };
  esp_err_t err;

  strncpy((char *) ap_config.password, config->password, sizeof(ap_config.password));

  if (config->ssid[0]) {
    LOG_INFO("Using config ssid: %s", config->ssid);

    strncpy((char *) ap_config.ssid, config->ssid, sizeof(ap_config.ssid));

  } else if ((err = esp_wifi_get_mac(WIFI_IF_AP, mac))) {
    LOG_ERROR("esp_wifi_get_mac WIFI_IF_AP: %s", esp_err_to_name(err));
    return -1;

  } else {
    LOG_INFO("Using default ssid: " WIFI_SSID_FMT, mac[3], mac[4], mac[5]);

    snprintf((char *) ap_config.ssid, sizeof(ap_config.ssid), WIFI_SSID_FMT, mac[3], mac[4], mac[5]);
  }

  if (!config->password[0]) {
    ap_config.authmode = WIFI_AUTH_OPEN;
  }

  if (wifi_listen(&ap_config)) {
    LOG_ERROR("wifi_listen");
    return -1;
  }

  if ((err = config_wifi_interface(config, WIFI_IF_AP, TCPIP_ADAPTER_IF_AP))) {
    LOG_ERROR("config_wifi_interface");
    return err;
  }

  return 0;
}

int config_wifi(const struct wifi_config *config)
{
  switch(config->mode) {
    case WIFI_MODE_NULL:
      return config_wifi_off(config);

    case WIFI_MODE_STA:
      return config_wifi_sta(config);

    case WIFI_MODE_AP:
    case WIFI_MODE_APSTA:
      return config_wifi_ap(config);

    default:
      LOG_ERROR("Unkonwn mode=%d", config->mode);
      return -1;
  }
}
