#pragma once

#include <dmx.h>
#include <uart.h>

struct dmx_input;
struct dmx_input_options {
  /* Write channel data */
  uint8_t *data;
  size_t size;

  /* Offset start of data to 1-indexed channel address*/
  unsigned address;
};

int dmx_input_new (struct dmx_input **inp, struct dmx_input_options options);

/*
 * Open UART for RX, and update state to sync to next BREAK.
 *
 * UART must already be setup.
 */
int dmx_input_open (struct dmx_input *in, struct uart *uart);

/*
 * Read one DMX packet, processing up to options->size DMX channels into options->data, starting at DMX channel options->address.
 *
 * @return <0 on error, or number of DMX channels updated.
 */
int dmx_input_read (struct dmx_input *in);

/*
 * Close UART for RX.
 */
 int dmx_input_close (struct dmx_input *in);
