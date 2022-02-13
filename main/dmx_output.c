#include "dmx.h"
#include "dmx_config.h"
#include "dmx_state.h"

#include <artnet.h>
#include <dmx_output.h>
#include <logging.h>

struct dmx_output_state dmx_output_states[DMX_OUTPUT_COUNT];

int init_dmx_output(struct dmx_output_state *state, int index, const struct dmx_output_config *config)
{
  int err;

  state->index = index;

  if (!config->enabled) {
    LOG_INFO("dmx-output%d: disabled", index + 1);
    return 0;
  }

  // dmx output
  struct dmx_output_options options;

  LOG_INFO("dmx-output%d: enabled", index + 1);

  if ((err = config_dmx_output_gpio(state, config, &options))) {
    LOG_ERROR("dmx-output%d: config_dmx_output_gpio", index + 1);
    return err;
  }

  if ((err = dmx_output_new(&state->dmx_output, options))) {
    LOG_ERROR("dmx_output_new");
    return err;
  }

  // artnet
  if (!(state->artnet_dmx = calloc(1, sizeof(*state->artnet_dmx)))) {
    LOG_ERROR("calloc: artnet_dmx");
    return -1;
  }

  return 0;
}

int init_dmx_outputs()
{
  int err;

  for (int i = 0; i < DMX_OUTPUT_COUNT; i++) {
    const struct dmx_output_config *config = &dmx_output_configs[i];
    struct dmx_output_state *state = &dmx_output_states[i];

    if ((err = init_dmx_output(state, i, config))) {
      LOG_ERROR("dmx%d: init_dmx_output", i + 1);
      return err;
    }
  }

  return 0;
}
