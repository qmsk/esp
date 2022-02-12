#include <gpio_out.h>
#include "gpio_pins.h"

#include <logging.h>

#include <driver/gpio.h>
#include <esp_err.h>
#include <esp8266/gpio_struct.h>

void gpio_out_clear(struct gpio_out *out)
{
  gpio_pins_out(out->pins, !out->level);
}

void gpio_out_set(struct gpio_out *out, enum gpio_out_pins pins)
{
  // first clear those not set
  if (out->level) {
    gpio_pins_clear((out->pins) & ~pins);
    gpio_pins_set(pins);
  } else {
    gpio_pins_set((out->pins) & ~pins);
    gpio_pins_clear(pins);
  }
}

void gpio_out_all(struct gpio_out *out)
{
  gpio_pins_out(out->pins, out->level);
}

int gpio_out_setup(struct gpio_out *out)
{
  gpio_config_t config = {
    .pin_bit_mask   = out->pins,
    .mode           = GPIO_MODE_OUTPUT,
    .pull_up_en     = (out->level == GPIO_OUT_LOW),  // keep ESP8266 default pull-up enabled if active-low
    .pull_down_en   = (out->level == GPIO_OUT_HIGH), // ESP8266 non-RTC GPIO pins only have internal pull-ups
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
