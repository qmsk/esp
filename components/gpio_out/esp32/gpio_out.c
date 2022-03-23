#include <gpio_out.h>
#include "gpio_pins.h"

#include <logging.h>

#include <esp_err.h>

void gpio_out_clear(struct gpio_out *out)
{
  gpio_pins_out(out->pins, GPIO_OUT_PINS_NONE ^ out->inverted);
}

void gpio_out_set(struct gpio_out *out, gpio_out_pins_t pins)
{
  gpio_pins_out(out->pins, pins ^ out->inverted);
}

void gpio_out_all(struct gpio_out *out)
{
  gpio_pins_out(out->pins, GPIO_OUT_PINS_ALL ^ out->inverted);
}

int gpio_out_setup(struct gpio_out *gpio_out)
{
  for (gpio_num_t gpio = 0; gpio < GPIO_OUT_PIN_COUNT; gpio++) {
    if (gpio_out->pins & gpio_out_pin(gpio)) {
      bool inverted = gpio_out->inverted & gpio_out_pin(gpio);
      gpio_pins_init(gpio, inverted);
    }
  }

  return 0;
}
