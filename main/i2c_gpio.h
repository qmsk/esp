#pragma once

#include "i2c.h"

#include <sdkconfig.h>

#include <gpio.h>

#if GPIO_I2C_ENABLED
  extern const struct config_enum i2c_gpio_type_enum[];
#endif

#if CONFIG_I2C_GPIO_ENABLED
  #include <config.h>

  extern const struct gpio_i2c_options i2c_gpio_options0;

  extern struct gpio_i2c_dev *i2c_gpio_devs[I2C_GPIO_COUNT + 1];

  #define I2C_GPIO_DEV(x) i2c_gpio_devs[(x)]
#endif
