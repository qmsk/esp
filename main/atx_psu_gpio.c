#include "atx_psu_gpio.h"
#include "gpio_type.h"
#include "i2c_gpio.h"

#include <logging.h>

int config_atx_psu_gpio(const struct atx_psu_config *config, struct gpio_options *gpio_options)
{
  int err;

  if ((err = set_gpio_type(gpio_options, config->gpio_type))) {
    LOG_ERROR("invalid gpio_type=%d", config->gpio_type);
    return err;
  }

  if (config->power_enable_gpio_mode != ATX_PSU_GPIO_MODE_DISABLED) {
    LOG_INFO("power-enable gpio type=%s mode=%s out pin=%d",
      config_enum_to_string(gpio_type_enum, config->gpio_type) ?: "?",
      config_enum_to_string(atx_psu_gpio_mode_enum, config->power_enable_gpio_mode) ?: "?",
      config->power_enable_gpio
    );

    gpio_options->out_pins |= gpio_host_pin(config->power_enable_gpio);

    if (config->power_enable_gpio_mode == ATX_PSU_GPIO_MODE_LOW) {
      gpio_options->inverted_pins |= gpio_host_pin(config->power_enable_gpio);
    }
  }

  if (config->power_good_gpio_mode != ATX_PSU_GPIO_MODE_DISABLED) {
    LOG_INFO("power-good gpio type=%s mode=%s in pin=%d",
      config_enum_to_string(gpio_type_enum, config->gpio_type) ?: "?",
      config_enum_to_string(atx_psu_gpio_mode_enum, config->power_good_gpio_mode) ?: "?",
      config->power_good_gpio
    );

    gpio_options->in_pins |= gpio_host_pin(config->power_good_gpio);

    if (config->power_good_gpio_mode == ATX_PSU_GPIO_MODE_LOW) {
      gpio_options->inverted_pins |= gpio_host_pin(config->power_good_gpio);
    }
  }

#if GPIO_I2C_ENABLED
  const struct gpio_i2c_options *i2c_options;
#endif

  switch(gpio_options->type) {
    case GPIO_TYPE_HOST:
      LOG_INFO("gpio host: in_pins=" GPIO_PINS_FMT " out_pins=" GPIO_PINS_FMT " inverted_pins=" GPIO_PINS_FMT,
        GPIO_PINS_ARGS(gpio_options->in_pins),
        GPIO_PINS_ARGS(gpio_options->out_pins),
        GPIO_PINS_ARGS(gpio_options->inverted_pins)
      );
      break;

  #if GPIO_I2C_ENABLED
    case GPIO_TYPE_I2C:
      i2c_options = gpio_i2c_options(gpio_options->i2c_dev);

      LOG_INFO("gpio i2c type=%s port=%d addr=%u timeout=%u: in_pins=" GPIO_PINS_FMT " out_pins=" GPIO_PINS_FMT " inverted_pins=" GPIO_PINS_FMT,
        config_enum_to_string(i2c_gpio_type_enum, i2c_options->type) ?: "?",
        i2c_options->port,
        i2c_options->addr,
        i2c_options->timeout,
        GPIO_PINS_ARGS(gpio_options->in_pins),
        GPIO_PINS_ARGS(gpio_options->out_pins),
        GPIO_PINS_ARGS(gpio_options->inverted_pins)
      );
      break;
  #endif
  }

  return 0;
}
