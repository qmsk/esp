#pragma once

#include <esp_netif_types.h>

void system_netif_up(esp_netif_t *netif);
void system_netif_connected(esp_netif_t *netif);
void system_netif_disconnected();
void system_netif_down(esp_netif_t *netif);
