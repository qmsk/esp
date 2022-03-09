#include "system_netif.h"
#include "../system_init.h"
#include "../system_network.h"
#include "../artnet.h"

#include <logging.h>

#include <esp_netif.h>
#include <esp_err.h>
#include <esp_event.h>

// last connected netif
esp_netif_t *configured_netif, *connected_netif;

int init_system_network()
{
  esp_err_t err;

  LOG_INFO("init netif...");

  if ((err = esp_netif_init())) {
    LOG_ERROR("esp_netif_init: %s", esp_err_to_name(err));
    return -1;
  }

  return 0;
}

void system_netif_up(esp_netif_t *netif)
{
  LOG_INFO("netif=%s configured", esp_netif_get_desc(netif));

  configured_netif = netif;
}

void system_netif_connected(esp_netif_t *netif)
{
  LOG_INFO("netif=%s connected", esp_netif_get_desc(netif));

  connected_netif = netif;

  update_artnet_network();
}

void system_netif_disconnected()
{
  if (connected_netif) {
    LOG_INFO("netif=%s disconnected", esp_netif_get_desc(connected_netif));

    connected_netif = NULL;
  } else {
    LOG_WARN("unknown netif=NULL disconnected");
  }
}

void system_netif_down(esp_netif_t *netif)
{
  if (netif == configured_netif) {
    LOG_INFO("configured netif=%s down", esp_netif_get_desc(netif));
    configured_netif = NULL;
  } else {
    LOG_WARN("unknown netif=%s down", esp_netif_get_desc(netif));
  }
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

  if (!configured_netif) {
    return 1;
  } else if ((err = esp_netif_get_mac(configured_netif, mac))) {
    LOG_ERROR("esp_netif_get_mac: %s", esp_err_to_name(err));
    return -1;
  }

  return 0;
}

int get_system_hostname(const char **hostnamep)
{
  esp_err_t err;

  if (!configured_netif) {
    return 1;
  } else if ((err = esp_netif_get_hostname(configured_netif, hostnamep))) {
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
