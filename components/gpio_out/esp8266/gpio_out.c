#include <gpio_out.h>
#include "gpio_pins.h"

#include <logging.h>

#include <driver/gpio.h>
#include <esp_err.h>
#include <esp8266/gpio_struct.h>

void gpio_out_clear(struct gpio_out *out)
{
  gpio_pins_out(out->pins, GPIO_OUT_PINS_NONE ^ out->inverted);
}

void gpio_out_set(struct gpio_out *out, enum gpio_out_pins pins)
{
  gpio_pins_out(out->pins, pins ^ out->inverted);
}

void gpio_out_all(struct gpio_out *out)
{
  gpio_pins_out(out->pins, GPIO_OUT_PINS_ALL ^ out->inverted);
}

int gpio_out_setup(struct gpio_out *out)
{
  gpio_config_t config = {
    .pin_bit_mask   = out->pins,
    .mode           = GPIO_MODE_OUTPUT,

    // XXX: should be per-pin
    .pull_up_en     = !!out->inverted,  // keep ESP8266 default pull-up enabled if active-low
    .pull_down_en   = !out->inverted,   // ESP8266 non-RTC GPIO pins only have internal pull-ups
  };
  esp_err_t err;

  gpio_out_clear(out);

  if (!out->pins) {
    // nothing to config
  } else if ((err = gpio_config(&config))) {
    LOG_ERROR("gpio_config: %s", esp_err_to_name(err));
    return -1;
  }

  return 0;
}
