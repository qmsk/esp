#pragma once

#include <soc/gpio_struct.h>

#define GPIO_HOST_PINS_LOW(x)  ((x) & 0xffffffff)
#define GPIO_HOST_PINS_HIGH(x) ((x) >> 32)

static inline gpio_pins_t gpio_host_get_pins(gpio_pins_t pins)
{
  return ((((gpio_pins_t) GPIO.in) & 0xffffffff) | (((gpio_pins_t) GPIO.in1.val) << 32)) & pins;
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

static inline gpio_pins_t gpio_host_intr_status(uint32_t core_id)
{
  if (core_id == 0) {
    return ((((gpio_pins_t) GPIO.pcpu_int) & 0xffffffff) | (((gpio_pins_t) GPIO.pcpu_int1.val) << 32));
  } else {
    return ((((gpio_pins_t) GPIO.acpu_int) & 0xffffffff) | (((gpio_pins_t) GPIO.acpu_int1.val) << 32));
  }
}

static inline void gpio_host_intr_clear(gpio_pins_t pins)
{
  GPIO.status_w1tc      = (uint32_t)(GPIO_HOST_PINS_LOW(pins));
  GPIO.status1_w1tc.val = (uint32_t)(GPIO_HOST_PINS_HIGH(pins));
}
