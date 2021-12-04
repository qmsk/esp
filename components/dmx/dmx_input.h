#pragma once

#include <dmx.h>

struct dmx_input {
  struct dmx_input_options options;
  
  struct uart0 *uart;
};
