#pragma once

#include <esp_wifi_types.h>

/* platform-specific implementations under esp32/wifi_netif.c, esp8266/wifi_tcpip_adapter.c */
int init_wifi_interface(wifi_interface_t interface);
int set_wifi_interface_hostname(wifi_interface_t interface, const char *hostname);
int set_wifi_interface_ip(wifi_interface_t interface, const char *ip, const char *netmask, const char *gw);

void on_wifi_interface_up(wifi_interface_t interface, bool connected);
void on_wifi_interface_down(wifi_interface_t interface);
