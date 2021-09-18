#include "wifi.h"
#include "wifi_internal.h"
#include "user_event.h"

#include <logging.h>

#include <string.h>

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

int config_wifi(const struct wifi_config *config)
{
  wifi_init_config_t init_config = WIFI_INIT_CONFIG_DEFAULT();
  esp_err_t err;

  LOG_INFO("mode=%s", wifi_mode_str(config->mode));

  if ((err = esp_wifi_init(&init_config))) {
    LOG_ERROR("esp_wifi_init: %s", esp_err_to_name(err));
    return -1;
  }

  if ((err = esp_wifi_set_storage(WIFI_STORAGE_RAM))) {
    LOG_ERROR("esp_wifi_set_storage: %s", esp_err_to_name(err));
    return -1;
  }

  if ((err = esp_wifi_set_mode(config->mode))) {
    LOG_ERROR("esp_wifi_set_mode WIFI_MODE_STA: %s", esp_err_to_name(err));
    return -1;
  }

  return 0;
}

/* Wifi must be started */
static int config_wifi_sta(const struct wifi_config *config)
{
  char hostname[TCPIP_HOSTNAME_MAX_SIZE];
  uint8_t mac[6];
  esp_err_t err;

  if (config->hostname[0]) {
    LOG_INFO("Using config hostname: %s", config->hostname);

    strlcpy(hostname, config->hostname, sizeof(hostname));

  } else if ((err = esp_wifi_get_mac(WIFI_IF_STA, mac))) {
    LOG_ERROR("esp_wifi_get_mac WIFI_IF_STA: %s", esp_err_to_name(err));
    return -1;

  } else {
    LOG_INFO("Using STA default hostname: " WIFI_HOSTNAME_FMT, mac[3], mac[4], mac[5]);

    snprintf(hostname, sizeof(hostname), WIFI_HOSTNAME_FMT, mac[3], mac[4], mac[5]);
  }

  if ((err = tcpip_adapter_set_hostname(TCPIP_ADAPTER_IF_STA, hostname))) {
    LOG_ERROR("tcpip_adapter_set_hostname TCPIP_ADAPTER_IF_STA: %s", esp_err_to_name(err));
    return -1;
  }

  return 0;
}

/* Wifi must be started */
static int config_wifi_ap(const struct wifi_config *config)
{
  char hostname[TCPIP_HOSTNAME_MAX_SIZE];
  uint8_t mac[6];
  esp_err_t err;

  if (config->hostname[0]) {
    LOG_INFO("Using config hostname: %s", config->hostname);

    strlcpy(hostname, config->hostname, sizeof(hostname));

  } else if ((err = esp_wifi_get_mac(WIFI_IF_AP, mac))) {
    LOG_ERROR("esp_wifi_get_mac WIFI_IF_AP: %s", esp_err_to_name(err));
    return -1;

  } else {
    LOG_INFO("Using AP default hostname: " WIFI_HOSTNAME_FMT, mac[3], mac[4], mac[5]);

    snprintf(hostname, sizeof(hostname), WIFI_HOSTNAME_FMT, mac[3], mac[4], mac[5]);
  }

  if ((err = tcpip_adapter_set_hostname(TCPIP_ADAPTER_IF_AP, hostname))) {
    LOG_ERROR("tcpip_adapter_set_hostname TCPIP_ADAPTER_IF_AP: %s", esp_err_to_name(err));
    return -1;
  }

  return 0;
}

static int start_wifi_sta(const struct wifi_config *config)
{
  wifi_config_t wifi_config = {
    .sta = {
      .threshold = {
        .authmode = config->auth_mode,
      },
    },
  };
  esp_err_t err;

  strncpy((char *) wifi_config.sta.ssid, config->ssid, sizeof(wifi_config.sta.ssid));
  strncpy((char *) wifi_config.sta.password, config->password, sizeof(wifi_config.sta.password));

  if (!config->password[0]) {
    wifi_config.sta.threshold.authmode = WIFI_AUTH_OPEN;
  }

  LOG_INFO("Start Wifi Station");

  if ((err = esp_wifi_start())) {
    LOG_ERROR("esp_wifi_start: %s", esp_err_to_name(err));
    return -1;
  }

  if ((err = config_wifi_sta(config))) {
    LOG_ERROR("config_wifi_sta");
    return err;
  }

  if (wifi_connect(&wifi_config)) {
    LOG_ERROR("wifi_connect");
    return -1;
  }

  return 0;
}

static int start_wifi_ap(const struct wifi_config *config)
{
  uint8_t mac[6];
  wifi_config_t wifi_config = {
    .ap = {
      .authmode = config->auth_mode,
      .max_connection = WIFI_AP_MAX_CONNECTION,
    }
  };
  esp_err_t err;

  strncpy((char *) wifi_config.ap.password, config->password, sizeof(wifi_config.ap.password));

  if (config->ssid[0]) {
    LOG_INFO("Using config ssid: %s", config->ssid);

    strncpy((char *) wifi_config.ap.ssid, config->ssid, sizeof(wifi_config.ap.ssid));

  } else if ((err = esp_wifi_get_mac(WIFI_IF_AP, mac))) {
    LOG_ERROR("esp_wifi_get_mac WIFI_IF_AP: %s", esp_err_to_name(err));
    return -1;

  } else {
    LOG_INFO("Using default ssid: " WIFI_SSID_FMT, mac[3], mac[4], mac[5]);

    snprintf((char *) wifi_config.ap.ssid, sizeof(wifi_config.ap.ssid), WIFI_SSID_FMT, mac[3], mac[4], mac[5]);
  }

  if (!config->password[0]) {
    wifi_config.ap.authmode = WIFI_AUTH_OPEN;
  }

  LOG_INFO("ssid=%.32s password=%s authmode=%s ssid_hidden=%s max_connection=%d",
    wifi_config.ap.ssid,
    wifi_config.ap.password[0] ? "***" : "",
    wifi_auth_mode_str(wifi_config.ap.authmode),
    wifi_config.ap.ssid_hidden ? "true" : "false",
    wifi_config.ap.max_connection
  );

  if ((err = esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_config))) {
    LOG_ERROR("esp_wifi_set_config ESP_IF_WIFI_AP: %s", esp_err_to_name(err));
    return -1;
  }

  user_state(USER_STATE_CONNECTING);

  if ((err = esp_wifi_start())) {
    LOG_ERROR("esp_wifi_start: %s", esp_err_to_name(err));
    return -1;
  }

  if ((err = config_wifi_ap(config))) {
    LOG_ERROR("config_wifi_ap");
    return err;
  }

  return 0;
}

int start_wifi(const struct wifi_config *config)
{
  switch(config->mode) {
    case WIFI_MODE_NULL:
      user_state(USER_STATE_DISCONNECTED);

      return 0;

    case WIFI_MODE_STA:
      return start_wifi_sta(config);

    case WIFI_MODE_AP:
      return start_wifi_ap(config);

    default:
      LOG_ERROR("Unkonwn mode=%d", config->mode);
      return -1;
  }
}
