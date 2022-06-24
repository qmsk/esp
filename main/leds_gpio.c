#include "leds.h"
#include "leds_config.h"
#include "leds_state.h"

#include <gpio.h>

#include <logging.h>

#if CONFIG_LEDS_GPIO_ENABLED
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

        leds_gpio_options[interface].pins |= gpio_host_pin(config->gpio_pin[i]);

        if (config->gpio_mode == LEDS_GPIO_MODE_LOW) {
          leds_gpio_options[interface].inverted |= gpio_host_pin(config->gpio_pin[i]);
        }
      }
    }

    if (!enabled) {
      LOG_INFO("leds: gpio not configured");
      return 0;
    }

    for (enum leds_interface interface = 0; interface < LEDS_INTERFACE_COUNT; interface++) {
      if (!interfaces_enabled[interface]) {
        continue;
      }

      LOG_INFO("leds: gpio[%s] -> pins=" GPIO_PINS_FMT " inverted=" GPIO_PINS_FMT,
        config_enum_to_string(leds_interface_enum, interface),
        GPIO_PINS_ARGS(leds_gpio_options[interface].pins),
        GPIO_PINS_ARGS(leds_gpio_options[interface].inverted)
      );

      if ((err = gpio_init(&leds_gpio_options[interface]))) {
        LOG_ERROR("gpio_init");
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
