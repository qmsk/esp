#include <gpio_out.h>
#include "gpio_pins.h"

enum gpio_out_pins gpio_out_pin(unsigned pin)
{
  if (pin < 16) {
    return 1 << pin;
  } else {
    return 0;
  }
}
