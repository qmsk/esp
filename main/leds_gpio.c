#include "leds.h"
#include "leds_config.h"
#include "leds_state.h"

#include <gpio_out.h>

#include <logging.h>

#if CONFIG_LEDS_GPIO_ENABLED
  struct gpio_out leds_gpio_out;

  int init_leds_gpio()
  {
    bool enabled = false;
    int err;

    for (int i = 0; i < LEDS_COUNT; i++)
    {
      const struct leds_config *config = &leds_configs[i];

      if (!config->enabled) {
        continue;
      }

      switch ((enum leds_gpio_mode) config->gpio_mode) {
        case LEDS_GPIO_MODE_DISABLED:
          break;

        case LEDS_GPIO_MODE_LOW:
          enabled = true;

          LOG_INFO("leds%d: gpio mode=LOW pin=%d", i + 1, config->gpio_pin);

          leds_gpio_out.pins |= gpio_out_pin(config->gpio_pin);
          leds_gpio_out.inverted |= gpio_out_pin(config->gpio_pin);
          break;

        case LEDS_GPIO_MODE_HIGH:
          enabled = true;

          LOG_INFO("leds%d: gpio mode=HIGH pin=%d", i + 1, config->gpio_pin);

          leds_gpio_out.pins |= gpio_out_pin(config->gpio_pin);
          break;

        default:
          LOG_ERROR("leds%d: invalid gpio_mode=%d", state->index + 1, config->gpio_mode);
          return -1;
      }
    }

    if (!enabled) {
      LOG_INFO("leds: gpio not configured");
      return 0;
    }

    LOG_INFO("leds: gpio -> pins=%08x inverted=%08x",
      leds_gpio_out.pins,
      leds_gpio_out.inverted
    );

    if ((err = gpio_out_setup(&leds_gpio_out))) {
      LOG_ERROR("gpio_out_setup");
      return err;
    }

    return 0;
  }

  int config_leds_gpio(struct leds_state *state, const struct leds_config *config, struct leds_options *options)
  {
    LOG_INFO("leds%d: gpio mode=%s pin=%d", state->index + 1,
        config_enum_to_string(leds_gpio_mode_enum, config->gpio_mode),
        config->gpio_pin
    );

    options->gpio_out = &leds_gpio_out;
    options->gpio_out_pins = gpio_out_pin(config->gpio_pin);

    return 0;
  }
#endif
