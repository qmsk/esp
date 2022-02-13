#include <gpio_out.h>

#include <esp8266/gpio_struct.h>

static inline void gpio_pins_out(enum gpio_out_pins pins, enum gpio_out_pins levels)
{
  GPIO.out_w1ts = (pins & levels);
  GPIO.out_w1tc = (pins & ~levels);
}

static inline void gpio_pins_set(enum gpio_out_pins pins, enum gpio_out_pins level)
{
  GPIO.out_w1ts = pins;
}

static inline void gpio_pins_clear(enum gpio_out_pins pins, enum gpio_out_pins level)
{
  GPIO.out_w1tc = pins;
}
