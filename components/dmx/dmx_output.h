#pragma once

#include <dmx_output.h>

struct dmx_output {
  struct dmx_output_options options;

  struct uart *uart;
};
