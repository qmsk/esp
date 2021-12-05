#pragma once

#include <dmx.h>

enum dmx_input_state {
  DMX_INPUT_STATE_BREAK = 0,
  DMX_INPUT_STATE_CMD,
  DMX_INPUT_STATE_DATA,
};

struct dmx_input {
  struct dmx_input_options options;

  struct uart0 *uart;

  enum dmx_input_state state;
  enum dmx_cmd state_cmd;
  unsigned state_index;
};
