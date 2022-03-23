#include "dmx.h"
#include "dmx_config.h"
#include "dmx_state.h"

#include <gpio_out.h>
#include <logging.h>

struct gpio_out dmx_gpio_out;

int init_dmx_gpio()
{
  bool enabled = false;
  int err;

  // outputs
  for (int i = 0; i < DMX_OUTPUT_COUNT; i++) {
    const struct dmx_output_config *config = &dmx_output_configs[i];

    if (!config->enabled) {
      continue;
    }

    switch(config->gpio_mode) {
      case DMX_GPIO_MODE_DISABLED:
        break;

      case DMX_GPIO_MODE_LOW:
        enabled = true;

        LOG_INFO("dmx-output%d: gpio mode=LOW pin=%d", i + 1, config->gpio_pin);

        dmx_gpio_out.pins |= gpio_out_pin(config->gpio_pin);
        dmx_gpio_out.inverted |= gpio_out_pin(config->gpio_pin);
        break;

      case DMX_GPIO_MODE_HIGH:
        enabled = true;

        LOG_INFO("dmx-output%d: gpio mode=HIGH pin=%d", i + 1, config->gpio_pin);

        dmx_gpio_out.pins |= gpio_out_pin(config->gpio_pin);
        break;

      default:
        LOG_ERROR("dmx-output%d: invalid gpio_mode=%d", i + 1, config->gpio_mode);
        return -1;
    }
  }

  if (!enabled) {
    LOG_INFO("dmx: gpio not configured");
    return 0;
  }

  LOG_INFO("dmx: gpio -> pins=" GPIO_OUT_PINS_FMT " inverted=" GPIO_OUT_PINS_FMT,
    GPIO_OUT_PINS_ARGS(dmx_gpio_out.pins),
    GPIO_OUT_PINS_ARGS(dmx_gpio_out.inverted)
  );

  if ((err = gpio_out_setup(&dmx_gpio_out))) {
    LOG_ERROR("gpio_out_setup");
    return err;
  }

  return 0;
}

int config_dmx_output_gpio(struct dmx_output_state *state, const struct dmx_output_config *config, struct dmx_output_options *options)
{
  LOG_INFO("dmx%d: gpio mode=%s pin=%d", state->index + 1,
      config_enum_to_string(dmx_gpio_mode_enum, config->gpio_mode),
      config->gpio_pin
  );

  options->gpio_out = &dmx_gpio_out;
  options->gpio_out_pins = gpio_out_pin(config->gpio_pin);

  return 0;
}
