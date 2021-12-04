#pragma once

#include <dmx_output.h>

struct dmx_output {
  struct uart1 *uart1;

  /* Optional */
  struct gpio_out *gpio_out;
  enum gpio_out_pins gpio_out_pins;
};
