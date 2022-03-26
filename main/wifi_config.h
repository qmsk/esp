#pragma once

#include <esp_wifi_types.h>

// defaults
#define WIFI_AP_AUTHMODE_DEFAULT WIFI_AUTH_WPA2_PSK
#define WIFI_AP_MAX_CONNECTION_DEFAULT 10

#define WIFI_STA_THRESHOLD_AUTHMODE_DEFAULT WIFI_AUTH_WPA2_PSK

// config
struct wifi_config {
  bool enabled;
  bool config_only_mode;
  int mode; /* wifi_mode_t */
  int auth_mode; /* wifi_auth_mode_t */
  uint16_t channel;
  char ssid[32];
  char password[64];
  char hostname[32];
  char ip[16];
  char netmask[15];
  char gw[15];
};

// globals
extern struct wifi_config wifi_config;

void get_wifi_hostname(wifi_interface_t interface, const struct wifi_config *config, char *buf, size_t size);

int config_wifi(const struct wifi_config *config);
