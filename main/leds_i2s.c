#include "leds.h"
#include "leds_config.h"
#include "leds_state.h"
#include "console.h"
#include "pin_mutex.h"

#include <logging.h>

// do not block if pin in use, timeout immediately
#define LEDS_I2S_PIN_TIMEOUT portMAX_DELAY

#if CONFIG_LEDS_I2S_ENABLED

  // config
  struct leds_i2s_config leds_i2s_config = {

  };

  const struct config_enum leds_i2s_port_enum[] = {
    { "",             .value = -1           },
  #if defined(I2S_PORT_0)
    { "I2S0",         .value = I2S_PORT_0    },
  #endif
  #if defined(I2S_PORT_1)
    { "I2S1",         .value = I2S_PORT_1    },
  #endif
    {},
  };

  #if defined(I2S_PORT_1)
    #define LEDS_I2S_PORT_DEFAULT_VALUE I2S_PORT_1
  #elif defined(I2S_PORT_0)
    #define LEDS_I2S_PORT_DEFAULT_VALUE I2S_PORT_0
  #else
    #define LEDS_I2S_PORT_DEFAULT_VALUE -1
  #endif

  const struct configtab leds_i2s_configtab[] = {
    { CONFIG_TYPE_ENUM, "port",
      .description = "Select host peripherial for I2S interface.",
      .enum_type = { .value = &leds_i2s_config.port, .values = leds_i2s_port_enum, .default_value = LEDS_I2S_PORT_DEFAULT_VALUE },
    },
    {},
  };

  // state
  struct i2s_out *leds_i2s_out;

  int init_leds_i2s()
  {
    const struct leds_i2s_config *i2s_config = &leds_i2s_config;
    size_t buffer_size = 0, buffer_align = 0;
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

      // update maximum transfer size
      size_t size, align;

    #if LEDS_I2S_GPIO_PINS_ENABLED
      if (config->i2s_data_pin_count > 1 || config->i2s_data_inv_pin_count > 1) {
        // parallel output
        unsigned pin_count = config->i2s_data_pin_count > config->i2s_data_inv_pin_count ? config->i2s_data_pin_count : config->i2s_data_inv_pin_count;

        // parallel output
        size = leds_i2s_parallel_buffer_size(config->protocol, config->count, pin_count);
        align = leds_i2s_parallel_buffer_align(config->protocol, pin_count);

        LOG_INFO("leds%d: i2s%d configured for %u parallel leds on %u pins, data buffer size=%u align=%u", i + 1, i2s_config->port, config->count, pin_count, size, align);
      } else {
        size = leds_i2s_serial_buffer_size(config->protocol, config->count);
        align = leds_i2s_serial_buffer_align(config->protocol);

        LOG_INFO("leds%d: i2s%d configured for %u serial leds, data buffer size=%u align=%u", i + 1, i2s_config->port, config->count, size, align);
      }
    #else
      size = leds_i2s_serial_buffer_size(config->protocol, config->count);
      align = leds_i2s_serial_buffer_align(config->protocol);

      LOG_INFO("leds%d: i2s%d configured for %u serial leds, data buffer size=%u align=%u", i + 1, i2s_config->port, config->count, size, align);
    #endif

      if (size > buffer_size) {
        buffer_size = size;
      }
      if (align > buffer_align) {
        buffer_align = align;
      }
    }

    if (!enabled) {
      LOG_INFO("leds: i2s%d not configured", i2s_config->port);
      return 0;
    }

    LOG_INFO("leds: i2s port=%d -> buffer_size=%u buffer_align=%u", i2s_config->port,
      buffer_size, buffer_align
    );

    if ((err = i2s_out_new(&leds_i2s_out, i2s_config->port, buffer_size, buffer_align))) {
      LOG_ERROR("i2s_out_new(port=%d)", i2s_config->port);
      return err;
    }

    return 0;
  }

  int config_leds_i2s(struct leds_state *state, const struct leds_config *config, struct leds_interface_i2s_options *options)
  {
    const struct leds_i2s_config *i2s_config = &leds_i2s_config;

    if (!leds_i2s_out) {
      LOG_ERROR("leds%d: i2s out not initialized", state->index + 1);
      return -1;
    }

    options->i2s_out = leds_i2s_out;
  #if CONFIG_IDF_TARGET_ESP8266
    options->pin_mutex = pin_mutex[PIN_MUTEX_I2S0_DATA]; // shared with console uart0
    options->pin_timeout = LEDS_I2S_PIN_TIMEOUT;
  #elif LEDS_I2S_GPIO_PINS_ENABLED
    if (config->i2s_data_pin_count > 1 || config->i2s_data_inv_pin_count > 1) {
      // parallel output
      options->data_pins_count = config->i2s_data_pin_count >= config->i2s_data_inv_pin_count ? config->i2s_data_pin_count : config->i2s_data_inv_pin_count;

      for (int i = 0; i < config->i2s_data_pin_count || i < config->i2s_data_inv_pin_count; i++) {
          options->data_pins[i] = i < config->i2s_data_pin_count ? config->i2s_data_pins[i] : GPIO_NUM_NC;
          options->inv_data_pins[i] = i < config->i2s_data_inv_pin_count ? config->i2s_data_inv_pins[i] : GPIO_NUM_NC;
          options->clock_pins[i] = i < config->i2s_clock_pin_count ? config->i2s_clock_pins[i] : GPIO_NUM_NC;

          LOG_INFO("leds%d: parallel data_pins[%d]=%d inv_data_pins[%d]=%d clock_pins[%d]=%d", state->index + 1,
            i, options->data_pins[i],
            i, options->inv_data_pins[i],
            i, options->clock_pins[i]
          );
      }
    } else {
      // serial output
      options->data_pins_count = 0;

      options->data_pin = config->i2s_data_pin_count > 0 ? config->i2s_data_pins[0] : GPIO_NUM_NC;
      options->inv_data_pin = config->i2s_data_inv_pin_count > 0 ? config->i2s_data_inv_pins[0] : GPIO_NUM_NC;
      options->clock_pin = config->i2s_clock_pin_count > 0 ? config->i2s_clock_pins[0] : GPIO_NUM_NC;

      LOG_INFO("leds%d: serial data_pin=%d inv_data_pin=%d clock_pin=%d", state->index + 1,
        options->data_pin,
        options->inv_data_pin,
        options->clock_pin
      );
    }
    // TODO: use i2s_pin_mutex for arbitrary gpio pins?
  #endif
    options->clock_rate = config->i2s_clock;

    LOG_INFO("leds%d: i2s port=%d: pin_mutex=%p data_pin_count=%u clock_rate=%d", state->index + 1,
      i2s_config->port,
      options->pin_mutex,
    #if LEDS_I2S_GPIO_PINS_ENABLED
      options->data_pins_count,
    #else
      0,
    #endif
      options->clock_rate
    );

    return 0;
  }

  int check_leds_i2s(struct leds_state *state)
  {
  #if CONFIG_IDF_TARGET_ESP8266
    const struct leds_options *options = leds_options(state->leds);

    if (is_console_running()) {
      if (options->i2s.pin_mutex && !uxSemaphoreGetCount(options->i2s.pin_mutex)) {
        LOG_WARN("I2S data pin busy, console running on UART0");
        return 1;
      }
    }
  #endif

    return 0;
  }
#endif
