#include "gpio.h"
#include "gpio_type.h"
#include "i2c_gpio.h"

#include <gpio.h>

#include <logging.h>

#define GPIO_TYPE_I2C_DEV(X) (GPIO_TYPE_I2C | ((X) << 8))


const struct config_enum gpio_type_enum[] = {
  { "HOST",       GPIO_TYPE_HOST              },
#if CONFIG_I2C_GPIO_ENABLED
  { "I2C-GPIO0",  GPIO_TYPE_I2C_DEV(0) },
  { "I2C-GPIO1",  GPIO_TYPE_I2C_DEV(1) },
  { "I2C-GPIO2",  GPIO_TYPE_I2C_DEV(2) },
  { "I2C-GPIO3",  GPIO_TYPE_I2C_DEV(3) },
  { "I2C-GPIO4",  GPIO_TYPE_I2C_DEV(4) },
#endif
  {},
};


int init_gpio()
{
  int err;

  LOG_INFO("init gpio interrupts");

  if ((err = gpio_intr_init())) {
    LOG_ERROR("gpio_intr_init");
    return err;
  }

  return 0;
}

int set_gpio_type(struct gpio_options *options, int type)
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
