#pragma once

#include <hal/gpio_types.h>
#include <hal/gpio_ll.h>
#include <soc/gpio_periph.h>

static void gpio_pins_init(gpio_num_t gpio, enum gpio_out_level level)
{
  gpio_ll_set_level(&GPIO, gpio, !level);

  gpio_ll_input_disable(&GPIO, gpio);
  gpio_ll_od_disable(&GPIO, gpio);
  gpio_ll_output_enable(&GPIO, gpio);

  if (level) {
    gpio_ll_pullup_dis(&GPIO, gpio);
    gpio_ll_pulldown_en(&GPIO, gpio);
  } else {
    gpio_ll_pulldown_dis(&GPIO, gpio);
    gpio_ll_pullup_en(&GPIO, gpio);
  }

  gpio_ll_iomux_func_sel(gpio, PIN_FUNC_GPIO);
}

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
