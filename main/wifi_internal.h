#pragma once

#include <esp_wifi_types.h>

// config defaults
#define WIFI_AP_AUTHMODE_DEFAULT WIFI_AUTH_WPA2_PSK
#define WIFI_AP_MAX_CONNECTION_DEFAULT 10

#define WIFI_STA_THRESHOLD_AUTHMODE_DEFAULT WIFI_AUTH_WPA2_PSK

// state
extern bool wifi_sta_connect, wifi_sta_started, wifi_sta_connected;
extern bool wifi_ap_listen, wifi_ap_started;
extern unsigned wifi_ap_connected; // count of stations

// actions
int wifi_scan(const wifi_scan_config_t *scan_config, int (*cb)(wifi_ap_record_t *ap, void *ctx), void *ctx);
int wifi_listen(const wifi_ap_config_t *ap_config);
int wifi_connect(const wifi_sta_config_t *sta_config);
int wifi_disconnect();
