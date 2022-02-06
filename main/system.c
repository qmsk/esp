#include "system.h"

#include <logging.h>

#include <esp_event.h>
#include <esp_err.h>
#include <esp_netif.h>

int init_system()
{
  esp_err_t err;

  LOG_INFO("init esp_netif and create default event loop...");

  if ((err = esp_netif_init())) {
    LOG_ERROR("esp_netif_init: %s", esp_err_to_name(err));
    return -1;
  }

  if ((err = esp_event_loop_create_default())) {
    LOG_ERROR("esp_event_loop_create_default: %s", esp_err_to_name(err));
    return -1;
  }

  if ((err = init_system_events())) {
    LOG_ERROR("init_system_events");
    return err;
  }

  return 0;
}
