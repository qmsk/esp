#include <gpio.h>
#include "../gpio.h"
#include "gpio_host.h"

#include <logging.h>

#include <esp_err.h>

static void gpio_host_setup_pin(gpio_pin_t gpio, bool input, bool output, bool inverted)
{
  // clear
  gpio_ll_set_level(&GPIO, gpio, inverted ? 1 : 0);

  if (input) {
    gpio_ll_input_enable(&GPIO, gpio);
  } else {
    gpio_ll_input_disable(&GPIO, gpio);
  }

  if (output) {
    gpio_ll_output_enable(&GPIO, gpio);
  } else {
    gpio_ll_output_disable(&GPIO, gpio);
  }

  gpio_ll_od_disable(&GPIO, gpio);

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
    bool input = options->in_pins & gpio_host_pin(gpio);
    bool output = options->out_pins & gpio_host_pin(gpio);
    bool inverted = options->inverted_pins & gpio_host_pin(gpio);

    if (input || output) {
      gpio_host_setup_pin(gpio, input, output, inverted);
    }
  }

  return 0;
}

int gpio_host_setup_input(const struct gpio_options *options, gpio_pins_t pins)
{
  gpio_host_setup_pins(options->in_pins, ~pins);

  return 0;
}

int gpio_host_get(const struct gpio_options *options, gpio_pins_t *pins)
{
  *pins = gpio_host_get_pins(options->in_pins) ^ (options->inverted_pins & options->in_pins);

  return 0;
}

int gpio_host_setup_output(const struct gpio_options *options, gpio_pins_t pins)
{
  gpio_host_setup_pins(options->out_pins, pins);

  return 0;
}

int gpio_host_clear(const struct gpio_options *options)
{
  gpio_host_set_pins(options->out_pins, GPIO_HOST_PINS_NONE ^ options->inverted_pins);

  return 0;
}

int gpio_host_set(const struct gpio_options *options, gpio_pins_t pins)
{
  gpio_host_set_pins(options->out_pins, pins ^ options->inverted_pins);

  return 0;
}

int gpio_host_set_all(const struct gpio_options *options)
{
  gpio_host_set_pins(options->out_pins, GPIO_HOST_PINS_ALL ^ options->inverted_pins);

  return 0;
}
