#include "system.h"

#include <logging.h>

#include <tcpip_adapter.h>
#include <esp_system.h>
#include <esp_wifi.h>

int get_system_mac(uint8_t mac[6])
{
  wifi_mode_t wifi_mode;
  esp_err_t err;

  if ((err = esp_wifi_get_mode(&wifi_mode))) {
    LOG_ERROR("esp_wifi_get_mode: %s", esp_err_to_name(err));
    return -1;
  }

  switch(wifi_mode) {
    case WIFI_MODE_NULL:
      LOG_WARN("WiFi not configured");
      return 1;

    case WIFI_MODE_STA:
      if ((err = esp_wifi_get_mac(WIFI_IF_STA, mac))) {
        LOG_ERROR("esp_wifi_get_mac WIFI_MODE_STA: %s", esp_err_to_name(err));
        return -1;
      }

      return 0;

    case WIFI_MODE_AP:
      if ((err = esp_wifi_get_mac(WIFI_IF_AP, mac))) {
        LOG_ERROR("esp_wifi_get_mac WIFI_MODE_AP: %s", esp_err_to_name(err));
        return -1;
      }

      return 0;

    default:
      LOG_WARN("unknown wifi_mode=%d", wifi_mode);
      return 1;
  }
}

int get_system_hostname(const char **hostnamep)
{
  wifi_mode_t wifi_mode;
  esp_err_t err;

  if ((err = esp_wifi_get_mode(&wifi_mode))) {
    LOG_ERROR("esp_wifi_get_mode: %s", esp_err_to_name(err));
    return -1;
  }

  switch(wifi_mode) {
    case WIFI_MODE_NULL:
      LOG_WARN("WiFi not configured");
      return 1;


    case WIFI_MODE_STA:
      if ((err = tcpip_adapter_get_hostname(TCPIP_ADAPTER_IF_STA, hostnamep))) {
        LOG_ERROR("tcpip_adapter_get_hostname TCPIP_ADAPTER_IF_STA: %s", esp_err_to_name(err));
        return -1;
      }

      return 0;

    case WIFI_MODE_AP:
      if ((err = tcpip_adapter_get_hostname(TCPIP_ADAPTER_IF_AP, hostnamep))) {
        LOG_ERROR("tcpip_adapter_get_hostname TCPIP_ADAPTER_IF_AP: %s", esp_err_to_name(err));
        return -1;
      }

      return 0;

    default:
      LOG_WARN("unknown wifi_mode=%d", wifi_mode);
      return 1;
  }
}

int get_system_ipv4_addr(ip4_addr_t *ip_addr)
{
  wifi_mode_t wifi_mode;
  tcpip_adapter_ip_info_t ip_info;
  esp_err_t err;

  if ((err = esp_wifi_get_mode(&wifi_mode))) {
    LOG_ERROR("esp_wifi_get_mode: %s", esp_err_to_name(err));
    return -1;
  }

  switch(wifi_mode) {
    case WIFI_MODE_NULL:
      LOG_WARN("WiFi not configured");
      return 1;

    case WIFI_MODE_STA:
      if ((err = tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_STA, &ip_info))) {
        LOG_ERROR("tcpip_adapter_get_ip_info TCPIP_ADAPTER_IF_STA: %s", esp_err_to_name(err));
        return -1;
      }

      *ip_addr = ip_info.ip;

      return 0;

    case WIFI_MODE_AP:
      if ((err = tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_AP, &ip_info))) {
        LOG_ERROR("tcpip_adapter_get_ip_info TCPIP_ADAPTER_IF_AP: %s", esp_err_to_name(err));
        return -1;
      }

      *ip_addr = ip_info.ip;

      return 0;

    default:
      LOG_WARN("unknown wifi_mode=%d", wifi_mode);
      return 1;
  }
}

void system_restart()
{
  LOG_INFO("restarting...");

  esp_restart();
}
