#pragma once

#include <esp_netif_types.h>
#include <esp_wifi_types.h>

// state
extern bool wifi_sta_connect, wifi_sta_started, wifi_sta_connected;
extern bool wifi_ap_listen, wifi_ap_started;
extern unsigned wifi_ap_connected; // count of stations

// netif
int init_wifi_netif(wifi_interface_t interface);
int make_wifi_netif(esp_netif_t **netifp, wifi_interface_t interface);

// actions
int wifi_scan(const wifi_scan_config_t *scan_config, int (*cb)(wifi_ap_record_t *ap, void *ctx), void *ctx);
int wifi_listen(const wifi_ap_config_t *ap_config);
int wifi_connect(const wifi_sta_config_t *sta_config);
int wifi_disconnect();
