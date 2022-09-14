#include <gpio.h>

IRAM_ATTR void gpio_host_intr_handler (const struct gpio_options *options, gpio_pins_t pins)
{
  if (options->interrupt_func) {
    options->interrupt_func(pins & options->interrupt_pins, options->interrupt_arg);
  }
}

gpio_pins_t gpio_host_pin(gpio_pin_t pin)
{
  if (pin >= 0 && pin < GPIO_HOST_PIN_COUNT) {
    // must cast to 64-bit wide literal before shifting
    return ((gpio_pins_t) 1) << pin;
  } else {
    return 0;
  }
}
