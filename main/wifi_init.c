#include "wifi.h"
#include "wifi_internal.h"

#include <logging.h>

#include <esp_err.h>
#include <esp_event.h>
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

int init_wifi()
{

  if (init_tcpip_adapter()) {
    LOG_ERROR("init_tcpip_adapter");
    return -1;
  }

  if (init_esp_event()) {
    LOG_ERROR("init_esp_event");
    return -1;
  }

  if (init_wifi_sta()) {
    LOG_ERROR("init_wifi_sta");
    return -1;
  }

  if (init_wifi_config(&wifi_config)) {
    LOG_ERROR("init_wifi_config");
    return -1;
  }

  return 0;
}
