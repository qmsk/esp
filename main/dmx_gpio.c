#include "dmx.h"
#include "dmx_config.h"
#include "dmx_state.h"
#include "gpio_type.h"
#include "i2c_gpio.h"

#include <gpio.h>
#include <logging.h>

struct gpio_options dmx_input_gpio_options = {};
struct gpio_options dmx_output_gpio_options = {};

int init_dmx_input_gpio()
{
  const struct dmx_input_config *config = &dmx_input_config;
  int err;

  if (config->gpio_mode == DMX_GPIO_MODE_DISABLED) {
    return 0;
  }

  if ((err = set_gpio_type(&dmx_input_gpio_options, config->gpio_type))) {
    LOG_ERROR("dmx-input: invalid gpio_type=%d", config->gpio_type);
    return err;
  }

  LOG_INFO("dmx-input: gpio type=%s mode=%s pin=%d",
    config_enum_to_string(gpio_type_enum, config->gpio_type) ?: "?",
    config_enum_to_string(dmx_gpio_mode_enum, config->gpio_mode) ?: "?",
    config->gpio_pin
  );

  dmx_input_gpio_options.out_pins |= gpio_host_pin(config->gpio_pin);

  if (config->gpio_mode == DMX_GPIO_MODE_LOW) {
    dmx_input_gpio_options.inverted_pins |= gpio_host_pin(config->gpio_pin);
  }

  const struct gpio_i2c_options *i2c_options;

  switch(dmx_input_gpio_options.type) {
    case GPIO_TYPE_HOST:
      LOG_INFO("dmx-input: gpio host: out_pins=" GPIO_PINS_FMT " inverted_pins=" GPIO_PINS_FMT,
        GPIO_PINS_ARGS(dmx_input_gpio_options.out_pins),
        GPIO_PINS_ARGS(dmx_input_gpio_options.inverted_pins)
      );
      break;

    case GPIO_TYPE_I2C:
      i2c_options = gpio_i2c_options(dmx_input_gpio_options.i2c_dev);

      LOG_INFO("dmx-input: gpio i2c type=%s port=%d addr=%u: out_pins=" GPIO_PINS_FMT " inverted_pins=" GPIO_PINS_FMT,
        config_enum_to_string(i2c_gpio_type_enum, i2c_options->type) ?: "?",
        i2c_options->port,
        i2c_options->addr,
        GPIO_PINS_ARGS(dmx_input_gpio_options.out_pins),
        GPIO_PINS_ARGS(dmx_input_gpio_options.inverted_pins)
      );
      break;
  }

  if ((err = gpio_setup(&dmx_input_gpio_options))) {
    LOG_ERROR("gpio_setup");
    return err;
  }

  return 0;

}

int init_dmx_output_gpio()
{
  bool enabled = false;
  int err;

  // outputs
  for (int i = 0; i < DMX_OUTPUT_COUNT; i++) {
    const struct dmx_output_config *config = &dmx_output_configs[i];

    if (config->gpio_mode == DMX_GPIO_MODE_DISABLED) {
      continue;
    } else {
      enabled = true;
    }

    if ((err = set_gpio_type(&dmx_output_gpio_options, config->gpio_type))) {
      LOG_ERROR("dmx-output%d: invalid gpio_type=%d", i + 1, config->gpio_type);
      return err;
    }

    for (unsigned j = 0; j < config->gpio_count; j++) {
      LOG_INFO("dmx-output%d: gpio type=%s mode=%s pin[%u]=%d", i + 1,
        config_enum_to_string(gpio_type_enum, config->gpio_type) ?: "?",
        config_enum_to_string(dmx_gpio_mode_enum, config->gpio_mode) ?: "?",
        j, config->gpio_pins[j]
      );

      dmx_output_gpio_options.out_pins |= gpio_host_pin(config->gpio_pins[j]);

      if (config->gpio_mode == DMX_GPIO_MODE_LOW) {
        dmx_output_gpio_options.inverted_pins |= gpio_host_pin(config->gpio_pins[j]);
      }
    }
  }

  if (!enabled) {
    LOG_INFO("dmx-output: gpio not configured");
    return 0;
  }

  const struct gpio_i2c_options *i2c_options;

  switch(dmx_output_gpio_options.type) {
    case GPIO_TYPE_HOST:
      LOG_INFO("dmx-output: gpio host: out_pins=" GPIO_PINS_FMT " inverted_pins=" GPIO_PINS_FMT,
        GPIO_PINS_ARGS(dmx_output_gpio_options.out_pins),
        GPIO_PINS_ARGS(dmx_output_gpio_options.inverted_pins)
      );
      break;

    case GPIO_TYPE_I2C:
      i2c_options = gpio_i2c_options(dmx_output_gpio_options.i2c_dev);

      LOG_INFO("dmx-output: gpio i2c type=%s port=%d addr=%u: out_pins=" GPIO_PINS_FMT " inverted_pins=" GPIO_PINS_FMT,
        config_enum_to_string(i2c_gpio_type_enum, i2c_options->type) ?: "?",
        i2c_options->port,
        i2c_options->addr,
        GPIO_PINS_ARGS(dmx_output_gpio_options.out_pins),
        GPIO_PINS_ARGS(dmx_output_gpio_options.inverted_pins)
      );
      break;
  }

  if ((err = gpio_setup(&dmx_output_gpio_options))) {
    LOG_ERROR("gpio_setup");
    return err;
  }

  return 0;
}

int config_dmx_input_gpio(struct dmx_input_state *state, const struct dmx_input_config *config, struct dmx_input_options *options)
{
  if (config->gpio_mode == DMX_GPIO_MODE_DISABLED) {
    return 0;
  }

  options->gpio_options = &dmx_input_gpio_options;
  options->gpio_out_pins = gpio_host_pin(config->gpio_pin);

  LOG_INFO("dmx-input: gpio type=%s mode=%s pins=" GPIO_PINS_FMT,
      config_enum_to_string(gpio_type_enum, config->gpio_type) ?: "?",
      config_enum_to_string(dmx_gpio_mode_enum, config->gpio_mode) ?: "?",
      GPIO_PINS_ARGS(options->gpio_out_pins)
  );

  return 0;
}

int config_dmx_output_gpio(struct dmx_output_state *state, const struct dmx_output_config *config, struct dmx_output_options *options)
{
  if (config->gpio_mode == DMX_GPIO_MODE_DISABLED) {
    return 0;
  }

  options->gpio_options = &dmx_output_gpio_options;
  options->gpio_out_pins = 0;

  for (unsigned i = 0; i < config->gpio_count; i++) {
    options->gpio_out_pins |= gpio_host_pin(config->gpio_pins[i]);
  }

  LOG_INFO("dmx-output%d: gpio type=%s mode=%s pins=" GPIO_PINS_FMT, state->index + 1,
      config_enum_to_string(gpio_type_enum, config->gpio_type) ?: "?",
      config_enum_to_string(dmx_gpio_mode_enum, config->gpio_mode) ?: "?",
      GPIO_PINS_ARGS(options->gpio_out_pins)
  );

  return 0;
}
