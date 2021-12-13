#pragma once

#include <dmx.h>
#include <gpio_out.h>
#include <uart.h>

struct dmx_output;
struct dmx_output_options {
  struct uart *uart;

  /* Optional */
  struct gpio_out *gpio_out;
  enum gpio_out_pins gpio_out_pins;
};

int dmx_output_new (struct dmx_output **outp, struct dmx_output_options options);

int dmx_output_cmd (struct dmx_output *out, enum dmx_cmd cmd, void *data, size_t len);
