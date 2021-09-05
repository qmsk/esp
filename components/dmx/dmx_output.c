#include <dmx.h>
#include "dmx_output.h"

#include <logging.h>

#include <stdlib.h>

int dmx_output_new (struct dmx_output **outp, struct uart1 *uart1, struct dmx_output_options options)
{
  struct dmx_output *out;

  if (!(out = calloc(1, sizeof(*out)))) {
    LOG_ERROR("calloc");
    return -1;
  }

  out->uart1 = uart1;
  out->gpio_out = options.gpio_out;
  out->gpio_out_pins = options.gpio_out_pins;

  *outp = out;

  return 0;
}

int dmx_output_enable (struct dmx_output *out)
{
  LOG_DEBUG("gpio_out=%p gpio_out_pins=%04x", out->gpio_out, out->gpio_out_pins);

  if (out->gpio_out) {
    gpio_out_set(out->gpio_out, out->gpio_out_pins);
  }

  return 0;
}

int dmx_output_cmd (struct dmx_output *out, enum dmx_cmd cmd, void *data, size_t len)
{
  int err;

  // send break/mark per spec minimums for transmit; actual timings will vary, these are minimums
  if ((err = uart1_break(out->uart1, DMX_BREAK_US, DMX_MARK_US))) {
    LOG_ERROR("uart1_break");
    return err;
  }

  if ((err = uart1_putc(out->uart1, cmd)) < 0) {
    LOG_ERROR("uart1_putc");
    return err;
  }

  for (uint8_t *ptr = data; len > 0; ) {
    ssize_t write;

    if ((write = uart1_write(out->uart1, ptr, len)) < 0) {
      LOG_ERROR("uart1_write");
      return write;
    }

    ptr += write;
    len -= write;
  }

  return 0;
}
