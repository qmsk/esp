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


  if ((err = init_wifi_config(&wifi_config))) {
    LOG_ERROR("init_wifi_config");
    return err;
  }

  return 0;
}
