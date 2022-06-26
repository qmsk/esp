#include <gpio.h>
#include "gpio.h"

#include <logging.h>

int gpio_setup(const struct gpio_options *options)
{
  switch (options->type) {
    case GPIO_TYPE_HOST:
      return gpio_host_setup(options);

    case GPIO_TYPE_I2C_PCA9534:
    case GPIO_TYPE_I2C_PCA9554:
      return gpio_i2c_pc54xx_setup(options);

    default:
      LOG_FATAL("unsupported type=%d", options->type);
  }
}

int gpio_in_setup(const struct gpio_options *options, gpio_pins_t pins)
{
  switch (options->type) {
    case GPIO_TYPE_HOST:
      return gpio_host_setup_input(options, pins);

    case GPIO_TYPE_I2C_PCA9534:
    case GPIO_TYPE_I2C_PCA9554:
      return gpio_i2c_pc54xx_setup_input(options, pins);

    default:
      LOG_FATAL("unsupported type=%d", options->type);
  }
}

int gpio_in_get(const struct gpio_options *options, gpio_pins_t *pins)
{
  switch (options->type) {
    case GPIO_TYPE_HOST:
      return gpio_host_get(options, pins);

    case GPIO_TYPE_I2C_PCA9534:
    case GPIO_TYPE_I2C_PCA9554:
      return gpio_i2c_pc54xx_get(options, pins);

    default:
      LOG_FATAL("unsupported type=%d", options->type);
  }
}

int gpio_out_setup(const struct gpio_options *options, gpio_pins_t pins)
{
  switch (options->type) {
    case GPIO_TYPE_HOST:
      return gpio_host_setup_output(options, pins);

    case GPIO_TYPE_I2C_PCA9534:
    case GPIO_TYPE_I2C_PCA9554:
      return gpio_i2c_pc54xx_setup_output(options, pins);

    default:
      LOG_FATAL("unsupported type=%d", options->type);
  }
}

int gpio_out_clear(const struct gpio_options *options)
{
  switch (options->type) {
    case GPIO_TYPE_HOST:
      return gpio_host_clear(options);

    case GPIO_TYPE_I2C_PCA9534:
    case GPIO_TYPE_I2C_PCA9554:
      return gpio_i2c_pc54xx_clear(options);

    default:
      LOG_FATAL("unsupported type=%d", options->type);
  }
}

int gpio_out_set(const struct gpio_options *options, gpio_pins_t pins)
{
  switch (options->type) {
    case GPIO_TYPE_HOST:
      return gpio_host_set(options, pins);

    case GPIO_TYPE_I2C_PCA9534:
    case GPIO_TYPE_I2C_PCA9554:
      return gpio_i2c_pc54xx_set(options, pins);

    default:
      LOG_FATAL("unsupported type=%d", options->type);
  }
}

int gpio_out_set_all(const struct gpio_options *options)
{
  switch (options->type) {
    case GPIO_TYPE_HOST:
      return gpio_host_set_all(options);

    case GPIO_TYPE_I2C_PCA9534:
    case GPIO_TYPE_I2C_PCA9554:
      return gpio_i2c_pc54xx_set_all(options);

    default:
      LOG_FATAL("unsupported type=%d", options->type);
  }
}
