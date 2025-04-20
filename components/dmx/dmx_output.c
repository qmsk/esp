#include <dmx_output.h>
#include <dmx_output_stats.h>
#include "dmx_output.h"

#include <logging.h>

#include <stdlib.h>

#define DMX_BREAK_BITS 23 // 4us per bit, 92us min break
#define DMX_MARK_AFTER_BREAK_BITS 3 // 4us per bit, 12us MAB

int dmx_output_init (struct dmx_output *out, struct dmx_output_options options)
{
  out->options = options;
  
  stats_timer_init(&out->stats.uart_open);
  stats_timer_init(&out->stats.uart_tx);

  stats_counter_init(&out->stats.tx_error);

  stats_counter_init(&out->stats.cmd_dimmer);

  stats_gauge_init(&out->stats.data_len);

  return 0;
}

int dmx_output_new (struct dmx_output **outp, struct dmx_output_options options)
{
  struct dmx_output *out;
  int err;

  if (!(out = calloc(1, sizeof(*out)))) {
    LOG_ERROR("calloc");
    return -1;
  }

  if ((err = dmx_output_init(out, options))) {
    LOG_ERROR("dmx_output_init");
    goto error;
  }

  *outp = out;

  return 0;

error:
  free(out);

  return err;
}

void dmx_output_stats(struct dmx_output *out, struct dmx_output_stats *stats)
{
  stats->uart_open = stats_timer_copy(&out->stats.uart_open);
  stats->uart_tx = stats_timer_copy(&out->stats.uart_tx);

  stats->tx_error = stats_counter_copy(&out->stats.tx_error);
  stats->cmd_dimmer = stats_counter_copy(&out->stats.cmd_dimmer);

  stats->data_len = stats_gauge_copy(&out->stats.data_len);
}

int dmx_output_open (struct dmx_output *out, struct uart *uart)
{
  int err;

  if (out->uart) {
    LOG_ERROR("already opened");
    return -1;
  }

  WITH_STATS_TIMER(&out->stats.uart_open) {
    if ((err = uart_open_tx(uart)) < 0) {
      LOG_ERROR("uart_open_tx");
      return err;
    } else if (err) {
      LOG_DEBUG("uart_open_tx: not setup");
      return err;
    } else {
      LOG_DEBUG("uart=%p", uart);

      out->uart = uart;
    }
  }

  // enable output
  if (out->options.gpio_options) {
    gpio_out_set(out->options.gpio_options, out->options.gpio_out_pins);
  }

  return 0;
}

int dmx_output_write (struct dmx_output *out, enum dmx_cmd cmd, void *data, size_t len)
{
  int err;

  if (!out->uart) {
    LOG_ERROR("not open");
    return 1;
  }

  stats_counter_increment(&out->stats.cmd_dimmer);

  WITH_STATS_TIMER(&out->stats.uart_tx) {
    // send break/mark per spec minimums for transmit; actual timings will vary, these are minimums
    if ((err = uart_break(out->uart, DMX_BREAK_BITS, DMX_MARK_AFTER_BREAK_BITS))) {
      LOG_ERROR("uart1_break");
      goto error;
    }

    if ((err = uart_putc(out->uart, cmd)) < 0) {
      LOG_ERROR("uart_putc");
      goto error;
    }

    for (uint8_t *ptr = data; len > 0; ) {
      ssize_t write;

      if ((write = uart_write(out->uart, ptr, len)) < 0) {
        LOG_ERROR("uart_write");
        err = write;
        goto error;
      }

      ptr += write;
      len -= write;
    }

    if ((err = uart_flush_write(out->uart))) {
      LOG_ERROR("uart_flush_write");
      goto error;
    }
  }

  return 0;

error:
  stats_counter_increment(&out->stats.tx_error);

  return err;
}

int dmx_output_close (struct dmx_output *out)
{
  if (!out->uart) {
    LOG_ERROR("no opened uart");
    return 1;
  }

  // disable output
  if (out->options.gpio_options) {
    gpio_out_clear(out->options.gpio_options);
  }

  if (uart_close_tx(out->uart)) {
    LOG_WARN("uart_close_tx");
  }

  out->uart = NULL;

  return 0;
}
