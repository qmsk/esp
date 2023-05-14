#include "config.h"
#include "config_vfs.h"
#include "artnet.h"
#include "atx_psu.h"
#include "console.h"
#include "dmx.h"
#include "eth.h"
#include "http.h"
#include "i2c.h"
#include "leds.h"
#include "wifi.h"

#include <config.h>
#include <config_cmd.h>
#include <logging.h>

#include <esp_err.h>

#include <errno.h>
#include <sdkconfig.h>

const struct configmod config_modules[] = {
  { "console",
    .description = (
      "UART Console"
    ),
    .table = console_configtab,
  },
#if CONFIG_I2C_GPIO_ENABLED
  { "i2c-gpio",
    .description = "I2C GPIO",
    .tables = i2c_gpio_configtabs,
    .tables_count = I2C_GPIO_COUNT,
  },
#endif
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
  { "leds-sequence",
    .description = "LEDS Sequence support",
    .table = leds_sequence_configtab,
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
  .path     = CONFIG_VFS_PATH,

  .modules = config_modules,
};

void disable_config()
{
  LOG_WARN("Disabling config loading");
  config_disabled = true;
}

int init_config()
{
  int err;

  if ((err = config_init(&config))) {
    LOG_ERROR("config_init");
    return err;
  }

  if ((err = init_config_vfs()) < 0) {
    LOG_ERROR("init_config_vfs");
    return err;
  } else if (err) {
    LOG_WARN("init_config_vfs");
    return 1;
  }

  if (config_disabled) {
    LOG_WARN("Configuration loading disabled");
    return 1;
  }

  if (config_load(&config, CONFIG_BOOT_FILE)) {
    if (errno == ENOENT) {
      LOG_WARN("spiffs %s file at %s not found", CONFIG_VFS_PATH, CONFIG_BOOT_FILE);
      return 1;
    } else {
      LOG_ERROR("config_load(%s)", CONFIG_BOOT_FILE);
      return -1;
    }
  }

  LOG_INFO("loaded boot config");

  return 0;
}

void reset_config()
{
  int err;

  // attempt to remove the config boot.ini file
  if ((err = config_delete(&config, CONFIG_BOOT_FILE)) < 0) {
    LOG_ERROR("config_delete");
  } else if (err > 0) {
    LOG_INFO("no config file: %s", CONFIG_BOOT_FILE);
    return;
  } else {
    LOG_WARN("removed config file: %s", CONFIG_BOOT_FILE);
    return; // success
  }

  // if spiffs is corrupted enough, format entire partition
  if (reset_config_vfs()) {
    LOG_ERROR("reset_config_vfs");
  } else {
    return; // success
  }

  LOG_WARN("unable to reset configuration!");
}
