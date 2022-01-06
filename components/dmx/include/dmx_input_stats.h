#pragma once

#include <dmx_input.h>
#include <stats.h>

struct dmx_input_stats {
  struct stats_counter rx_overflow, rx_error, rx_break, rx_desync;
  struct stats_counter cmd_dimmer, cmd_unknown;

  struct stats_gauge data_len;
};

/*
* Get stats for input
 */
void dmx_input_stats(struct dmx_input *in, struct dmx_input_stats *stats);
