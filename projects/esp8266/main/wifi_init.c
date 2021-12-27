#include "wifi.h"
#include "wifi_init.h"

#include <logging.h>

#include <esp_err.h>

int init_wifi_system()
{
  wifi_init_config_t init_config = WIFI_INIT_CONFIG_DEFAULT();
  esp_err_t err;

  if ((err = esp_wifi_init(&init_config))) {
    LOG_ERROR("esp_wifi_init: %s", esp_err_to_name(err));
    return -1;
  }

  if ((err = esp_wifi_set_storage(WIFI_STORAGE_RAM))) {
    LOG_ERROR("esp_wifi_set_storage: %s", esp_err_to_name(err));
    return -1;
  }

  return 0;
}

int init_wifi()
{
  int err;

  if ((err = init_wifi_timer())) {
    LOG_ERROR("init_wifi_timer");
    return err;
  }

  if ((err = init_wifi_events())) {
    LOG_ERROR("init_wifi_events");
    return err;
  }

  if ((err = init_wifi_system())) {
    LOG_ERROR("init_wifi_system");
    return err;
  }

  if ((err = config_wifi(&wifi_config))) {
    LOG_ERROR("config_wifi");
    return err;
  }

  return 0;
}
