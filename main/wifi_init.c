#include "wifi.h"
#include "wifi_internal.h"

#include <logging.h>

#include <esp_err.h>

int init_wifi()
{
  int err;

  if ((err = init_wifi_events())) {
    LOG_ERROR("init_wifi_events");
    return err;
  }

  if ((err = config_wifi(&wifi_config))) {
    LOG_ERROR("config_wifi");
    return err;
  }

  if ((err = start_wifi(&wifi_config))) {
    LOG_ERROR("start_wifi");
    return err;
  }

  return 0;
}
