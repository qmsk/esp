#include <gpio_out.h>

#include <esp8266/gpio_struct.h>

static inline void gpio_pins_set(enum gpio_out_pins pins)
{
  GPIO.out_w1ts = pins;
}

static inline void gpio_pins_clear(enum gpio_out_pins pins)
{
  GPIO.out_w1tc = pins;
}

static inline void gpio_pins_out(enum gpio_out_pins pins, enum gpio_out_level level)
{
  if (level) {
    GPIO.out_w1ts = pins;
  } else {
    GPIO.out_w1tc = pins;
  }
}
