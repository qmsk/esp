#ifndef DMX_INPUT_H
#define DMX_INPUT_H

#include <dmx.h>
#include <uart0.h>

struct dmx_input;
struct dmx_input_options {
  /* Write channel data */
  uint8_t *data;
  size_t size;

  /* Offset start of data to 1-indexed channel address*/
  unsigned address;

  /* Detect end of short packet */
  unsigned frame_timeout; // in units of frames (44us)
};

int dmx_input_new (struct dmx_input **inp, struct dmx_input_options options);

int dmx_input_open (struct dmx_input *in, struct uart0 *uart);

int dmx_input_read (struct dmx_input *in);

#endif
