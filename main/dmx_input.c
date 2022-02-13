#include "dmx.h"
#include "dmx_config.h"
#include "dmx_state.h"

#include <artnet.h>
#include <dmx_input.h>
#include <logging.h>

struct dmx_input_state dmx_input_state;

int init_dmx_input()
{
  const struct dmx_input_config *config = &dmx_input_config;
  struct dmx_input_state *state = &dmx_input_state;
  int err;

  if (!config->enabled) {
    LOG_INFO("dmx-input: disabled");
    return 0;
  }

  // artnet
  if (!(state->artnet_dmx = calloc(1, sizeof(*state->artnet_dmx)))) {
    LOG_ERROR("calloc: artnet_dmx");
    return -1;
  }

  // dmx input
  struct dmx_input_options options = {
    .data          = state->artnet_dmx->data,
    .size          = sizeof(state->artnet_dmx->data),
  };

  LOG_INFO("dmx-input: enabled data=%p size=%u", options.data, options.size);

  if ((err = dmx_input_new(&state->dmx_input, options))) {
    LOG_ERROR("dmx_input_new");
    return err;
  }

  return 0;
}
