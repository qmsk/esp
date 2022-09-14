#pragma once

#include <soc/gpio_struct.h>

#define GPIO_INTR_PINS_LOW(x)  ((x) & 0xffffffff)
#define GPIO_INTR_PINS_HIGH(x) ((x) >> 32)

static inline gpio_pins_t gpio_intr_status(uint32_t core_id)
{
  if (core_id == 0) {
    return ((((gpio_pins_t) GPIO.pcpu_int) & 0xffffffff) | (((gpio_pins_t) GPIO.pcpu_int1.val) << 32));
  } else {
    return ((((gpio_pins_t) GPIO.acpu_int) & 0xffffffff) | (((gpio_pins_t) GPIO.acpu_int1.val) << 32));
  }
}

static inline void gpio_intr_clear(gpio_pins_t pins)
{
  GPIO.status_w1tc      = (uint32_t)(GPIO_INTR_PINS_LOW(pins));
  GPIO.status1_w1tc.val = (uint32_t)(GPIO_INTR_PINS_HIGH(pins));
}
