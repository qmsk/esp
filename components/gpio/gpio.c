#include <gpio.h>
#include "gpio.h"

#include <logging.h>

int gpio_init(const struct gpio_options *options)
{
  switch (options->type) {
    case GPIO_TYPE_HOST:
      return gpio_host_init(options);

    default:
      LOG_FATAL("unsupported type=%d", options->type);
  }
}

void gpio_out_clear(const struct gpio_options *options)
{
  switch (options->type) {
    case GPIO_TYPE_HOST:
      return gpio_host_clear(options);

    default:
      LOG_FATAL("unsupported type=%d", options->type);
  }
}

void gpio_out_set(const struct gpio_options *options, gpio_pins_t pins)
{
  switch (options->type) {
    case GPIO_TYPE_HOST:
      return gpio_host_set(options, pins);

    default:
      LOG_FATAL("unsupported type=%d", options->type);
  }
}

void gpio_out_set_all(const struct gpio_options *options)
{
  switch (options->type) {
    case GPIO_TYPE_HOST:
      return gpio_host_set_all(options);

    default:
      LOG_FATAL("unsupported type=%d", options->type);
  }
}
