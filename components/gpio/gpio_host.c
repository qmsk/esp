#include <gpio.h>

gpio_pins_t gpio_host_pin(gpio_pin_t pin)
{
  if (pin >= 0 && pin < GPIO_HOST_PIN_COUNT) {
    // must cast to 64-bit wide literal before shifting
    return ((gpio_pins_t) 1) << pin;
  } else {
    return 0;
  }
}
