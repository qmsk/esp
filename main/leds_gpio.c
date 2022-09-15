#include "leds.h"
#include "leds_config.h"
#include "leds_state.h"
#include "gpio_type.h"

#include <gpio.h>

#include <logging.h>


#if CONFIG_LEDS_GPIO_ENABLED
  #if GPIO_I2C_ENABLED
    #define LEDS_GPIO_I2C_TIMEOUT (20 / portTICK_RATE_MS)
  #endif

  // by interface
  struct gpio_options leds_gpio_options[LEDS_INTERFACE_COUNT] = {
  #if CONFIG_LEDS_SPI_ENABLED
    [LEDS_INTERFACE_SPI] = { },
  #endif
  #if CONFIG_LEDS_UART_ENABLED
    [LEDS_INTERFACE_UART] = { },
  #endif
  #if CONFIG_LEDS_I2S_ENABLED
    [LEDS_INTERFACE_I2S] = { },
  #endif
  };

  int init_leds_gpio()
  {
    bool enabled = false;
    bool interfaces_enabled[LEDS_INTERFACE_COUNT] = {};
    int err;

    for (int i = 0; i < LEDS_COUNT; i++)
    {
      const struct leds_config *config = &leds_configs[i];
      enum leds_interface interface = config->interface;

      if (!config->enabled) {
        continue;
      }

      if (interface == LEDS_INTERFACE_NONE) {
        interface = leds_interface_for_protocol(config->protocol);
      }

      if (config->gpio_mode == LEDS_GPIO_MODE_DISABLED) {
        continue;
      }

      //
      enabled = true;
      interfaces_enabled[interface] = true;

      // options
      if ((err = set_gpio_type(&leds_gpio_options[interface], config->gpio_type))) {
        LOG_ERROR("leds%d: invalid gpio_type=%d", i + 1, config->gpio_type);
        return err;
      }

      for (unsigned j = 0; j < config->gpio_count; j++) {
        LOG_INFO("leds%d: gpio[%s] type=%s mode=%s pin[%u]=%d", i + 1,
          config_enum_to_string(leds_interface_enum, interface) ?: "?",
          config_enum_to_string(gpio_type_enum, config->gpio_type) ?: "?",
          config_enum_to_string(leds_gpio_mode_enum, config->gpio_mode) ?: "?",
          j, config->gpio_pin[j]
        );

        leds_gpio_options[interface].out_pins |= gpio_host_pin(config->gpio_pin[j]);

        if (config->gpio_mode == LEDS_GPIO_MODE_LOW) {
          leds_gpio_options[interface].inverted_pins |= gpio_host_pin(config->gpio_pin[j]);
        }
      }
    }

    if (!enabled) {
      LOG_INFO("leds: gpio not configured");
      return 0;
    }

    for (enum leds_interface interface = 0; interface < LEDS_INTERFACE_COUNT; interface++) {
      struct gpio_options *options = &leds_gpio_options[interface];
      const struct gpio_i2c_options *i2c_options;

      if (!interfaces_enabled[interface]) {
        continue;
      }

      switch (options->type) {
        case GPIO_TYPE_HOST:
          LOG_INFO("leds: gpio[%s] host: out_pins=" GPIO_PINS_FMT " inverted_pins=" GPIO_PINS_FMT,
            config_enum_to_string(leds_interface_enum, interface),
            GPIO_PINS_ARGS(options->out_pins),
            GPIO_PINS_ARGS(options->inverted_pins)
          );

          break;

      #if GPIO_I2C_ENABLED
        case GPIO_TYPE_I2C:
          i2c_options = gpio_i2c_options(options->i2c_dev);

          options->i2c_timeout = LEDS_GPIO_I2C_TIMEOUT;

          LOG_INFO("leds gpio[%s]: i2c type=%s port=%u addr=%u: out_pins=" GPIO_PINS_FMT " inverted_pins=" GPIO_PINS_FMT,
            config_enum_to_string(leds_interface_enum, interface),
            config_enum_to_string(i2c_gpio_type_enum, i2c_options->type) ?: "?",
            i2c_options->port,
            i2c_options->addr,
            GPIO_PINS_ARGS(options->out_pins),
            GPIO_PINS_ARGS(options->inverted_pins)
          );

          break;
      #endif

        default:
          LOG_ERROR("invalid leds-gpio type=%d", options->type);
          return -1;
      }


      if ((err = gpio_setup(options))) {
        LOG_ERROR("gpio_setup");
        return err;
      }
    }

    return 0;
  }

  int config_leds_gpio(struct leds_state *state, const struct leds_config *config, enum leds_interface interface, struct leds_interface_options_gpio *options)
  {
    if (config->gpio_mode == LEDS_GPIO_MODE_DISABLED) {
      return 0;
    }

    options->gpio_options = &leds_gpio_options[interface];
    options->gpio_out_pins = 0;

    for (unsigned i = 0; i < config->gpio_count; i++) {
      LOG_INFO("leds%d: gpio[%s] mode=%s pin[%u]=%d", state->index + 1,
        config_enum_to_string(leds_interface_enum, interface),
        config_enum_to_string(leds_gpio_mode_enum, config->gpio_mode),
        i, config->gpio_pin[i]
      );

      options->gpio_out_pins |= gpio_host_pin(config->gpio_pin[i]);
    }

    return 0;
  }
#endif
