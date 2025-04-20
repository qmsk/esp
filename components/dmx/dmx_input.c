#include <dmx_input.h>
#include <dmx_input_stats.h>
#include "dmx_input.h"

#include <logging.h>

#include <errno.h>
#include <stdlib.h>

static void dmx_input_stats_init(struct dmx_input_stats *stats)
{
  stats_timer_init(&stats->uart_open);
  stats_timer_init(&stats->uart_rx);

  stats_counter_init(&stats->rx_overflow);
  stats_counter_init(&stats->rx_error);
  stats_counter_init(&stats->rx_break);
  stats_counter_init(&stats->rx_desync);
  stats_counter_init(&stats->cmd_dimmer);
  stats_counter_init(&stats->cmd_unknown);

  stats_gauge_init(&stats->data_len);
}

int dmx_input_init (struct dmx_input *in, struct dmx_input_options options)
{
  LOG_DEBUG("data=%p size=%u",
    options.data,
    options.size
  );

  in->options = options;

  dmx_input_stats_init(&in->stats);

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

void dmx_input_stats(struct dmx_input *in, struct dmx_input_stats *stats, bool reset)
{
  stats->uart_open = stats_timer_copy(&in->stats.uart_open);
  stats->uart_rx = stats_timer_copy(&in->stats.uart_rx);

  stats->rx_overflow = stats_counter_copy(&in->stats.rx_overflow);
  stats->rx_error = stats_counter_copy(&in->stats.rx_error);
  stats->rx_break = stats_counter_copy(&in->stats.rx_break);
  stats->rx_desync = stats_counter_copy(&in->stats.rx_desync);
  stats->cmd_dimmer = stats_counter_copy(&in->stats.cmd_dimmer);
  stats->cmd_unknown = stats_counter_copy(&in->stats.cmd_unknown);

  stats->data_len = stats_gauge_copy(&in->stats.data_len);

  if (reset) {
    dmx_input_stats_init(&in->stats);
  }
}

int dmx_input_open (struct dmx_input *in, struct uart *uart)
{
  int err;

  LOG_DEBUG("dmx_input=%p uart=%p", in, uart);

  WITH_STATS_TIMER(&in->stats.uart_open) {
    if ((err = uart_open_rx(uart))) {
      LOG_ERROR("uart_open_rx");
      return err;
    }
  }

  in->uart = uart;
  in->stop = false;
  in->state = DMX_INPUT_STATE_BREAK;
  in->state_len = 0;

  // enable input
  if (in->options.gpio_options) {
    gpio_out_set(in->options.gpio_options, in->options.gpio_out_pins);
  }

  return 0;
}

static void dmx_input_process_error (struct dmx_input *in, int err)
{
  LOG_DEBUG("error=%d", err);

  switch(err) {
    case EOVERFLOW:
      if (stats_counter_zero(&in->stats.rx_overflow)) {
        LOG_WARN("UART RX overflow");
      }

      stats_counter_increment(&in->stats.rx_overflow);

      break;

    case EBADMSG:
      if (stats_counter_zero(&in->stats.rx_error)) {
        LOG_WARN("UART RX error");
      }

      stats_counter_increment(&in->stats.rx_error);

      break;

    case ESPIPE:
      if (stats_counter_zero(&in->stats.rx_desync)) {
        LOG_WARN("UART RX break desynchronized");
      }

      stats_counter_increment(&in->stats.rx_desync);

      break;

    case EINTR:
      LOG_DEBUG("UART RX abort");

      break;
  }

  in->state = DMX_INPUT_STATE_BREAK;
  in->state_len = 0;
}

static void dmx_input_process_break (struct dmx_input *in)
{
  LOG_DEBUG("break");

  stats_counter_increment(&in->stats.rx_break);

  in->state = DMX_INPUT_STATE_CMD;
}

static void dmx_input_process_cmd (struct dmx_input *in, enum dmx_cmd cmd)
{
  LOG_DEBUG("cmd=%#04x", cmd);

  in->state_cmd = cmd;
  in->state_len = 0;

  switch(cmd) {
    case DMX_CMD_DIMMER:
      stats_counter_increment(&in->stats.cmd_dimmer);

      in->state = DMX_INPUT_STATE_DATA;
      in->state_data_index = 0;

      break;

    default:
      stats_counter_increment(&in->stats.cmd_unknown);

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

  if (!in->uart) {
    LOG_ERROR("no open uart");
    return -1;
  }

  if (in->stop) {
    return 0;
  }

  // read until data -> break
  while (!in->state_len || read) {
    WITH_STATS_TIMER(&in->stats.uart_rx) {
      if ((read = uart_read(in->uart, buf, sizeof(buf))) < 0) {
        dmx_input_process_error(in, -read);
        return read;
      }
    }

    // detect break
    if (read == 0) {
      dmx_input_process_break(in);
    } else {
      dmx_input_process(in, buf, read);
    }
  }

  // received complete packet
  stats_gauge_sample(&in->stats.data_len, in->state_len);

  return in->state_len;
}

int dmx_input_stop (struct dmx_input *in)
{
  int err;

  if (!in->uart) {
    LOG_ERROR("no open uart");
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

  if (!in->uart) {
    LOG_ERROR("no open uart");
    return -1;
  }

  LOG_DEBUG("dmx_input=%p uart=%p", in, in->uart);

  // disable input
  if (in->options.gpio_options) {
    gpio_out_clear(in->options.gpio_options);
  }

  if ((err = uart_close_rx(in->uart))) {
    LOG_ERROR("uart_close_rx");
    return err;
  }

  in->uart = NULL;

  return 0;
}
