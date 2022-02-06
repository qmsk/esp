#include "system.h"

#include <logging.h>

#include <esp_event.h>
#include <esp_err.h>
#include <esp_netif.h>
#include <nvs_flash.h>

static int init_system_nvs_harder()
{
  esp_err_t err;

  LOG_INFO("erase nvs flash filesystem...");

  if ((err = nvs_flash_erase())) {
    LOG_ERROR("vfs_flash_erase: %s", esp_err_to_name(err));
    return -1;
  }

  if ((err = nvs_flash_init())) {
    LOG_ERROR("nvs_flash_init: %s", esp_err_to_name(err));
    return -1;
  }

  return 0;
}

static int init_system_nvs()
{
  esp_err_t err;

  LOG_INFO("init nvs flash filesystem...");

  switch ((err = nvs_flash_init())) {
    case ESP_OK:
      return 0;

    case ESP_ERR_NVS_NO_FREE_PAGES:
    case ESP_ERR_NVS_NEW_VERSION_FOUND:
      LOG_WARN("nvs_flash_init: %s", esp_err_to_name(err));
      return init_system_nvs_harder();

    default:
      LOG_ERROR("nvs_flash_init: %s", esp_err_to_name(err));
      return -1;
  }
}

static int init_system_network()
{
  esp_err_t err;

  LOG_INFO("init network stack...");

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

int init_system()
{
  int err;

  if ((err = init_system_nvs())) {
    LOG_ERROR("init_system_nvs");
    return err;
  }

  if ((err = init_system_network())) {
    LOG_ERROR("init_system_network");
    return err;
  }


  return 0;
}

void system_restart()
{
  LOG_INFO("restarting...");

  esp_restart();
}
