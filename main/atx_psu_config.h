#pragma once

#include <atx_psu.h>
#include <config.h>

enum atx_psu_gpio_mode {
  ATX_PSU_GPIO_MODE_DISABLED  = -1,
  ATX_PSU_GPIO_MODE_LOW       = 0,
  ATX_PSU_GPIO_MODE_HIGH      = 1,
};

struct atx_psu_config {
  bool enabled;

  int gpio_type;
  int power_enable_gpio_mode;
  int power_good_gpio_mode;

  uint16_t power_enable_gpio;
  uint16_t power_good_gpio;

  uint16_t timeout;
};

extern struct atx_psu_config atx_psu_config;
extern const struct config_enum atx_psu_gpio_mode_enum[];
