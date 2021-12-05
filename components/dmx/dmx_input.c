#include <dmx_input.h>
#include "dmx_input.h"

#include <logging.h>

#include <stdlib.h>

#define DMX_CHANNEL_COUNT 512

int dmx_input_init (struct dmx_input *in, struct dmx_input_options options)
{
  in->options = options;

  return 0;
}

int dmx_input_new (struct dmx_input **inp, struct dmx_input_options options)
{
  struct dmx_input *in;
  int err;

  LOG_DEBUG("options data=%p size=%u, frame_timeout=%u",
    options.data,
    options.size,
    options.frame_timeout
  );

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
  struct uart0_options dmx_uart_options = {
    .clock_div   = UART0_BAUD_250000,
    .data_bits   = UART0_DATA_BITS_8,
    .parity_bits = UART0_PARTIY_DISABLE,
    .stop_bits   = UART0_STOP_BITS_2,

    .rx_timeout = in->options.frame_timeout,

    .swap       = true, // use alternate RX/TX pins
  };
  int err;

  LOG_DEBUG("dmx_input=%p uart0=%p", in, uart);

  if ((err = uart0_open(uart, dmx_uart_options))) {
    LOG_ERROR("uart0_open");
    return err;
  }

  in->uart = uart;
  in->state = DMX_INPUT_STATE_BREAK;

  return 0;
}

static void dmx_input_process_break (struct dmx_input *in)
{
  LOG_DEBUG("break");

  in->state = DMX_INPUT_STATE_CMD;
}

static void dmx_input_process_cmd (struct dmx_input *in, enum dmx_cmd cmd)
{
  LOG_DEBUG("cmd=%#04x", cmd);

  in->state_cmd = cmd;

  switch(cmd) {
    case DMX_CMD_DIMMER:
      in->state = DMX_INPUT_STATE_DATA;
      in->state_data_index = 0;

      break;

    default:
      in->state = DMX_INPUT_STATE_NOOP;

      break;
  }
}

static void dmx_input_process_data (struct dmx_input *in, uint8_t data)
{
  unsigned index = in->state_data_index++;

  LOG_DEBUG("index=%u data=%#04x", index, data);

  if (in->options.address) {
    if (index + 1 < in->options.address) {
      return;
    } else {
      index = index + 1 - in->options.address;
    }
  }

  if (index < in->options.size) {
    in->options.data[index] = data;
  }
}

static void dmx_input_process (struct dmx_input *in, uint8_t *buf, size_t len)
{
  for (uint8_t *ptr = buf; ptr < buf + len; ptr++) {
    switch(in->state) {
      case DMX_INPUT_STATE_BREAK:
        /* Ignore, waiting for break to sync start of packet */
        break;

      case DMX_INPUT_STATE_CMD:
        dmx_input_process_cmd(in, (enum dmx_cmd) (*ptr));
        break;

      case DMX_INPUT_STATE_DATA:
        dmx_input_process_data(in, *ptr);
        break;

      case DMX_INPUT_STATE_NOOP:
        /* ignore */
        break;
    }
  }
}

int dmx_input_read (struct dmx_input *in)
{
  uint8_t buf[64];
  bool started = false;
  int read = 0;

  // read until break
  while (!started || read > 0) {
    // XXX: sync to start of break, but how to determine end of packet?!
    if ((read = uart0_read(in->uart, buf, sizeof(buf))) < 0) {
      // lost sync
      in->state = DMX_INPUT_STATE_BREAK;

      LOG_ERROR("uart0_read");
      return read;
    }

    // detect break
    if (read == 0) {
      dmx_input_process_break(in);
    } else {
      dmx_input_process(in, buf, read);
    }

    if (in->state) {
      started = true;
    }
  }

  return 0;
}
