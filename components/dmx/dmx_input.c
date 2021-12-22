#include <dmx_input.h>
#include "dmx_input.h"

#include <logging.h>

#include <stdlib.h>

int dmx_input_init (struct dmx_input *in, struct dmx_input_options options)
{
  LOG_DEBUG("data=%p size=%u, frame_timeout=%u",
    options.data,
    options.size,
    options.frame_timeout
  );

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

int dmx_input_open (struct dmx_input *in, struct uart *uart)
{
  int err;

  LOG_DEBUG("dmx_input=%p uart=%p", in, uart);

  if ((err = uart_open_rx(uart))) {
    LOG_ERROR("uart_open_rx");
    return err;
  }

  in->uart = uart;
  in->stop = false;
  in->state = DMX_INPUT_STATE_BREAK;
  in->state_len = 0;

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
  in->state_len = 0;

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
    in->state_len++;
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
  int read = -1;

  if (in->stop) {
    return 0;
  }

  // read until data -> break
  while (!in->state_len || read) {
    // XXX: sync to start of break, but how to determine end of packet?!
    if ((read = uart_read(in->uart, buf, sizeof(buf))) < 0) {
      // lost sync
      in->state = DMX_INPUT_STATE_BREAK;

      LOG_ERROR("uart_read");
      return read;
    }

    // detect break
    if (read == 0) {
      dmx_input_process_break(in);
    } else {
      dmx_input_process(in, buf, read);
    }
  }

  return in->state_len;
}

int dmx_input_stop (struct dmx_input *in)
{
  int err;

  if (!in->uart) {
    return -1;
  }

  in->stop = true;

  if ((err = uart_abort_read(in->uart))) {
    LOG_ERROR("uart_abort_read");
    return err;
  }

  return 0;
}

int dmx_input_close (struct dmx_input *in)
{
  int err;

  LOG_DEBUG("dmx_input=%p uart=%p", in, in->uart);

  if ((err = uart_close_rx(in->uart))) {
    LOG_ERROR("uart_close_rx");
    return err;
  }

  in->uart = NULL;

  return 0;
}
