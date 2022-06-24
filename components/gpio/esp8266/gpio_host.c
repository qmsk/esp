#include <gpio.h>
#include "../gpio.h"
#include "gpio_host.h"

#include <logging.h>

#include <driver/gpio.h>
#include <esp_err.h>
#include <esp8266/gpio_struct.h>

int gpio_host_setup(const struct gpio_options *options)
{
  gpio_config_t config = {
    .pin_bit_mask   = options->pins,
    .mode           = GPIO_MODE_OUTPUT,

    // XXX: should be per-pin
    .pull_up_en     = !!options->inverted,  // keep ESP8266 default pull-up enabled if active-low
    .pull_down_en   = !options->inverted,   // ESP8266 non-RTC GPIO pins only have internal pull-ups
  };
  esp_err_t err;

  gpio_host_out_pins(options->pins, GPIO_HOST_PINS_NONE ^ options->inverted);

  if (!options->pins) {
    // nothing to config
  } else if ((err = gpio_config(&config))) {
    LOG_ERROR("gpio_config: %s", esp_err_to_name(err));
    return -1;
  }

  return 0;
}

int gpio_host_clear(const struct gpio_options *options)
{
  gpio_host_out_pins(options->pins, GPIO_HOST_PINS_NONE ^ options->inverted);

  return 0;
}

int gpio_host_set(const struct gpio_options *options, gpio_pins_t pins)
{
  gpio_host_out_pins(options->pins, pins ^ options->inverted);

  return 0;
}

int gpio_host_set_all(const struct gpio_options *options)
{
  gpio_host_out_pins(options->pins, GPIO_HOST_PINS_ALL ^ options->inverted);

  return 0;
}
