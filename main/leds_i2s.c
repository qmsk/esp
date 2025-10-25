#include "leds.h"
#include "leds_config.h"
#include "leds_state.h"
#include "console.h"
#include "pin_mutex.h"

#include <logging.h>

// do not block if pin in use, timeout immediately
#define LEDS_I2S_PIN_TIMEOUT portMAX_DELAY

#if CONFIG_LEDS_I2S_ENABLED
  #if LEDS_I2S_PARALLEL_ENABLED
    unsigned config_leds_i2s_data_width(const struct leds_config *config)
    {
      if (config->i2s_data_width > 0) {
        return config->i2s_data_width;
      }

      if (config->i2s_data_pin_count > 1 || config->i2s_data_inv_pin_count > 1) {
        // parallel output
        return config->i2s_data_pin_count > config->i2s_data_inv_pin_count ? config->i2s_data_pin_count : config->i2s_data_inv_pin_count;
      } else if (config->i2s_data_pin_count == 1 || config->i2s_data_inv_pin_count == 1) {
        // serial output
        return 1;
      } else {
        // no output
        return 0;
      }
    }
  #endif

  // state
  struct i2s_out *leds_i2s_out[LEDS_I2S_INTERFACE_COUNT];

  int init_leds_i2s(unsigned port)
  {
    size_t buffer_size = 0, buffer_align = 0;
    unsigned data_repeat = 0;
    bool enabled = false;
    int err;

    for (int i = 0; i < LEDS_COUNT; i++)
    {
      const struct leds_config *config = &leds_configs[i];

      if (!config->enabled) {
        continue;
      }

      if (config->interface != LEDS_INTERFACE_I2S(port)) {
        continue;
      }

      enabled = true;

      // update maximum transfer size
      size_t size, align;

    #if LEDS_I2S_PARALLEL_ENABLED
      unsigned data_width = config_leds_i2s_data_width(config);

      if (data_width > 1) {
        size = leds_i2s_parallel_buffer_size(config->protocol, config->count, data_width);
        align = leds_i2s_parallel_buffer_align(config->protocol, data_width);

        LOG_INFO("leds%d: i2s%u configured for %u parallel leds on %u pins, data buffer size=%u align=%u", i + 1, port, config->count, data_width, size, align);
      } else if (data_width) {
        size = leds_i2s_serial_buffer_size(config->protocol, config->count);
        align = leds_i2s_serial_buffer_align(config->protocol);

        LOG_INFO("leds%d: i2s%u configured for %u serial leds, data buffer size=%u align=%u", i + 1, port, config->count, size, align);
      } else {
        size = 0;
        align = 0;

        LOG_WARN("leds%d: i2s%u configured for %u leds without data outputs", i + 1, port, config->count);
      }
    #else
      size = leds_i2s_serial_buffer_size(config->protocol, config->count);
      align = leds_i2s_serial_buffer_align(config->protocol);

      LOG_INFO("leds%d: i2s%u configured for %u serial leds, data buffer size=%u align=%u", i + 1, port, config->count, size, align);
    #endif

      if (size > buffer_size) {
        buffer_size = size;
      }
      if (align > buffer_align) {
        buffer_align = align;
      }
      if (config->i2s_data_copies > data_repeat + 1) {
        data_repeat = config->i2s_data_copies - 1;
      }
    }

    if (!enabled) {
      LOG_INFO("leds: i2s%u not configured", port);
      return 0;
    }

    LOG_INFO("leds: i2s%u -> buffer_size=%u buffer_align=%u repeat_data_count=%u", port,
      buffer_size, buffer_align, data_repeat
    );

    if ((err = i2s_out_new(&leds_i2s_out[port], port, buffer_size, buffer_align, data_repeat))) {
      LOG_ERROR("i2s_out_new(port=%d)", port);
      return err;
    }

    return 0;
  }

  int config_leds_i2s(struct leds_state *state, const struct leds_config *config, struct leds_interface_i2s_options *options)
  {
    unsigned port = leds_interface_i2s_port(config->interface);

    if (!leds_i2s_out[port]) {
      LOG_ERROR("leds%d: i2s out not initialized", state->index + 1);
      return -1;
    }

    options->i2s_out = leds_i2s_out[port];
  #if CONFIG_IDF_TARGET_ESP8266
  // TODO: use i2s_pin_mutex for arbitrary gpio pins with LEDS_I2S_GPIO_PINS_ENABLED?
    options->pin_mutex = pin_mutex[PIN_MUTEX_I2S0_DATA]; // shared with console uart0
    options->pin_timeout = LEDS_I2S_PIN_TIMEOUT;
  #endif
    options->clock_rate = config->i2s_clock;
  #if LEDS_I2S_GPIO_PINS_ENABLED
    options->gpio_pins_count = 0;
    
    // max
    if (config->i2s_clock_pin_count > options->gpio_pins_count) {
      options->gpio_pins_count = config->i2s_clock_pin_count;
    }
    if (config->i2s_data_pin_count > options->gpio_pins_count) {
      options->gpio_pins_count = config->i2s_data_pin_count;
    }
    if (config->i2s_data_inv_pin_count > options->gpio_pins_count) {
      options->gpio_pins_count = config->i2s_data_inv_pin_count;
    }
    
    for (int i = 0; i < options->gpio_pins_count; i++) {
        options->data_pins[i] = i < config->i2s_data_pin_count ? config->i2s_data_pins[i] : GPIO_NUM_NC;
        options->inv_data_pins[i] = i < config->i2s_data_inv_pin_count ? config->i2s_data_inv_pins[i] : GPIO_NUM_NC;
        options->clock_pins[i] = i < config->i2s_clock_pin_count ? config->i2s_clock_pins[i] : GPIO_NUM_NC;

        LOG_INFO("leds%d: i2s data_pins[%d]=%d inv_data_pins[%d]=%d clock_pins[%d]=%d", state->index + 1,
          i, options->data_pins[i],
          i, options->inv_data_pins[i],
          i, options->clock_pins[i]
        );
    }
  #endif
  #if LEDS_I2S_PARALLEL_ENABLED
    unsigned data_width = config_leds_i2s_data_width(config);

    if (data_width > 1) {
      options->parallel = data_width;
    } else if (data_width == 1) {
      // just use serial mode
      options->parallel = 0;
    } else {
      options->parallel = 0;
    }
  #endif

    if (config->i2s_data_copies > 1) {
      LOG_INFO("leds%d: repeat copies=%u", state->index + 1, config->i2s_data_copies);

      options->repeat = config->i2s_data_copies - 1;
    }

    LOG_INFO("leds%d: i2s%d pin_mutex=%p clock_rate=%d gpio_pins_count=%u parallel=%u repeat=%u", state->index + 1, port,
      options->pin_mutex,
      options->clock_rate,
    #if LEDS_I2S_GPIO_PINS_ENABLED
      options->gpio_pins_count,
    #else
      0,
    #endif
    #if LEDS_I2S_PARALLEL_ENABLED
      options->parallel,
    #else
      0,
    #endif
      options->repeat
    );

    return 0;
  }

# if CONFIG_IDF_TARGET_ESP8266
  int check_leds_i2s(struct leds_state *state)
  {
    const struct leds_options *options = leds_options(state->leds);

    if (is_console_running()) {
      if (options->i2s.pin_mutex && !uxSemaphoreGetCount(options->i2s.pin_mutex)) {
        LOG_WARN("I2S data pin busy, console running on UART0");
        return 1;
      }
    }
    
    return 0;
  }
# endif
#endif
