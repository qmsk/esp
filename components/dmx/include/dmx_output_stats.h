#pragma once

#include <dmx_output.h>
#include <stats.h>

struct dmx_output_stats {
  struct stats_timer uart_open;
  struct stats_timer uart_tx;

  struct stats_counter tx_error;
  struct stats_counter cmd_dimmer;

  struct stats_gauge data_len;
};

/*
* Get stats for output.
 */
void dmx_output_stats(struct dmx_output *out, struct dmx_output_stats *stats);
