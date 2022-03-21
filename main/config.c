#include "config.h"
#include "artnet.h"
#include "atx_psu.h"
#include "console.h"
#include "dmx.h"
#include "eth.h"
#include "http.h"
#include "leds.h"
#include "wifi.h"

#include <config.h>
#include <config_cmd.h>
#include <logging.h>

#include <esp_err.h>
#include <esp_spiffs.h>

#include <errno.h>
#include <sdkconfig.h>

#define CONFIG_BASE_PATH "/config"
#define CONFIG_FILE_NAME "boot.ini"
#define CONFIG_PARTITON_LABEL "config"
#define CONFIG_MAX_FILES 4

const struct configmod config_modules[] = {
  { "console",
    .description = (
      "UART Console"
    ),
    .table = console_configtab,
  },
  { "wifi",
    .description = (
      "WiFi AP/STA mode, using static/dynamic IPv4 addressing."
    ),
    .table = wifi_configtab,
  },
#if CONFIG_ETH_ENABLED
  { "eth",
    .description = (
      "10/100BASE-TX Ethernet, using static/dynamic IPv4 addressing."
    ),
    .table = eth_configtab,
  },
#endif
  { "http",
    .description = "HTTP API + Web frontend with optional HTTP basic authentication.",
    .table = http_configtab,
  },
  { "atx-psu",
    .description = (
      "Control ATX-PSU based on leds output."
      "\n"
      "The ATX-PSU will be powered on when led outputs are active,"
      " and powered off into standby mode if all led outputs are idle (zero-valued)."
      "\n"
      "Assumes an active-high NPN transistor with POWER_EN -> collector, gpio -> 1k ohm -> base, emitter -> GND."
    ),
    .table = atx_psu_configtab,
  },
  { "artnet",
    .table = artnet_configtab,
  },
  { "dmx-uart",
    .description = "DMX UART interface",
    .table = dmx_uart_configtab,
  },
  { "dmx-input",
    .description = "DMX input",
    .table = dmx_input_configtab,
  },
  { "dmx-output",
    .description = "DMX output",
    .tables = dmx_output_configtabs,
    .tables_count = DMX_OUTPUT_COUNT,
  },
  { "leds-spi",
    .description = "LEDS SPI interface",
    .table = leds_spi_configtab,
  },
  { "leds-uart",
    .description = "LEDS UART interface",
    .table = leds_uart_configtab,
  },
  { "leds-i2s",
    .description = "LEDS I2S interface",
    .table = leds_i2s_configtab,
  },
  { "leds",
    .description = (
      "Control LEDs using synchronous (separate clock/data) serial protocols via Art-Net."
      "\n"
      "Multiple serial outputs can be multiplexed from the same SPI driver by using GPIOs to control"
      " an external driver chip with active-high/low output-enable GPIO lines."
    ),
    .tables = leds_configtabs,
    .tables_count = LEDS_COUNT,
    .alias = "spi-leds",
  },
  {}
};

static bool config_disabled = false;
struct config config = {
  .filename = (CONFIG_BASE_PATH "/" CONFIG_FILE_NAME),

  .modules = config_modules,
};

void disable_config()
{
  LOG_WARN("Disabling config loading");
  config_disabled = true;
}

int init_config()
{
  esp_vfs_spiffs_conf_t conf = {
    .base_path              = CONFIG_BASE_PATH,
    .partition_label        = CONFIG_PARTITON_LABEL,
    .max_files              = CONFIG_MAX_FILES,
    .format_if_mount_failed = true,
  };
  esp_err_t err;

  if ((err = config_init(&config))) {
    LOG_ERROR("config_init");
    return err;
  }

  LOG_INFO("mount/format partition=%s at base_path=%s with max_files=%u", conf.partition_label, conf.base_path, conf.max_files);

  if ((err = esp_vfs_spiffs_register(&conf))) {
    if (err == ESP_ERR_NOT_FOUND) {
      LOG_WARN("config partition with label=%s not found", conf.partition_label);
    } else {
      LOG_ERROR("esp_vfs_spiffs_register: %s", esp_err_to_name(err));
    }
  }

  if (config_disabled) {
    LOG_WARN("Configuration loading disabled");
    return 1;
  }

  if (config_load(&config)) {
    if (errno == ENOENT) {
      LOG_WARN("spiffs file at %s not found", config.filename);
      return 1;
    } else {
      LOG_ERROR("config_load(%s)", config.filename);
      return -1;
    }
  }

  LOG_INFO("loaded config");

  return 0;
}

void reset_config()
{
  int err;

  // attempt to remove the config boot.ini file
  if ((err = config_reset(&config)) < 0) {
    LOG_ERROR("config_reset");
  } else if (err > 0) {
    LOG_INFO("no config file: %s", config.filename);
    return;
  } else {
    LOG_WARN("removed config file: %s", config.filename);
    return; // success
  }

  // if spiffs is corrupted enough, format entire partition
  if ((err = esp_spiffs_format(CONFIG_PARTITON_LABEL))) {
    LOG_ERROR("esp_spiffs_format: %s", esp_err_to_name(err));
  } else {
    LOG_WARN("formatted config filesystem with partition label=%s", CONFIG_PARTITON_LABEL);
    return; // success
  }

  LOG_WARN("unable to reset configuration!");
}
