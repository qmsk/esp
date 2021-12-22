#pragma once

#include <dmx.h>

enum dmx_input_state {
  DMX_INPUT_STATE_BREAK = 0,
  DMX_INPUT_STATE_CMD,
  DMX_INPUT_STATE_DATA,
  DMX_INPUT_STATE_NOOP,
};

struct dmx_input {
  struct dmx_input_options options;

  struct uart *uart;
  bool stop;

  enum dmx_input_state state;
  enum dmx_cmd state_cmd;
  unsigned state_data_index;
  size_t state_len;
};
