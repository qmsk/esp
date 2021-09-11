#pragma once

#include <esp_wifi.h>

#define WIFI_AUTHMODE_THRESHOLD WIFI_AUTH_WPA2_PSK

const char *wifi_auth_mode_str(wifi_auth_mode_t auth_mode);
const char *wifi_cipher_type_str(wifi_cipher_type_t cipher_type);
const char *tcpip_adapter_dhcp_status_str(tcpip_adapter_dhcp_status_t status);
const char *tcpip_adapter_if_str(tcpip_adapter_if_t adapter_if);

extern struct wifi_config {
  bool enabled;
  char ssid[32];
  char password[64];
  char hostname[32];
} wifi_config;

int init_wifi_hostname(const struct wifi_config *config);
int init_wifi_config(const struct wifi_config *config);
int init_wifi_events();

int wifi_scan(const wifi_scan_config_t *scan_config);
int wifi_connect(wifi_config_t *config);
int wifi_info();

int tcpip_adapter_info();
