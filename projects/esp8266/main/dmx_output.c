#include <dmx_output.h>
#include "dmx.h"
#include "artnet.h"
#include "console.h"
#include "user_event.h"

#include <artnet.h>
#include <logging.h>
#include <gpio_out.h>
#include <uart.h>

#define DMX_ARTNET_TASK_NAME_FMT "dmx%d"
#define DMX_ARTNET_TASK_STACK 1024
#define DMX_ARTNET_TASK_PRIORITY (tskIDLE_PRIORITY + 2)

struct gpio_out dmx_gpio_out;
struct dmx_output_state dmx_output_states[DMX_OUTPUT_COUNT];

static int init_dmx_gpio()
{
  enum gpio_out_pins pins = 0;
  enum gpio_out_level level = 0;
  int err;

  for (int i = 0; i < DMX_OUTPUT_COUNT; i++) {
    const struct dmx_output_config *config = &dmx_output_configs[i];

    if (config->gpio_mode != -1) {
      level = config->gpio_mode; // XXX: correctly handle conflicting levels?
      pins |= gpio_out_pin(config->gpio_pin);
    }
  }

  LOG_INFO("pins=%04x level=%d", pins, level);

  if ((err = gpio_out_init(&dmx_gpio_out, pins, level))) {
    LOG_ERROR("gpio_out_init");
    return err;
  }

  return 0;
}

static int init_dmx_output(struct dmx_output_state *state, int index, const struct dmx_output_config *config)
{
  struct dmx_output_options options = {
    .uart           = dmx_uart,

    .gpio_out       = &dmx_gpio_out,
    .gpio_out_pins  = gpio_out_pin(config->gpio_pin),
  };
  int err;

  LOG_INFO("dmx%d: gpio_out_pins=%04x", index, options.gpio_out_pins);

  if ((err = dmx_output_new(&state->dmx_output, options))) {
    LOG_ERROR("dmx_output_new");
    return err;
  }

  return 0;
}

static int init_dmx_output_artnet(struct dmx_output_state *state, int index, const struct dmx_output_config *config)
{
  struct artnet_output_options options = {
    .port = (enum artnet_port) (index), // use dmx%d index as output port number
    .address = config->artnet_universe, // net/subnet set by add_artnet_output()
  };
  int err;

  LOG_INFO("dmx%d: artnet_universe=%u", index, config->artnet_universe);

  if (!(state->artnet_queue = xQueueCreate(1, sizeof(struct artnet_dmx)))) {
    LOG_ERROR("xQueueCreate");
    return -1;
  }

  if ((err = add_artnet_output(options, state->artnet_queue))) {
    LOG_ERROR("add_artnet_output");
    return err;
  }

  return 0;
}

int init_dmx_outputs()
{
  int err;

  if ((err = init_dmx_gpio())) {
    LOG_ERROR("init_dmx_gpio");
    return err;
  }

  for (int i = 0; i < DMX_OUTPUT_COUNT; i++)
  {
    const struct dmx_output_config *config = &dmx_output_configs[i];
    struct dmx_output_state *state = &dmx_output_states[i];

    if (config->enabled) {
      if ((err = init_dmx_output(state, i, config))) {
        LOG_ERROR("init_dmx_output");
        return err;
      }
    }

    if (config->enabled && config->artnet_enabled) {
      if ((err = init_dmx_output_artnet(state, i, config))) {
        LOG_ERROR("init_dmx_output_artnet");
        return err;
      }
    }
  }

  return 0;
}

int check_dmx_output_interface()
{
  const struct dmx_config *config = &dmx_config;

  switch (config->uart) {
    case UART_0:
      if (is_console_running()) {
        LOG_WARN("UART0 busy, console running on UART0");
        return 1;
      }
      return 0;

    default:
      return 0;
  }
}

int output_dmx(struct dmx_output_state *state, void *data, size_t len)
{
  int err;

  if (!state->dmx_output) {
    LOG_WARN("disabled");
    return -1;
  }

  if ((err = check_dmx_output_interface())) {
    return err;
  }

  for (int i = 0; i < len; i++) {
    LOG_DEBUG("[%03d] %02x", i, ((uint8_t *) data)[i]);
  }

  user_activity(USER_ACTIVITY_DMX_OUTPUT);

  if ((err = dmx_output_cmd(state->dmx_output, DMX_CMD_DIMMER, data, len)) < 0) {
    LOG_ERROR("dmx_output_cmd");
    return err;
  } else if (err) {
    LOG_WARN("dmx_output_cmd: UART not setup, DMX not running");
    return 1;
  }

  return 0;
}

static int dmx_output_artnet_dmx(struct dmx_output_state *state, struct artnet_dmx *dmx)
{
  LOG_DEBUG("len=%u", dmx->len);

  return output_dmx(state, dmx->data, dmx->len);
}

static void dmx_output_main(void *ctx)
{
  struct dmx_output_state *state = ctx;

  for (;;) {
    if (!xQueueReceive(state->artnet_queue, &state->artnet_dmx, portMAX_DELAY)) {
      LOG_WARN("xQueueReceive");
    } else if (dmx_output_artnet_dmx(state, &state->artnet_dmx) < 0) {
      LOG_WARN("dmx_output_artnet_dmx");
    }
  }
}

int start_dmx_output(struct dmx_output_state *state, int index)
{
  char task_name[configMAX_TASK_NAME_LEN];

  snprintf(task_name, sizeof(task_name), DMX_ARTNET_TASK_NAME_FMT, index);

  if (xTaskCreate(&dmx_output_main, task_name, DMX_ARTNET_TASK_STACK, state, DMX_ARTNET_TASK_PRIORITY, &state->task) <= 0) {
    LOG_ERROR("xTaskCreate");
    return -1;
  } else {
    LOG_DEBUG("task=%p", state->task);
  }

  return 0;
}

int start_dmx_outputs()
{
  int err;

  for (int i = 0; i < DMX_OUTPUT_COUNT; i++)
  {
    const struct dmx_output_config *config = &dmx_output_configs[i];
    struct dmx_output_state *state = &dmx_output_states[i];

    if (!config->enabled || !state->dmx_output) {
      continue;
    }
    if (!config->artnet_enabled || !state->artnet_queue) {
      // only used for Art-NET output
      continue;
    }

    if ((err = start_dmx_output(state, i))) {
      LOG_ERROR("start_dmx_output %d", i);
      return err;
    }
  }

  return 0;
}
