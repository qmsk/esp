#include <gpio_out.h>

#include <esp8266/gpio_struct.h>

static inline void gpio_pins_out(enum gpio_out_pins pins, enum gpio_out_pins levels)
{
  GPIO.out_w1ts = (pins & levels);
  GPIO.out_w1tc = (pins & ~levels);
}
