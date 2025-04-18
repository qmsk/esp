#include "dmx.h"
#include "dmx_config.h"
#include "dmx_state.h"
#include "artnet_state.h"
#include "user.h"

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

  // artnet_dmx used as dmx_input buffer
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

  if ((err = config_dmx_input_gpio(state, config, &options))) {
    LOG_ERROR("dmx-input: config_dmx_input_gpio");
    return err;
  }

  if ((err = dmx_input_new(&state->dmx_input, options))) {
    LOG_ERROR("dmx_input_new");
    return err;
  }

  // artnet
  if (config->artnet_enabled) {
    struct artnet_input_options artnet_options = {
      .address      = config->artnet_universe,
    };
    int err;

    if ((err = add_artnet_input(&state->artnet_input, artnet_options))) {
      LOG_ERROR("add_artnet_input");
      return err;
    }
  }

  return 0;
}

int run_dmx_input(struct dmx_input_state *state)
{
  int read;
  int err;

  LOG_INFO("open uart...");

  if ((err = open_dmx_input_uart(state->dmx_input))) {
    LOG_ERROR("dmx-input: open_dmx_input_uart");
    return -1;
  }

  LOG_INFO("start read loop");

  for (;;) {
    if ((read = dmx_input_read(state->dmx_input)) < 0) {
      LOG_DEBUG("dmx_input_read");
      continue;
    } else if (read) {
      LOG_DEBUG("dmx_input_read: len=%d", read);

      state->artnet_dmx->len = read;
    } else {
      LOG_INFO("dmx_input_read: stopped");
      break;
    }

    user_activity(USER_ACTIVITY_DMX_INPUT);

    if (state->artnet_input) {
      artnet_input_dmx(state->artnet_input, state->artnet_dmx);
    }
  }

  if ((err = dmx_input_close(state->dmx_input))) {
    LOG_ERROR("dmx_input_close");
    return -1;
  }

  return 0;
}

int stop_dmx_input(struct dmx_input_state *state)
{
  int err;

  LOG_DEBUG("dmx_input=%p", state->dmx_input);

  // stops the running run_dmx_input loop
  if ((err = dmx_input_stop(state->dmx_input))) {
    LOG_ERROR("dmx_input_stop");
    return err;
  }

  return 0;
}
