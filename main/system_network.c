#include "system_network.h"
#include "system_state.h"
#include "artnet.h"

#include <logging.h>

#include <esp_err.h>

// last connected netif
esp_netif_t *connected_netif;

void system_netif_connected(esp_netif_t *netif)
{
  connected_netif = netif;

  update_artnet_network();
}

void system_netif_disconnected()
{
  connected_netif = NULL;
}

bool get_system_connected()
{
  if (!connected_netif) {
    return false;
  } else if (!esp_netif_is_netif_up(connected_netif)) {
    return false;
  } else {
    return true;
  }
}

int get_system_mac(uint8_t mac[6])
{
  esp_err_t err;

  if (!connected_netif) {
    return 1;
  } else if ((err = esp_netif_get_mac(connected_netif, mac))) {
    LOG_ERROR("esp_netif_get_mac: %s", esp_err_to_name(err));
    return -1;
  }

  return 0;
}

int get_system_hostname(const char **hostnamep)
{
  esp_err_t err;

  if (!connected_netif) {
    return 1;
  } else if ((err = esp_netif_get_hostname(connected_netif, hostnamep))) {
    LOG_ERROR("esp_netif_get_hostname: %s", esp_err_to_name(err));
    return -1;
  }

  return 0;
}

int get_system_ipv4_addr(SYSTEM_IPV4_ADDR_TYPE *ip_addr)
{
  esp_netif_ip_info_t ip_info;
  esp_err_t err;

  if (!connected_netif) {
    return 1;
  } else if ((err = esp_netif_get_ip_info(connected_netif, &ip_info))) {
    LOG_ERROR("esp_netif_get_ip_info: %s", esp_err_to_name(err));
    return -1;
  }

  *ip_addr = ip_info.ip;

  return 0;
}
