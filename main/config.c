#include "config.h"

#include "activity_led.h"
#include "atx_psu.h"
#include "spi_leds.h"
#include "spiffs.h"
#include "wifi.h"

#include <config.h>
#include <errno.h>
#include <logging.h>

#define CONFIG_PARTITON_LABEL ("config")
#define CONFIG_BASE_PATH ("/config")
#define CONFIG_MAX_FILES 4

const struct configmod config_modules[] = {
  { "activity_led",
    .table = activity_led_configtab,
  },
  { "atx_psu",
    .table = atx_psu_configtab,
  },
  { "wifi",
    .table = wifi_configtab,
  },
  { "spi_leds",
    .table = spi_leds_configtab,
  },
  {}
};

struct config config = {
  .filename = "/config/boot.ini",

  .modules = config_modules,
};

int init_config()
{
  int err;

  if ((err = init_spiffs_partition(CONFIG_BASE_PATH, CONFIG_PARTITON_LABEL, CONFIG_MAX_FILES)) < 0) {
    LOG_ERROR("init_spiffs_partition");
  } else if (err) {
    LOG_WARN("No configuration partition available");
    return 0;
  }

  if (config_load(&config)) {
    if (errno == ENOENT) {
      LOG_WARN("No configuration available");
      return 0;
    } else {
      LOG_ERROR("Load config failed");
      return -1;
    }
  }

  LOG_INFO("Config loaded");

  return 0;
}

const struct cmdtab config_cmdtab = {
  .arg      = &config,
  .commands = config_commands,
};
