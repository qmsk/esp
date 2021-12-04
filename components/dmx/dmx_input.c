#include <dmx_input.h>
#include "dmx_input.h"

#include <logging.h>

#include <stdlib.h>

int dmx_input_init (struct dmx_input *in, struct dmx_input_options options)
{
  in->options = options;

  return 0;
}

int dmx_input_new (struct dmx_input **inp, struct dmx_input_options options)
{
  struct dmx_input *in;
  int err;

  if (!(in = calloc(1, sizeof(*in)))) {
    LOG_ERROR("calloc");
    return -1;
  }

  if ((err = dmx_input_init(in, options))) {
    LOG_ERROR("dmx_input_init");
    goto error;
  }

  *inp = in;

  return 0;

error:
  free(in);

  return err;
}

int dmx_input_open (struct dmx_input *in, struct uart0 *uart)
{
  uint8_t buf[64];
  struct uart0_options dmx_uart_options = {
    .clock_div   = UART0_BAUD_250000,
    .data_bits   = UART0_DATA_BITS_8,
    .parity_bits = UART0_PARTIY_DISABLE,
    .stop_bits   = UART0_STOP_BITS_2,

    .rx_timeout = in->options.frame_timeout,
  };
  int ret;

  if ((ret = uart0_open(in->uart, dmx_uart_options))) {
    LOG_ERROR("uart0_open");
    return ret;
  }

  in->uart = uart;

  // XXX: discard data up to break?
  while ((ret = uart0_read(in->uart, buf, sizeof(buf))) > 0) {

  }

  if (ret < 0) {
    LOG_ERROR("uart0_read");
    return ret;
  }

  return 0;
}

int dmx_input_read (struct dmx_input *in, enum dmx_cmd *cmd, void *buf, size_t size)
{
  int err;

  // XXX: sync to start of break, but how to determine end of packet?!
  err = 0;

  return err;
}
