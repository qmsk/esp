#include "atx_psu_config.h"
#include "gpio_type.h"

#include <atx_psu.h>
#include <gpio.h>
#include <logging.h>

#define ATX_PSU_CONFIG_TIMEOUT_DEFAULT_VALUE 10

struct atx_psu_config atx_psu_config = {};

const struct config_enum atx_psu_gpio_mode_enum[] = {
  { "",     ATX_PSU_GPIO_MODE_DISABLED  },
  { "LOW",  ATX_PSU_GPIO_MODE_LOW       },
  { "HIGH", ATX_PSU_GPIO_MODE_HIGH      },
  {}
};

const struct configtab atx_psu_configtab[] = {
  { CONFIG_TYPE_BOOL, "enabled",
    .bool_type = { .value = &atx_psu_config.enabled },
  },
  { CONFIG_TYPE_ENUM, "gpio_type",
   .description = "Use built-in (HOST) GPIO pins, or external I2C GPIO expander via i2c-gpio config",
   .enum_type = { .value = &atx_psu_config.gpio_type, .values = gpio_type_enum, .default_value = GPIO_TYPE_HOST },
  },
  { CONFIG_TYPE_ENUM, "power_enable_gpio_mode",
   .description = "GPIO pin will be driven high/low to enable the ATX PS_ON output.",
   .enum_type = { .value = &atx_psu_config.power_enable_gpio_mode, .values = atx_psu_gpio_mode_enum, .default_value = ATX_PSU_GPIO_MODE_DISABLED },
  },
  { CONFIG_TYPE_ENUM, "power_good_gpio_mode",
   .description = "GPIO pin will be driven high/low by the ATX PWR_OK input.",
   .enum_type = { .value = &atx_psu_config.power_good_gpio_mode, .values = atx_psu_gpio_mode_enum, .default_value = ATX_PSU_GPIO_MODE_DISABLED },
  },
  { CONFIG_TYPE_UINT16, "power_enable_gpio",
    .alias = "gpio",
    .uint16_type = { .value = &atx_psu_config.power_enable_gpio, .max = GPIO_PIN_MAX },
  },
  { CONFIG_TYPE_UINT16, "power_good_gpio",
    .uint16_type = { .value = &atx_psu_config.power_good_gpio, .max = GPIO_PIN_MAX },
  },
  { CONFIG_TYPE_UINT16, "timeout",
    .description = "Power off ATX PSU after timeout seconds of idle",
    .uint16_type = { .value = &atx_psu_config.timeout, .default_value = ATX_PSU_CONFIG_TIMEOUT_DEFAULT_VALUE },
  },
  {}
};
