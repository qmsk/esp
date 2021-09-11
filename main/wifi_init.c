#include "wifi.h"
#include "wifi_internal.h"

#include <logging.h>

#include <esp_err.h>
#include <esp_event.h>
#include <mdns.h>
#include <tcpip_adapter.h>

int init_tcpip_adapter()
{
  tcpip_adapter_init();

  return 0;
}

int init_esp_event()
{
  esp_err_t err;

  if ((err = esp_event_loop_create_default())) {
    LOG_ERROR("esp_event_loop_create_default: %s", esp_err_to_name(err));
    return -1;
  }

  return 0;
}

int init_wifi_sta()
{
  wifi_init_config_t init_config = WIFI_INIT_CONFIG_DEFAULT();
  esp_err_t err;

  LOG_INFO("Start ESP8266 WiFI in Station mode");

  if ((err = esp_wifi_init(&init_config))) {
    LOG_ERROR("esp_wifi_init: %s", esp_err_to_name(err));
    return -1;
  }

  if ((err = esp_wifi_set_storage(WIFI_STORAGE_RAM))) {
    LOG_ERROR("esp_wifi_set_storage: %s", esp_err_to_name(err));
    return -1;
  }

  if ((err = esp_wifi_set_mode(WIFI_MODE_STA))) {
    LOG_ERROR("esp_wifi_set_mode WIFI_MODE_STA: %s", esp_err_to_name(err));
    return -1;
  }

  if ((err = esp_wifi_start())) {
    LOG_ERROR("esp_wifi_start: %s", esp_err_to_name(err));
    return -1;
  }

  return 0;
}

#if CONFIG_ENABLE_MDNS
int init_mdns()
{
  const char *hostname;
  esp_err_t err;

  if ((err = tcpip_adapter_get_hostname(TCPIP_ADAPTER_IF_STA, &hostname))) {
    LOG_ERROR("tcpip_adapter_get_hostname TCPIP_ADAPTER_IF_STA: %s", esp_err_to_name(err));
    return -1;
  }

  LOG_INFO("hostname=%s", hostname);

  if ((err = mdns_init())) {
    LOG_ERROR("mdns_init: %s", esp_err_to_name(err));
    return -1;
  }

  if ((err = mdns_hostname_set(hostname))) {
    LOG_ERROR("mdns_hostname_set: %s", esp_err_to_name(err));
    return -1;
  }

  return 0;
}
#endif

int init_wifi()
{
  int err;

  if ((err = init_tcpip_adapter())) {
    LOG_ERROR("init_tcpip_adapter");
    return err;
  }

  if ((err = init_esp_event())) {
    LOG_ERROR("init_esp_event");
    return err;
  }

  if ((err = init_wifi_events())) {
    LOG_ERROR("init_wifi_events");
    return err;
  }

  if ((err = init_wifi_sta())) {
    LOG_ERROR("init_wifi_sta");
    return err;
  }

  if ((err = init_wifi_hostname(&wifi_config))) {
    LOG_ERROR("init_wifi_hostname");
    return err;
  }

#if CONFIG_ENABLE_MDNS
  if ((err = init_mdns())) {
    LOG_ERROR("init_mdns");
    return err;
  }
#endif

  if ((err = init_wifi_config(&wifi_config))) {
    LOG_ERROR("init_wifi_config");
    return err;
  }

  return 0;
}
