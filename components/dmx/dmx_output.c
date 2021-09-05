#include <dmx.h>
#include "dmx_output.h"

#include <logging.h>

#include <stdlib.h>

static const struct uart1_options dmx_uart1_options = {
  .clock_div   = UART1_BAUD_250000,
  .data_bits   = UART1_DATA_BITS_8,
  .parity_bits = UART1_PARTIY_DISABLE,
  .stop_bits   = UART1_STOP_BITS_2,
};

#define DMX_BREAK_US 92
#define DMX_MARK_US 12

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

int dmx_output_cmd (struct dmx_output *out, enum dmx_cmd cmd, void *data, size_t len)
{
  int err;

  if ((err = uart1_open(out->uart1, dmx_uart1_options))) {
    LOG_ERROR("uart1_open");
    return err;
  }

  // enable output
  if (out->gpio_out) {
    gpio_out_set(out->gpio_out, out->gpio_out_pins);
  }

  // send break/mark per spec minimums for transmit; actual timings will vary, these are minimums
  if ((err = uart1_break(out->uart1, DMX_BREAK_US, DMX_MARK_US))) {
    LOG_ERROR("uart1_break");
    goto error;
  }

  if ((err = uart1_putc(out->uart1, cmd)) < 0) {
    LOG_ERROR("uart1_putc");
    goto error;
  }

  for (uint8_t *ptr = data; len > 0; ) {
    ssize_t write;

    if ((write = uart1_write(out->uart1, ptr, len)) < 0) {
      LOG_ERROR("uart1_write");
      goto error;
    }

    ptr += write;
    len -= write;
  }

error:
  if (uart1_close(out->uart1)) {
    LOG_WARN("uart1_close");
  }

  return err;
}
