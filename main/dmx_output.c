#include <dmx_output.h>
#include "dmx.h"
#include "user_event.h"

#include <artnet.h>

#include <logging.h>
#include <uart.h>

// fit one complete DMX frame into the uart1 TX buffer
#define DMX_UART UART_1
#define DMX_UART_RX_BUFFER_SIZE 0
#define DMX_UART_TX_BUFFER_SIZE (512 + 1)

struct uart *dmx_uart;
struct gpio_out dmx_gpio_out;
struct dmx_output_state dmx_output_states[DMX_OUTPUT_COUNT];

static int dmx_init_uart()
{
  int err;

  if ((err = uart_new(&dmx_uart, DMX_UART, DMX_UART_RX_BUFFER_SIZE, DMX_UART_TX_BUFFER_SIZE))) {
    LOG_ERROR("uart_new");
    return err;
  }

  return 0;
}

static int dmx_init_gpio(const struct dmx_output_config *configs)
{
  int err;
  enum gpio_out_pins pins = 0;
  enum gpio_out_level level = 0;

  for (int i = 0; i < DMX_OUTPUT_COUNT; i++) {
    const struct dmx_output_config *config = &configs[i];

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

static int dmx_init_output(struct dmx_output_state *state, int index, const struct dmx_output_config *config)
{
  struct dmx_output_options options = {
    .uart           = dmx_uart,

    .gpio_out       = &dmx_gpio_out,
    .gpio_out_pins  = gpio_out_pin(config->gpio_pin),
  };
  int err;

  LOG_INFO("%d: gpio_out_pins=%04x", index, options.gpio_out_pins);

  if ((err = dmx_output_new(&state->dmx_output, options))) {
    LOG_ERROR("dmx_output_new");
    return err;
  }

  return 0;
}

static bool dmx_outputs_enabled()
{
  for (int i = 0; i < DMX_OUTPUT_COUNT; i++)
  {
    const struct dmx_output_config *config = &dmx_output_configs[i];

    if (config->enabled) {
      return true;
    }
  }

  return false;
}

int init_dmx_outputs()
{
  int err;

  if (!dmx_outputs_enabled()) {
    LOG_INFO("disabled");
    return 0;
  }

  if ((err = dmx_init_uart())) {
    LOG_ERROR("dmx_init_uart");
    return err;
  }

  if ((err = dmx_init_gpio(dmx_output_configs))) {
    LOG_ERROR("dmx_init_gpio");
    return err;
  }

  for (int i = 0; i < DMX_OUTPUT_COUNT; i++)
  {
    const struct dmx_output_config *config = &dmx_output_configs[i];
    struct dmx_output_state *state = &dmx_output_states[i];

    if (config->enabled) {
      if ((err = dmx_init_output(state, i, config))) {
        LOG_ERROR("dmx_init_output");
        return err;
      }
    }

    if (config->enabled && config->artnet_enabled) {
      if ((err = init_dmx_artnet_output(state, i, config))) {
        LOG_ERROR("init_dmx_artnet_output");
        return err;
      }
    }
  }

  return 0;
}

int output_dmx(struct dmx_output_state *state, void *data, size_t len)
{
  int err;

  if (!state->dmx_output) {
    LOG_WARN("disabled");
    return -1;
  }

  for (int i = 0; i < len; i++) {
    LOG_DEBUG("[%03d] %02x", i, ((uint8_t *) data)[i]);
  }

  user_activity(USER_ACTIVITY_DMX_OUTPUT);

  if ((err = dmx_output_cmd(state->dmx_output, DMX_CMD_DIMMER, data, len))) {
    LOG_ERROR("dmx_output_cmd");
    return err;
  }

  return 0;
}
