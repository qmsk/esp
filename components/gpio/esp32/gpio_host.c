#include <gpio.h>
#include "../gpio.h"
#include "gpio_host.h"

#include <logging.h>

#include <esp_err.h>

static void gpio_host_setup_pin(gpio_pin_t gpio, bool inverted)
{
  // clear
  gpio_ll_set_level(&GPIO, gpio, inverted ? true : false);

  gpio_ll_input_disable(&GPIO, gpio);
  gpio_ll_od_disable(&GPIO, gpio);
  gpio_ll_output_enable(&GPIO, gpio);

  if (inverted) {
    gpio_ll_pulldown_dis(&GPIO, gpio);
    gpio_ll_pullup_en(&GPIO, gpio);
  } else {
    gpio_ll_pullup_dis(&GPIO, gpio);
    gpio_ll_pulldown_en(&GPIO, gpio);
  }

  gpio_ll_iomux_func_sel(GPIO_PIN_MUX_REG[gpio], PIN_FUNC_GPIO);
}

int gpio_host_setup(const struct gpio_options *options)
{
  for (gpio_pin_t gpio = 0; gpio < GPIO_HOST_PIN_COUNT; gpio++) {
    if (options->pins & gpio_host_pin(gpio)) {
      bool inverted = options->inverted & gpio_host_pin(gpio);

      gpio_host_setup_pin(gpio, inverted);
    }
  }

  return 0;
}

void gpio_host_clear(const struct gpio_options *options)
{
  gpio_host_out_pins(options->pins, GPIO_HOST_PINS_NONE ^ options->inverted);
}

void gpio_host_set(const struct gpio_options *options, gpio_pins_t pins)
{
  gpio_host_out_pins(options->pins, pins ^ options->inverted);
}

void gpio_host_set_all(const struct gpio_options *options)
{
  gpio_host_out_pins(options->pins, GPIO_HOST_PINS_ALL ^ options->inverted);
}
