#include <dmx_input.h>
#include "dmx.h"

#include "artnet.h"
#include "user_event.h"

#include <logging.h>
#include <uart0.h>
#include <gpio_out.h>

struct gpio_out dmx_input_gpio_debug;
struct uart0 *dmx_uart0;
struct dmx_input_state *dmx_input_state;

// fit one complete DMX frame into the uart1 RX buffer
#define DMX_RX_BUFFER_SIZE (512 + 1)

#define DMX_INPUT_FRAME_TIMEOUT 20

#define DMX_INPUT_TASK_STACK 2048
#define DMX_INPUT_TASK_PRIORITY (tskIDLE_PRIORITY + 2)

static void dmx_input_main(void *ctx)
{
  struct dmx_input_state *state = ctx;
  int read, err;

  if ((err = dmx_input_open(state->dmx_input, dmx_uart0))) {
    LOG_ERROR("dmx_input_open");
    return; // XXX: alert
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
}

int init_dmx_input_state(struct dmx_input_state *state, const struct dmx_input_config *config)
{
  struct dmx_input_options options = {
    .data          = state->artnet_dmx.data,
    .size          = sizeof(state->artnet_dmx.data),

    .frame_timeout = DMX_INPUT_FRAME_TIMEOUT,
  };
  struct artnet_input_options artnet_options = {
    .port         = ARTNET_PORT_1,
    .index        = 0,

    .address      = config->artnet_universe,
  };
  int err;

  if ((err = dmx_input_new(&state->dmx_input, options))) {
    LOG_ERROR("dmx_input_new");
    return err;
  }

  if (config->artnet_enabled) {
    LOG_INFO("add artnet input on port=%u", artnet_options.port);

    if ((err = add_artnet_input(&state->artnet_input, artnet_options))) {
      LOG_ERROR("add_artnet_input");
      return err;
    }
  }

  if (xTaskCreate(&dmx_input_main, "dmx-input", DMX_INPUT_TASK_STACK, state, DMX_INPUT_TASK_PRIORITY, &state->task) <= 0) {
    LOG_ERROR("xTaskCreate");
    return -1;
  } else {
    LOG_DEBUG("task=%p", state->task);
  }

  return 0;
}

int init_dmx_input()
{
  const struct dmx_input_config *config = &dmx_input_config;
  int err;

  if (!config->enabled) {
    LOG_INFO("disabled");
    return 0;
  }

  if (!(dmx_input_state = calloc(1, sizeof(*dmx_input_state)))) {
    LOG_ERROR("calloc");
    return -1;
  }

  if ((err = uart0_new(&dmx_uart0, DMX_RX_BUFFER_SIZE))) {
    LOG_ERROR("uart0_new");
    return err;
  }

  if ((err = init_dmx_input_state(dmx_input_state, config))) {
    LOG_ERROR("init_dmx_input_state");
    return err;
  }

  return 0;
}
