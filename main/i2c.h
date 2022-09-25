#pragma once

#include <config.h>

#include <sdkconfig.h>

#if CONFIG_I2C_MASTER_ENABLED
  int init_i2c_master();
#endif

#if CONFIG_I2C_GPIO_ENABLED
  #define I2C_GPIO_COUNT 4

  /* Setup i2c-gpio0 used by user-leds via sdkconfig */
  int init_i2c_gpio();

  /* Setup additional i2c-gpio devices used via config by other components */
  int start_i2c_gpio();

  extern const struct configtab *i2c_gpio_configtabs[I2C_GPIO_COUNT];
#endif
