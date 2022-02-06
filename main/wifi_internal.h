#pragma once

#include <esp_wifi_types.h>

#define WIFI_AP_AUTHMODE_DEFAULT WIFI_AUTH_WPA2_PSK
#define WIFI_AP_MAX_CONNECTION_DEFAULT 10

#define WIFI_STA_THRESHOLD_AUTHMODE_DEFAULT WIFI_AUTH_WPA2_PSK

int wifi_scan(const wifi_scan_config_t *scan_config, int (*cb)(wifi_ap_record_t *ap, void *ctx), void *ctx);
int wifi_listen(const wifi_ap_config_t *ap_config);
