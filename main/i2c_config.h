#pragma once

#include "i2c.h"

#include <sdkconfig.h>

#include <config.h>

#if CONFIG_I2C_GPIO_ENABLED
  #include <gpio.h>

  struct i2c_gpio_config {
    int type;
    uint16_t addr;
    uint16_t int_pin;
  };

  extern struct i2c_gpio_config i2c_gpio_config;
  extern struct i2c_gpio_config i2c_gpio_configs[I2C_GPIO_COUNT];
#endif
