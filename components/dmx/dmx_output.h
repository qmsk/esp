#pragma once

#include <dmx.h>

struct dmx_output {
  struct uart1 *uart1;
  struct uart1_options uart1_options;

  /* Optional */
  struct gpio_out *gpio_out;
  enum gpio_out_pins gpio_out_pins;
};
