#include "atx_psu_gpio.h"
#include "gpio_type.h"
#include "i2c_gpio.h"

#include <logging.h>

#if GPIO_I2C_ENABLED
  #define ATX_PSU_GPIO_I2C_TIMEOUT (20 / portTICK_RATE_MS)
#endif

struct gpio_options atx_psu_gpio_options = {

};

int init_atx_psu_gpio(const struct atx_psu_config *config)
{
  int err;

  if ((err = set_gpio_type(&atx_psu_gpio_options, config->gpio_type))) {
    LOG_ERROR("invalid gpio_type=%d", config->gpio_type);
    return err;
  }

  if (config->power_enable_gpio_mode != ATX_PSU_GPIO_MODE_DISABLED) {
    LOG_INFO("power-enable gpio type=%s mode=%s out pin=%d",
      config_enum_to_string(gpio_type_enum, config->gpio_type) ?: "?",
      config_enum_to_string(atx_psu_gpio_mode_enum, config->power_enable_gpio_mode) ?: "?",
      config->power_enable_gpio
    );

    atx_psu_gpio_options.out_pins |= gpio_host_pin(config->power_enable_gpio);

    if (config->power_enable_gpio_mode == ATX_PSU_GPIO_MODE_LOW) {
      atx_psu_gpio_options.inverted_pins |= gpio_host_pin(config->power_enable_gpio);
    }
  }

  if (config->power_good_gpio_mode != ATX_PSU_GPIO_MODE_DISABLED) {
    LOG_INFO("power-good gpio type=%s mode=%s in pin=%d",
      config_enum_to_string(gpio_type_enum, config->gpio_type) ?: "?",
      config_enum_to_string(atx_psu_gpio_mode_enum, config->power_good_gpio_mode) ?: "?",
      config->power_good_gpio
    );

    atx_psu_gpio_options.in_pins |= gpio_host_pin(config->power_good_gpio);

    if (config->power_good_gpio_mode == ATX_PSU_GPIO_MODE_LOW) {
      atx_psu_gpio_options.inverted_pins |= gpio_host_pin(config->power_good_gpio);
    }
  }

  const struct gpio_i2c_options *i2c_options;

  switch(atx_psu_gpio_options.type) {
    case GPIO_TYPE_HOST:
      LOG_INFO("gpio host: in_pins=" GPIO_PINS_FMT " out_pins=" GPIO_PINS_FMT " inverted_pins=" GPIO_PINS_FMT,
        GPIO_PINS_ARGS(atx_psu_gpio_options.in_pins),
        GPIO_PINS_ARGS(atx_psu_gpio_options.out_pins),
        GPIO_PINS_ARGS(atx_psu_gpio_options.inverted_pins)
      );
      break;

    case GPIO_TYPE_I2C:
      i2c_options = gpio_i2c_options(atx_psu_gpio_options.i2c_dev);

      atx_psu_gpio_options.i2c_timeout = ATX_PSU_GPIO_I2C_TIMEOUT;

      LOG_INFO("gpio i2c type=%s port=%d addr=%u timeout=%u: in_pins=" GPIO_PINS_FMT " out_pins=" GPIO_PINS_FMT " inverted_pins=" GPIO_PINS_FMT,
        config_enum_to_string(i2c_gpio_type_enum, i2c_options->type) ?: "?",
        i2c_options->port,
        i2c_options->addr,
        atx_psu_gpio_options.i2c_timeout,
        GPIO_PINS_ARGS(atx_psu_gpio_options.in_pins),
        GPIO_PINS_ARGS(atx_psu_gpio_options.out_pins),
        GPIO_PINS_ARGS(atx_psu_gpio_options.inverted_pins)
      );
      break;
  }

  if ((err = gpio_setup(&atx_psu_gpio_options))) {
    LOG_ERROR("gpio_setup");
    return err;
  }

  return 0;
}

void config_atx_psu_gpio(const struct atx_psu_config *config, struct atx_psu_options *options)
{
  options->gpio_options = &atx_psu_gpio_options;
}
