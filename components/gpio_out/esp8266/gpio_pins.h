#include <gpio_out.h>

#include <esp8266/gpio_struct.h>

static inline void gpio_pins_out(uint32_t pins, uint32_t levels)
{
  GPIO.out_w1ts = (pins & levels);
  GPIO.out_w1tc = (pins & ~levels);
}
