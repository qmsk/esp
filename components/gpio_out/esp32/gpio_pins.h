#pragma once

#include <hal/gpio_types.h>
#include <hal/gpio_ll.h>
#include <soc/gpio_periph.h>

static void gpio_pins_init(gpio_num_t gpio, bool inverted)
{
  // clear
  gpio_ll_set_level(&GPIO, gpio, inverted ? true : false);

  gpio_ll_input_disable(&GPIO, gpio);
  gpio_ll_od_disable(&GPIO, gpio);
  gpio_ll_output_enable(&GPIO, gpio);

  if (inverted) {
    gpio_ll_pulldown_dis(&GPIO, gpio);
    gpio_ll_pullup_en(&GPIO, gpio);
  } else {
    gpio_ll_pullup_dis(&GPIO, gpio);
    gpio_ll_pulldown_en(&GPIO, gpio);
  }

  gpio_ll_iomux_func_sel(GPIO_PIN_MUX_REG[gpio], PIN_FUNC_GPIO);
}

static inline void gpio_pins_out(enum gpio_out_pins pins, enum gpio_out_pins levels)
{
  GPIO.out_w1ts = (pins & levels);
  GPIO.out_w1tc = (pins & ~levels);
}
