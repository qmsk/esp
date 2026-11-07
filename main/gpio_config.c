#include "gpio_config.h"

#include <logging.h>

#include <sdkconfig.h>

#define GPIO_TYPE_I2C_DEV(X) (GPIO_TYPE_I2C | ((X) << 8))

const struct config_enum gpio_type_enum[] = {
  { "HOST",       .value = GPIO_TYPE_HOST       },
#if CONFIG_I2C_GPIO_ENABLED
  { "I2C-GPIO0",  .value = GPIO_TYPE_I2C_DEV(0) },
  { "I2C-GPIO1",  .value = GPIO_TYPE_I2C_DEV(1) },
  { "I2C-GPIO2",  .value = GPIO_TYPE_I2C_DEV(2) },
  { "I2C-GPIO3",  .value = GPIO_TYPE_I2C_DEV(3) },
  { "I2C-GPIO4",  .value = GPIO_TYPE_I2C_DEV(4) },
#endif
  {},
};

int config_gpio_type(struct gpio_options *options, int type)
{
  switch(type) {
    case GPIO_TYPE_HOST:
      options->type = GPIO_TYPE_HOST;

      return 0;

  #if CONFIG_I2C_GPIO_ENABLED
    case GPIO_TYPE_I2C_DEV(0):
      options->type = GPIO_TYPE_I2C;

      if (!(options->i2c_dev = i2c_gpio_devs[0])) {
        LOG_ERROR("I2C-GPIO0 not initialized");
        return -1;
      }

      return 0;

    case GPIO_TYPE_I2C_DEV(1):
      options->type = GPIO_TYPE_I2C;
      if (!(options->i2c_dev = i2c_gpio_devs[1])) {
        LOG_ERROR("I2C-GPIO1 not initialized");
        return -1;
      }

      return 0;

    case GPIO_TYPE_I2C_DEV(2):
      options->type = GPIO_TYPE_I2C;
      if (!(options->i2c_dev = i2c_gpio_devs[2])) {
        LOG_ERROR("I2C-GPIO2 not initialized");
        return -1;
      }

      return 0;

    case GPIO_TYPE_I2C_DEV(3):
      options->type = GPIO_TYPE_I2C;
      if (!(options->i2c_dev = i2c_gpio_devs[3])) {
        LOG_ERROR("I2C-GPIO3 not initialized");
        return -1;
      }

      return 0;

    case GPIO_TYPE_I2C_DEV(4):
      options->type = GPIO_TYPE_I2C;
      if (!(options->i2c_dev = i2c_gpio_devs[4])) {
        LOG_ERROR("I2C-GPIO4 not initialized");
        return -1;
      }

      return 0;
  #endif

    default:
      LOG_ERROR("Invalid type=%x", type);
      return -1;
  }
}
