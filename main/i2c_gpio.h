#pragma once

#include "i2c.h"

#include <sdkconfig.h>

#if CONFIG_I2C_GPIO_ENABLED
  #include <gpio.h>

  extern const struct gpio_i2c_options i2c_gpio_options0;

  extern struct gpio_i2c_dev *i2c_gpio_devs[I2C_GPIO_COUNT + 1];
#endif
