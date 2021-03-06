#pragma once

#include <esp_wifi.h>

#define WIFI_AUTHMODE_THRESHOLD WIFI_AUTH_WPA2_PSK

extern struct wifi_config {
  bool enabled;
  char ssid[32];
  char password[64];
} wifi_config;

int init_wifi_config(const struct wifi_config *config);

int wifi_scan(const wifi_scan_config_t *scan_config);
int wifi_connect(wifi_config_t *config);
int wifi_info();

int tcpip_adapter_info();
