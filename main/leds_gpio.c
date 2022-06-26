#include "leds.h"
#include "leds_config.h"
#include "leds_state.h"
#include "i2c_config.h"

#include <gpio.h>

#include <logging.h>

#define LEDS_GPIO_I2C_TIMEOUT (1 / portTICK_RATE_MS)

#if CONFIG_LEDS_GPIO_ENABLED
  // config
  struct leds_gpio_config leds_gpio_config = { };

  const struct config_enum leds_gpio_type_enum[] = {
    { "HOST",     GPIO_TYPE_HOST            },
  #if LEDS_GPIO_I2C_ENABLED
    { "PCA9534",  GPIO_TYPE_I2C_PCA9534     },
    { "PCA9554",  GPIO_TYPE_I2C_PCA9554     },
  #endif
    {},
  };

  const struct configtab leds_gpio_configtab[] = {
    { CONFIG_TYPE_ENUM, "type",
      .description = "Select GPIO interface type.",
      .enum_type = { .value = &leds_gpio_config.type, .values = leds_gpio_type_enum, .default_value = GPIO_TYPE_HOST },
    },
  #if LEDS_GPIO_I2C_ENABLED
    { CONFIG_TYPE_UINT16, "i2c_addr",
      .description = "Select I2C GPIO device address.",
      .uint16_type = { .value = &leds_gpio_config.i2c_addr, .max = GPIO_I2C_ADDR_MAX },
    },
  #endif
    {},
  };

  // by interface
  struct gpio_options leds_gpio_options[LEDS_INTERFACE_COUNT] = {
  #if CONFIG_LEDS_SPI_ENABLED
    [LEDS_INTERFACE_SPI] = {
      .type   = GPIO_TYPE_HOST,
    },
  #endif
  #if CONFIG_LEDS_UART_ENABLED
    [LEDS_INTERFACE_UART] = {
      .type   = GPIO_TYPE_HOST,
    },
  #endif
  #if CONFIG_LEDS_I2S_ENABLED
    [LEDS_INTERFACE_I2S] = {
      .type   = GPIO_TYPE_HOST,
    },
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

      for (unsigned i = 0; i < config->gpio_count; i++) {
        LOG_INFO("leds%d: gpio[%s] mode=%s pin[%u]=%d", i + 1,
          config_enum_to_string(leds_interface_enum, interface) ?: "?",
          config_enum_to_string(leds_gpio_mode_enum, config->gpio_mode) ?: "?",
          i, config->gpio_pin[i]
        );

        leds_gpio_options[interface].out_pins |= gpio_host_pin(config->gpio_pin[i]);

        if (config->gpio_mode == LEDS_GPIO_MODE_LOW) {
          leds_gpio_options[interface].inverted_pins |= gpio_host_pin(config->gpio_pin[i]);
        }
      }
    }

    if (!enabled) {
      LOG_INFO("leds: gpio not configured");
      return 0;
    }

    for (enum leds_interface interface = 0; interface < LEDS_INTERFACE_COUNT; interface++) {
      struct gpio_options *options = &leds_gpio_options[interface];

      if (!interfaces_enabled[interface]) {
        continue;
      }

      switch ((options->type = leds_gpio_config.type)) {
        case GPIO_TYPE_HOST:
          break;

      #if LEDS_GPIO_I2C_ENABLED
        case GPIO_TYPE_I2C_PCA9534:
        case GPIO_TYPE_I2C_PCA9554:
          options->i2c.port = I2C_MASTER_PORT;
          options->i2c.addr = leds_gpio_config.i2c_addr;
          options->i2c.timeout = LEDS_GPIO_I2C_TIMEOUT;

          LOG_INFO("leds: gpio[%s] i2c port=%d addr=%u timeout=%d",
            config_enum_to_string(leds_interface_enum, interface),
            options->i2c.port,
            options->i2c.addr,
            options->i2c.timeout
          );

          break;
      #endif

        default:
          LOG_ERROR("invalid leds-gpio type=%d", options->type);
          return -1;
      }


      LOG_INFO("leds: gpio[%s] -> type=%d out_pins=" GPIO_PINS_FMT " inverted_pins=" GPIO_PINS_FMT,
        config_enum_to_string(leds_interface_enum, interface),
        options->type,
        GPIO_PINS_ARGS(options->out_pins),
        GPIO_PINS_ARGS(options->inverted_pins)
      );

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
