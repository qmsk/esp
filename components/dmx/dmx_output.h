#pragma once

#include <dmx_output.h>
#include <dmx_output_stats.h>

struct dmx_output {
  struct dmx_output_options options;
  struct dmx_output_stats stats;

  struct uart *uart;
};
