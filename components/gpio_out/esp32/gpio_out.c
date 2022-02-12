#include <gpio_out.h>
#include "gpio_pins.h"

#include <logging.h>

#include <esp_err.h>

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

int gpio_out_init(struct gpio_out *gpio_out, enum gpio_out_pins pins, enum gpio_out_level level)
{
  gpio_out->pins = pins;
  gpio_out->level = level;

  for (gpio_num_t gpio = 0; gpio < GPIO_OUT_PIN_COUNT; gpio++) {
    if (pins & gpio_out_pin(gpio)) {
      gpio_pins_init(gpio, level);
    }
  }

  return 0;
}
