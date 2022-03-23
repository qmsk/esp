#pragma once

#include <dmx.h>
#include <gpio_out.h>
#include <uart.h>

struct dmx_output;
struct dmx_output_options {
  /* Optional */
  struct gpio_out *gpio_out;
  gpio_out_pins_t gpio_out_pins;
};

int dmx_output_new (struct dmx_output **outp, struct dmx_output_options options);

/*
 * Open UART for TX.
 *
 * Sets gpio_out if configured.
 *
 * UART must already be setup.
 *
 * Returns <0 on error, 0 on success, >0 if UART not setup.
 */
int dmx_output_open (struct dmx_output *out, struct uart *uart);

/*
 * Write cmd + data on UART TX.
 *
 * DMX output must be open!
 *
 * Returns <0 on error, 0 on success, >0 if UART not open.
 */
int dmx_output_write (struct dmx_output *out, enum dmx_cmd cmd, void *data, size_t len);

/*
 * Close UART for TX.
 *
 * Clears gpio_out if configured.
 */
 int dmx_output_close (struct dmx_output *out);
