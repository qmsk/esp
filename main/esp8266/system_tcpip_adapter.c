#include "system_tcpip_adapter.h"
#include "../system_init.h"
#include "../artnet.h"

#include <logging.h>

#include <esp_wifi.h>
#include <tcpip_adapter.h>
#include <esp_err.h>

// last connected tcpip_adapter
tcpip_adapter_if_t connected_tcpip_adapter = TCPIP_ADAPTER_IF_MAX;

int init_system_network()
{
  esp_err_t err;

  LOG_INFO("init network stack...");

  tcpip_adapter_init();

  if ((err = esp_event_loop_create_default())) {
    LOG_ERROR("esp_event_loop_create_default: %s", esp_err_to_name(err));
    return -1;
  }

  return 0;
}

void system_tcpip_adapter_connected(tcpip_adapter_if_t tcpip_if)
{
  connected_tcpip_adapter = tcpip_if;

  update_artnet_network();
}

void system_tcpip_adapter_disconnected()
{
  connected_tcpip_adapter = TCPIP_ADAPTER_IF_MAX;
}

int get_system_mac(uint8_t mac[6])
{
  esp_err_t err;

  switch(connected_tcpip_adapter) {
    case TCPIP_ADAPTER_IF_MAX:
      // not connected
      return 1;

    case TCPIP_ADAPTER_IF_STA:
      if ((err = esp_wifi_get_mac(WIFI_IF_STA, mac))) {
        LOG_ERROR("esp_wifi_get_mac WIFI_MODE_STA: %s", esp_err_to_name(err));
        return -1;
      }

      return 0;

    case TCPIP_ADAPTER_IF_AP:
      if ((err = esp_wifi_get_mac(WIFI_IF_AP, mac))) {
        LOG_ERROR("esp_wifi_get_mac WIFI_MODE_AP: %s", esp_err_to_name(err));
        return -1;
      }

      return 0;

    default:
      LOG_WARN("unsupported connected_tcpip_adapter=%d", connected_tcpip_adapter);
      return -1;
  }
}

int get_system_hostname(const char **hostnamep)
{
  esp_err_t err;

  switch(connected_tcpip_adapter) {
    case TCPIP_ADAPTER_IF_MAX:
      // not connected
      return 1;

    case TCPIP_ADAPTER_IF_STA:
    case TCPIP_ADAPTER_IF_AP:
    case TCPIP_ADAPTER_IF_ETH:
      if ((err = tcpip_adapter_get_hostname(connected_tcpip_adapter, hostnamep))) {
        LOG_ERROR("tcpip_adapter_get_hostname %d: %s", connected_tcpip_adapter, esp_err_to_name(err));
        return -1;
      }

      return 0;

    default:
      LOG_WARN("unsupported connected_tcpip_adapter=%d", connected_tcpip_adapter);
      return -1;
  }
}

int get_system_ipv4_addr(ip4_addr_t *ip_addr)
{
  tcpip_adapter_ip_info_t ip_info;
  esp_err_t err;

  switch(connected_tcpip_adapter) {
    case TCPIP_ADAPTER_IF_MAX:
      // not connected
      return 1;

    case TCPIP_ADAPTER_IF_STA:
    case TCPIP_ADAPTER_IF_AP:
    case TCPIP_ADAPTER_IF_ETH:
      if ((err = tcpip_adapter_get_ip_info(connected_tcpip_adapter, &ip_info))) {
        LOG_ERROR("tcpip_adapter_get_ip_info %d: %s", connected_tcpip_adapter, esp_err_to_name(err));
        return -1;
      }

      *ip_addr = ip_info.ip;

      return 0;

    default:
      LOG_WARN("unsupported connected_tcpip_adapter=%d", connected_tcpip_adapter);
      return -1;
  }
}
