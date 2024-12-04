#include "wifi.h"
#include "wifi_config.h"
#include "wifi_interface.h"
#include "wifi_state.h"

#include <logging.h>
#include <system_wifi.h>

#include <esp_wifi.h>

#include <string.h>

#include <sdkconfig.h>

#define WIFI_CONFIG_CHANNEL_MAX 13
#define WIFI_HOSTNAME_MAX_SIZE 32

#define WIFI_CONFIG_ENABLED_DEFAULT true
#define WIFI_CONFIG_MODE_DEFAULT WIFI_MODE_AP
#define WIFI_CONFIG_AUTHMODE_DEFAULT WIFI_AUTH_WPA2_PSK

#define WIFI_SSID_FMT "qmsk-esp-%02x%02x%02x"
#define WIFI_HOSTNAME_FMT "qmsk-esp-%02x%02x%02x"

struct wifi_config wifi_config = {};

const struct config_enum wifi_mode_enum[] = {
  { "OFF",    .value = WIFI_MODE_NULL  },
  { "STA",    .value = WIFI_MODE_STA   },
  { "AP",     .value = WIFI_MODE_AP    },
  // TODO: AP/STA?
  {}
};

const struct config_enum wifi_auth_mode_enum[] = {
  { "OPEN",           .value = WIFI_AUTH_OPEN },
  { "WEP",            .value = WIFI_AUTH_WEP },
  { "WPA-PSK",        .value = WIFI_AUTH_WPA_PSK },
  { "WPA2-PSK",       .value = WIFI_AUTH_WPA2_PSK },
  { "WPA/2-PSK",      .value = WIFI_AUTH_WPA_WPA2_PSK },
  { "WPA3-PSK",       .value = WIFI_AUTH_WPA3_PSK },
  { "WPA2/3-PSK",     .value = WIFI_AUTH_WPA2_WPA3_PSK },
#if !CONFIG_IDF_TARGET_ESP8266
  { "WPAI-PSK",       .value = WIFI_AUTH_WAPI_PSK },
#endif
  {}
};

const struct configtab wifi_configtab[] = {
  { CONFIG_TYPE_BOOL, "enabled",
    .description = (
      "Only start WiFi in config mode."
    ),
    .bool_type = { .value = &wifi_config.enabled, .default_value = WIFI_CONFIG_ENABLED_DEFAULT },
  },
  { CONFIG_TYPE_BOOL, "config_only_mode",
    .description = (
      "Only start WiFi in config mode."
    ),
    .bool_type = { .value = &wifi_config.config_only_mode },
  },
  { CONFIG_TYPE_ENUM, "mode",
    .enum_type = { .value = &wifi_config.mode, .values = wifi_mode_enum, .default_value = WIFI_CONFIG_MODE_DEFAULT },
  },
  { CONFIG_TYPE_ENUM, "auth_mode",
    .description = (
      "For STA mode: minimum threshold for AP provided auth level\n"
      "For AP mode: provided auth level, assuming password is set\n"
    ),
    .enum_type = { .value = &wifi_config.auth_mode, .values = wifi_auth_mode_enum, .default_value = WIFI_CONFIG_AUTHMODE_DEFAULT },
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
    .string_type = { .value = wifi_config.password, .size = sizeof(wifi_config.password) },
    .secret = true,
  },
  { CONFIG_TYPE_STRING, "hostname",
    .string_type = { .value = wifi_config.hostname, .size = sizeof(wifi_config.hostname) },
  },
  { CONFIG_TYPE_STRING, "ip",
    .string_type = { .value = wifi_config.ip, .size = sizeof(wifi_config.ip) },
  },
  { CONFIG_TYPE_STRING, "netmask",
    .string_type = { .value = wifi_config.netmask, .size = sizeof(wifi_config.netmask) },
  },
  { CONFIG_TYPE_STRING, "gw",
    .string_type = { .value = wifi_config.gw, .size = sizeof(wifi_config.gw) },
  },
  {}
};

static int set_wifi_ssid(void *buf, size_t size, const char *value, wifi_interface_t interface)
{
  uint8_t mac[6];
  esp_err_t err;

  if (strlen(value) > 0) {
    LOG_INFO("Using configured ssid: %s", value);

    strncpy(buf, value, size);

  } else if ((err = esp_wifi_get_mac(interface, mac))) {
    LOG_ERROR("esp_wifi_get_mac: %s", esp_err_to_name(err));
    return -1;

  } else {
    LOG_INFO("Using default ssid: " WIFI_SSID_FMT, mac[3], mac[4], mac[5]);

    snprintf(buf, size, WIFI_SSID_FMT, mac[3], mac[4], mac[5]);
  }

  return 0;
}

static int set_wifi_password(void *buf, size_t size, const char *value, wifi_auth_mode_t *set_automode)
{
  strncpy(buf, value, size);

  if (!value[0] && set_automode) {
    *set_automode = WIFI_AUTH_OPEN;
  }

  return 0;
}

void get_wifi_hostname(wifi_interface_t interface, const struct wifi_config *config, char *buf, size_t size)
{
  uint8_t mac[6];
  bool use_config_hostname;
  esp_err_t err;

  if (strlen(config->hostname) >= size) {
    LOG_WARN("Invalid hostname with len=%d > max=%u", strlen(config->hostname), size);

    use_config_hostname = false;
  } else if (strlen(config->hostname) > 0) {
    use_config_hostname = true;
  } else {
    use_config_hostname = false;
  }

  if (use_config_hostname) {
    LOG_INFO("Using config hostname: %s", config->hostname);

    strlcpy(buf, config->hostname, size);

  } else if ((err = esp_wifi_get_mac(interface, mac))) {
    LOG_ERROR("esp_wifi_get_mac %s: %s", wifi_interface_str(interface), esp_err_to_name(err));

    snprintf(buf, size, WIFI_HOSTNAME_FMT, 0x00, 0x00, 0x00);

  } else {
    LOG_INFO("Using %s default hostname: " WIFI_HOSTNAME_FMT, wifi_interface_str(interface), mac[3], mac[4], mac[5]);

    snprintf(buf, size, WIFI_HOSTNAME_FMT, mac[3], mac[4], mac[5]);
  }
}

static int config_wifi_hostname(wifi_interface_t interface, const struct wifi_config *config)
{
  char hostname[WIFI_HOSTNAME_MAX_SIZE];
  esp_err_t err;

  get_wifi_hostname(interface, config, hostname, sizeof(hostname));

  if ((err = set_wifi_interface_hostname(interface, hostname))) {
    LOG_ERROR("set_wifi_interface_hostname");
    return err;
  }

  return 0;
}

static int config_wifi_network(wifi_interface_t interface, const struct wifi_config *config)
{
  int err;

  if (!config->ip[0] || !config->netmask[0] || !config->gw[0]) {
    LOG_INFO("Using IPv4 default DHCP addressing");
    return 0;
  }

  LOG_INFO("Using IPv4 config: ip=%s netmask=%s gw=%s", config->ip, config->netmask, config->gw);

  if ((err = set_wifi_interface_ip(interface, config->ip, config->netmask, config->gw))) {
    LOG_ERROR("set_wifi_interface_ip");
    return err;
  }

  return 0;
}

static int config_wifi_dhcps(wifi_interface_t interface)
{
  int err;

  LOG_INFO("Disabling IPv4 DHCP Server Router/DNS offer");

  if ((err = set_wifi_interface_dhcps_options(interface))) {
    LOG_ERROR("set_wifi_interface_dhcps_options");
    return err;
  }

  return 0;
}

static int config_wifi_null(const struct wifi_config *config)
{
  esp_err_t err;

  if ((err = esp_wifi_set_mode(WIFI_MODE_NULL))) {
    LOG_ERROR("esp_wifi_set_mode: %s", esp_err_to_name(err));
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

  if ((err = set_wifi_ssid(&sta_config.ssid, sizeof(sta_config.ssid), config->ssid, WIFI_IF_STA))) {
    LOG_ERROR("set_wifi_ssid");
    return err;
  }

  if ((err = set_wifi_password(&sta_config.password, sizeof(sta_config.password), config->password, &sta_config.threshold.authmode))) {
    LOG_ERROR("set_wifi_ssid");
    return err;
  }

  if ((err = config_wifi_hostname(WIFI_IF_STA, config))) {
    LOG_ERROR("config_wifi_hostname");
    return err;
  }

  if ((err = config_wifi_network(WIFI_IF_STA, config))) {
    LOG_ERROR("config_wifi_network");
    return err;
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

  return 0;
}

static int config_wifi_ap(const struct wifi_config *config)
{
  wifi_ap_config_t ap_config = {
    .channel = config->channel,
    .authmode = config->auth_mode,
    .max_connection = WIFI_AP_MAX_CONNECTION_DEFAULT,
  };
  esp_err_t err;

  if ((err = set_wifi_ssid(&ap_config.ssid, sizeof(ap_config.ssid), config->ssid, WIFI_IF_AP))) {
    LOG_ERROR("set_wifi_ssid");
    return err;
  }

  if ((err = set_wifi_password(&ap_config.password, sizeof(ap_config.password), config->password, &ap_config.authmode))) {
    LOG_ERROR("set_wifi_ssid");
    return err;
  }

  if ((err = config_wifi_hostname(WIFI_IF_AP, config))) {
    LOG_ERROR("config_wifi_hostname");
    return err;
  }

  if ((err = config_wifi_network(WIFI_IF_AP, config))) {
    LOG_ERROR("config_wifi_network");
    return err;
  }

  if ((err = config_wifi_dhcps(WIFI_IF_AP))) {
    LOG_ERROR("config_wifi_dhcps");
    return err;
  }

  // ensure default AP mode/config
  if ((err = esp_wifi_set_mode(WIFI_MODE_AP))) {
    LOG_ERROR("esp_wifi_set_mode WIFI_MODE_AP: %s", esp_err_to_name(err));
    return -1;
  }

  if (wifi_listen(&ap_config)) {
    LOG_ERROR("wifi_listen");
    return -1;
  }

  return 0;
}

int config_wifi(const struct wifi_config *config)
{
  switch(config->mode) {
    case WIFI_MODE_NULL:
      return config_wifi_null(config);

    case WIFI_MODE_STA:
      return config_wifi_sta(config);

    case WIFI_MODE_AP:
      return config_wifi_ap(config);

    default:
      LOG_ERROR("Unknown mode=%d", config->mode);
      return -1;
  }

  return 0;
}
