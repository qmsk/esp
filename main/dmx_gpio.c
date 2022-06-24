#include "dmx.h"
#include "dmx_config.h"
#include "dmx_state.h"

#include <gpio.h>
#include <logging.h>

struct gpio_options dmx_gpio_options = {
  .type   = GPIO_TYPE_HOST,
};

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

        dmx_gpio_options.pins |= gpio_host_pin(config->gpio_pin);
        dmx_gpio_options.inverted |= gpio_host_pin(config->gpio_pin);
        break;

      case DMX_GPIO_MODE_HIGH:
        enabled = true;

        LOG_INFO("dmx-output%d: gpio mode=HIGH pin=%d", i + 1, config->gpio_pin);

        dmx_gpio_options.pins |= gpio_host_pin(config->gpio_pin);
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

  LOG_INFO("dmx: gpio -> pins=" GPIO_PINS_FMT " inverted=" GPIO_PINS_FMT,
    GPIO_PINS_ARGS(dmx_gpio_options.pins),
    GPIO_PINS_ARGS(dmx_gpio_options.inverted)
  );

  if ((err = gpio_setup(&dmx_gpio_options))) {
    LOG_ERROR("gpio_setup");
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

  options->gpio_options = &dmx_gpio_options;
  options->gpio_out_pins = gpio_host_pin(config->gpio_pin);

  return 0;
}
