#include <gpio_out.h>

gpio_out_pins_t gpio_out_pin(gpio_out_pin_t pin)
{
  if (pin >= 0 && pin < GPIO_OUT_PIN_COUNT) {
    return 1 << pin;
  } else {
    return 0;
  }
}
