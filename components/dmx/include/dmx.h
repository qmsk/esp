#ifndef DMX_H
#define DMX_H

#include <gpio_out.h>
#include <uart1.h>

enum dmx_cmd {
    DMX_CMD_DIMMER = 0x00,
};

struct dmx_output;
struct dmx_output_options {
  struct gpio_out *gpio_out;
  enum gpio_out_pins gpio_out_pins;
};

int dmx_output_new (struct dmx_output **outp, struct uart1 *uart1, struct dmx_output_options options);

int dmx_output_cmd (struct dmx_output *out, enum dmx_cmd cmd, void *data, size_t len);

#endif
