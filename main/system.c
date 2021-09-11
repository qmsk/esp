#include "system.h"

#include <logging.h>

#include <tcpip_adapter.h>
#include <esp_wifi.h>

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
      LOG_INFO("using STA hostname");
      if ((err = tcpip_adapter_get_hostname(TCPIP_ADAPTER_IF_STA, hostnamep))) {
        LOG_ERROR("tcpip_adapter_get_hostname TCPIP_ADAPTER_IF_STA: %s", esp_err_to_name(err));
        return -1;
      }

      return 0;

    case WIFI_MODE_AP:
      LOG_INFO("using AP hostname");

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
