#include "leds.h"
#include "leds_config.h"
#include "leds_state.h"

#include <logging.h>

#if CONFIG_LEDS_I2S_ENABLED
  // config
  struct leds_i2s_config leds_i2s_config = {

  };

  const struct config_enum leds_i2s_port_enum[] = {
    { "",              -1           },
  # if defined(I2S_PORT_0)
    { "I2S0",         I2S_PORT_0    },
  # endif
  # if defined(I2S_PORT_1)
    { "I2S1",         I2S_PORT_1    },
  # endif
    {},
  };

  const struct configtab leds_i2s_configtab[] = {
    { CONFIG_TYPE_ENUM, "port",
      .description = "Select host peripherial for I2S interface.",
      .enum_type = { .value = &leds_i2s_config.port, .values = leds_i2s_port_enum, .default_value = -1 },
    },
    {},
  };

  // state
  struct i2s_out *leds_i2s_out;

  int init_leds_i2s()
  {
    const struct leds_i2s_config *i2s_config = &leds_i2s_config;
    size_t buffer_size = 0;
    bool enabled = false;
    int err;

    if (i2s_config->port < 0) {
      LOG_INFO("leds: i2s disabled");
      return 0;
    }

    for (int i = 0; i < LEDS_COUNT; i++)
    {
      const struct leds_config *config = &leds_configs[i];

      if (!config->enabled) {
        continue;
      }

      if (config->interface != LEDS_INTERFACE_I2S) {
        continue;
      }

      enabled = true;

      // update maximum trasnfer size
      size_t size = leds_i2s_buffer_for_protocol(config->protocol, config->count);

      LOG_INFO("leds%d: i2s%d configured, data buffer size=%u", i + 1, i2s_config->port, size);

      if (size > buffer_size) {
        buffer_size = size;
      }
    }

    if (!enabled) {
      LOG_INFO("leds: i2s%d not configured", i2s_config->port);
      return 0;
    }

    LOG_INFO("leds: i2s port=%d -> buffer_size=%u", i2s_config->port,
      buffer_size
    );

    if ((err = i2s_out_new(&leds_i2s_out, i2s_config->port, buffer_size))) {
      LOG_ERROR("i2s_out_new(port=%d)", i2s_config->port);
      return err;
    }

    return 0;
  }

  int config_leds_i2s(struct leds_state *state, const struct leds_config *config, struct leds_options *options)
  {
    const struct leds_i2s_config *i2s_config = &leds_i2s_config;

    if (!leds_i2s_out) {
      LOG_ERROR("leds%d: i2s out not initialized", state->index + 1);
      return -1;
    }

    options->i2s_out = leds_i2s_out;
  #if LEDS_I2S_GPIO_PIN_ENABLED
    options->i2s_gpio_pin = config->i2s_gpio_pin;
  #endif

    LOG_INFO("leds%d: i2s port=%d: gpio_pin=%d", state->index + 1,
      i2s_config->port,
    #if LEDS_I2S_GPIO_PIN_ENABLED
      options->i2s_gpio_pin
    #else
      -1
    #endif
    );

    return 0;
  }
#endif