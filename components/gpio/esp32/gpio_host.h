#pragma once

#include <soc/gpio_struct.h>

#define GPIO_HOST_PINS_LOW(x)  ((x) & 0xffffffff)
#define GPIO_HOST_PINS_HIGH(x) ((x) >> 32)

static inline gpio_pins_t gpio_host_get_pins(gpio_pins_t pins)
{
  return ((((gpio_pins_t) GPIO.in) & 0xffffffff) | (((gpio_pins_t) GPIO.in1.val) << 32)) & pins;
}

// NOTE: using esp_rom_gpio_connect_out_signal() will also set GPIO_ENABLE_REG!
static inline void gpio_host_setup_signal(gpio_pin_t pin)
{
  GPIO.func_out_sel_cfg[pin].func_sel = SIG_GPIO_OUT_IDX;
  GPIO.func_out_sel_cfg[pin].oen_sel = 0;
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
