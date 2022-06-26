#pragma once

#include <hal/gpio_types.h>
#include <hal/gpio_ll.h>
#include <soc/gpio_periph.h>

#define GPIO_HOST_PINS_LOW(x)  ((x) & 0xffffffff)
#define GPIO_HOST_PINS_HIGH(x) ((x) >> 32)

static inline gpio_pins_t gpio_host_get_pins(gpio_pins_t pins)
{
  return (GPIO_HOST_PINS_LOW((gpio_pins_t) GPIO.in) | GPIO_HOST_PINS_HIGH((gpio_pins_t) GPIO.in1.val)) & pins;
}

static inline void gpio_host_setup_pins(gpio_pins_t pins, gpio_pins_t outputs)
{
  GPIO.enable_w1ts      = (uint32_t)(GPIO_HOST_PINS_LOW(pins)  &  GPIO_HOST_PINS_LOW(outputs));
  GPIO.enable1_w1ts.val = (uint32_t)(GPIO_HOST_PINS_HIGH(pins) &  GPIO_HOST_PINS_HIGH(outputs));

  GPIO.enable_w1tc      = (uint32_t)(GPIO_HOST_PINS_LOW(pins)  & ~GPIO_HOST_PINS_LOW(outputs));
  GPIO.enable1_w1tc.val = (uint32_t)(GPIO_HOST_PINS_HIGH(pins) & ~GPIO_HOST_PINS_HIGH(outputs));
}

static inline void gpio_host_set_pins(gpio_pins_t pins, gpio_pins_t levels)
{
  GPIO.out_w1ts       = (uint32_t)(GPIO_HOST_PINS_LOW(pins)  &  GPIO_HOST_PINS_LOW(levels));
  GPIO.out1_w1ts.val  = (uint32_t)(GPIO_HOST_PINS_HIGH(pins) &  GPIO_HOST_PINS_HIGH(levels));

  GPIO.out_w1tc       = (uint32_t)(GPIO_HOST_PINS_LOW(pins)  & ~GPIO_HOST_PINS_LOW(levels));
  GPIO.out1_w1tc.val  = (uint32_t)(GPIO_HOST_PINS_HIGH(pins) & ~GPIO_HOST_PINS_HIGH(levels));
}
