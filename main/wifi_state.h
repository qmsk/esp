#pragma once

#include <esp_wifi_types.h>

// state
extern bool wifi_sta_connect, wifi_sta_started, wifi_sta_connected;
extern bool wifi_ap_listen, wifi_ap_started;
extern unsigned wifi_ap_connected; // count of stations

// actions
int wifi_scan(const wifi_scan_config_t *scan_config, int (*cb)(wifi_ap_record_t *ap, void *ctx), void *ctx);
int wifi_listen(const wifi_ap_config_t *ap_config);
int wifi_connect(const wifi_sta_config_t *sta_config);
int wifi_disconnect();
