#include <gpio_out.h>

enum gpio_out_pins gpio_out_pin(int pin)
{
  if (pin >= 0 && pin < GPIO_OUT_PIN_COUNT) {
    return 1 << pin;
  } else {
    return 0;
  }
}
