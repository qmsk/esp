#include <gpio.h>
#include "gpio.h"

#include <logging.h>

int gpio_setup(const struct gpio_options *options)
{
  switch (options->type) {
    case GPIO_TYPE_HOST:
      return gpio_host_setup(options);

  #if GPIO_I2C_ENABLED
    case GPIO_TYPE_I2C:
      return gpio_i2c_setup(options);
  #endif

    default:
      LOG_FATAL("unsupported type=%d", options->type);
  }
}

int gpio_in_setup(const struct gpio_options *options, gpio_pins_t pins)
{
  switch (options->type) {
    case GPIO_TYPE_HOST:
      return gpio_host_setup_input(options, pins);

  #if GPIO_I2C_ENABLED
    case GPIO_TYPE_I2C:
      return gpio_i2c_setup_input(options, pins);
  #endif

    default:
      LOG_FATAL("unsupported type=%d", options->type);
  }
}

int gpio_in_get(const struct gpio_options *options, gpio_pins_t *pins)
{
  switch (options->type) {
    case GPIO_TYPE_HOST:
      return gpio_host_get(options, pins);

  #if GPIO_I2C_ENABLED
    case GPIO_TYPE_I2C:
      return gpio_i2c_get(options, pins);
  #endif

    default:
      LOG_FATAL("unsupported type=%d", options->type);
  }
}

int gpio_out_setup(const struct gpio_options *options, gpio_pins_t pins)
{
  switch (options->type) {
    case GPIO_TYPE_HOST:
      return gpio_host_setup_output(options, pins);

  #if GPIO_I2C_ENABLED
    case GPIO_TYPE_I2C:
      return gpio_i2c_setup_output(options, pins);
  #endif

    default:
      LOG_FATAL("unsupported type=%d", options->type);
  }
}

int gpio_out_clear(const struct gpio_options *options)
{
  switch (options->type) {
    case GPIO_TYPE_HOST:
      return gpio_host_clear(options);

  #if GPIO_I2C_ENABLED
    case GPIO_TYPE_I2C:
      return gpio_i2c_set(options, 0);
  #endif

    default:
      LOG_FATAL("unsupported type=%d", options->type);
  }
}

int gpio_out_set(const struct gpio_options *options, gpio_pins_t pins)
{
  switch (options->type) {
    case GPIO_TYPE_HOST:
      return gpio_host_set(options, pins);

  #if GPIO_I2C_ENABLED
    case GPIO_TYPE_I2C:
      return gpio_i2c_set(options, pins);
  #endif

    default:
      LOG_FATAL("unsupported type=%d", options->type);
  }
}

int gpio_out_set_all(const struct gpio_options *options)
{
  switch (options->type) {
    case GPIO_TYPE_HOST:
      return gpio_host_set_all(options);

  #if GPIO_I2C_ENABLED
    case GPIO_TYPE_I2C:
      return gpio_i2c_set(options, GPIO_I2C_PINS_ALL);
  #endif

    default:
      LOG_FATAL("unsupported type=%d", options->type);
  }
}
