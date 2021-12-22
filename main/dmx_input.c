#include <dmx_input.h>
#include "dmx.h"

#include "artnet.h"
#include "user_event.h"

#include <logging.h>
#include <uart.h>

struct dmx_input_state *dmx_input_state;

static int init_dmx_input(struct dmx_input_state *state, const struct dmx_config *config)
{
  struct dmx_input_options options = {
    .data          = state->artnet_dmx.data,
    .size          = sizeof(state->artnet_dmx.data),
  };
  int err;

  if ((err = dmx_input_new(&state->dmx_input, options))) {
    LOG_ERROR("dmx_input_new");
    return err;
  }

  return 0;
}

static int init_dmx_input_artnet(struct dmx_input_state *state, const struct dmx_config *config)
{
  struct artnet_input_options artnet_options = {
    .port         = ARTNET_PORT_1,
    .index        = 0,

    .address      = config->input_artnet_universe,
  };
  int err;

  LOG_INFO("port=%u universe=%u", artnet_options.port, artnet_options.address);

  if ((err = add_artnet_input(&state->artnet_input, artnet_options))) {
    LOG_ERROR("add_artnet_input");
    return err;
  }

  return 0;
}

int init_dmx_inputs()
{
  const struct dmx_config *config = &dmx_config;
  int err;

  if (!(dmx_input_state = calloc(1, sizeof(*dmx_input_state)))) {
    LOG_ERROR("calloc");
    return -1;
  }

  if ((err = init_dmx_input(dmx_input_state, config))) {
    LOG_ERROR("init_dmx_input");
    return err;
  }

  if (config->input_artnet_enabled) {
    if ((err = init_dmx_input_artnet(dmx_input_state, config))) {
      LOG_ERROR("init_dmx_input_artnet");
      return err;
    }
  }

  return 0;
}

int dmx_input_main(struct dmx_input_state *state)
{
  int read;
  int err;

  if ((err = dmx_input_open(state->dmx_input, dmx_uart))) {
    LOG_ERROR("dmx_input_setup");
    return -1;
  }

  for (;;) {
    if ((read = dmx_input_read(state->dmx_input)) < 0) {
      LOG_ERROR("dmx_input_read");
      continue;
    } else {
      LOG_DEBUG("dmx_input_read: len=%d", read);

      state->artnet_dmx.len = read;
    }

    user_activity(USER_ACTIVITY_DMX_INPUT);

    if (state->artnet_input) {
      artnet_input_dmx(state->artnet_input, &state->artnet_dmx);
    }
  }

  if ((err = dmx_input_close(state->dmx_input))) {
    LOG_ERROR("dmx_input_close");
    return -1;
  }
}
