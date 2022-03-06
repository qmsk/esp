#pragma once

#include <esp_wifi_types.h>

// defaults
#define WIFI_AP_AUTHMODE_DEFAULT WIFI_AUTH_WPA2_PSK
#define WIFI_AP_MAX_CONNECTION_DEFAULT 10

#define WIFI_STA_THRESHOLD_AUTHMODE_DEFAULT WIFI_AUTH_WPA2_PSK

// config
struct wifi_config {
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

int config_wifi(const struct wifi_config *config);
