#pragma once

#include <hal/gpio_types.h>
#include <hal/gpio_ll.h>
#include <soc/gpio_periph.h>

#define GPIO_HOST_PINS_LOW(x)  ((x) & 0xffffffff)
#define GPIO_HOST_PINS_HIGH(x) ((x) >> 32)

static inline void gpio_host_out_pins(gpio_pins_t pins, gpio_pins_t levels)
{
  GPIO.out_w1ts       = (uint32_t)(GPIO_HOST_PINS_LOW(pins)  &  GPIO_HOST_PINS_LOW(levels));
  GPIO.out1_w1ts.val  = (uint32_t)(GPIO_HOST_PINS_HIGH(pins) &  GPIO_HOST_PINS_HIGH(levels));

  GPIO.out_w1tc       = (uint32_t)(GPIO_HOST_PINS_LOW(pins)  & ~GPIO_HOST_PINS_LOW(levels));
  GPIO.out1_w1tc.val  = (uint32_t)(GPIO_HOST_PINS_HIGH(pins) & ~GPIO_HOST_PINS_HIGH(levels));
}
