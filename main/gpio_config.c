#include "gpio_config.h"

#include <logging.h>

#include <sdkconfig.h>

/* Evaluate optional CONFIG_* in cpp macros */
#define __ARG_PLACEHOLDER_1 0,
#define ___CONFIG_ENABLED(__ignored, val, ...) val
#define __CONFIG_ENABLED(arg1_or_junk) ___CONFIG_ENABLED(arg1_or_junk 1, 0)
#define _CONFIG_ENABLED(config) __CONFIG_ENABLED(__ARG_PLACEHOLDER_##config)
#define CONFIG_ENABLED(config) _CONFIG_ENABLED(config)
#define IF_CONFIG_ENABLED(config, flags) (_CONFIG_ENABLED(config) ? (flags) : 0)

#define GPIO_TYPE_I2C_DEV(X) (GPIO_TYPE_I2C | ((X) << 8))

const struct config_enum gpio_type_enum[] = {
  { "HOST",       .value = GPIO_TYPE_HOST       },
#if CONFIG_I2C_GPIO_ENABLED
  { "I2C-GPIO0",  .value = GPIO_TYPE_I2C_DEV(0) },
  { "I2C-GPIO1",  .value = GPIO_TYPE_I2C_DEV(1) },
  { "I2C-GPIO2",  .value = GPIO_TYPE_I2C_DEV(2) },
  { "I2C-GPIO3",  .value = GPIO_TYPE_I2C_DEV(3) },
  { "I2C-GPIO4",  .value = GPIO_TYPE_I2C_DEV(4) },
#endif
  {},
};

#define CONFIG_ESP32_GPIO_PIN(x) \
    { \
        .type       = GPIO_TYPE_HOST, \
        .pin        = GPIO_HOST_PIN(x), \
        .name       = CONFIG_ESP32_GPIO ## x ## _NAME, \
        .reserved   = CONFIG_ENABLED(CONFIG_ESP32_GPIO ## x ## _RESERVED), \
        .flags      = (\
              0 \
            | IF_CONFIG_ENABLED(CONFIG_ESP32_GPIO ## x ## _FUNC_GPIO,         CONFIG_GPIO_OUTPUT | CONFIG_GPIO_INPUT) \
            | IF_CONFIG_ENABLED(CONFIG_ESP32_GPIO ## x ## _FUNC_GPIO_INPUT,   CONFIG_GPIO_INPUT) \
            | IF_CONFIG_ENABLED(CONFIG_ESP32_GPIO ## x ## _FUNC_GPIO_OUTPUT,  CONFIG_GPIO_OUTPUT) \
        ), \
    }

const struct config_gpio config_gpios[] = {
#if CONFIG_IDF_TARGET_ESP8266
    // TODO
#elif CONFIG_IDF_TARGET_ESP32
    CONFIG_ESP32_GPIO_PIN(0),
    CONFIG_ESP32_GPIO_PIN(1),
    CONFIG_ESP32_GPIO_PIN(2),
    CONFIG_ESP32_GPIO_PIN(3),
    CONFIG_ESP32_GPIO_PIN(4),
    CONFIG_ESP32_GPIO_PIN(5),
    CONFIG_ESP32_GPIO_PIN(6),
    CONFIG_ESP32_GPIO_PIN(7),
    CONFIG_ESP32_GPIO_PIN(8),
    CONFIG_ESP32_GPIO_PIN(9),
    CONFIG_ESP32_GPIO_PIN(10),
    CONFIG_ESP32_GPIO_PIN(11),
    CONFIG_ESP32_GPIO_PIN(12),
    CONFIG_ESP32_GPIO_PIN(13),
    CONFIG_ESP32_GPIO_PIN(14),
    CONFIG_ESP32_GPIO_PIN(15),
    CONFIG_ESP32_GPIO_PIN(16),
    CONFIG_ESP32_GPIO_PIN(17),
    CONFIG_ESP32_GPIO_PIN(18),
    CONFIG_ESP32_GPIO_PIN(19),
    CONFIG_ESP32_GPIO_PIN(21),
    CONFIG_ESP32_GPIO_PIN(22),
    CONFIG_ESP32_GPIO_PIN(23),
    CONFIG_ESP32_GPIO_PIN(25),
    CONFIG_ESP32_GPIO_PIN(26),
    CONFIG_ESP32_GPIO_PIN(27),
    CONFIG_ESP32_GPIO_PIN(32),
    CONFIG_ESP32_GPIO_PIN(33),
    CONFIG_ESP32_GPIO_PIN(34),
    CONFIG_ESP32_GPIO_PIN(35),
    CONFIG_ESP32_GPIO_PIN(36),
    CONFIG_ESP32_GPIO_PIN(39),
#endif
    {},
};

int config_gpio_type(struct gpio_options *options, int type)
{
  switch(type) {
    case GPIO_TYPE_HOST:
      options->type = GPIO_TYPE_HOST;

      return 0;

  #if CONFIG_I2C_GPIO_ENABLED
    case GPIO_TYPE_I2C_DEV(0):
      options->type = GPIO_TYPE_I2C;

      if (!(options->i2c_dev = i2c_gpio_devs[0])) {
        LOG_ERROR("I2C-GPIO0 not initialized");
        return -1;
      }

      return 0;

    case GPIO_TYPE_I2C_DEV(1):
      options->type = GPIO_TYPE_I2C;
      if (!(options->i2c_dev = i2c_gpio_devs[1])) {
        LOG_ERROR("I2C-GPIO1 not initialized");
        return -1;
      }

      return 0;

    case GPIO_TYPE_I2C_DEV(2):
      options->type = GPIO_TYPE_I2C;
      if (!(options->i2c_dev = i2c_gpio_devs[2])) {
        LOG_ERROR("I2C-GPIO2 not initialized");
        return -1;
      }

      return 0;

    case GPIO_TYPE_I2C_DEV(3):
      options->type = GPIO_TYPE_I2C;
      if (!(options->i2c_dev = i2c_gpio_devs[3])) {
        LOG_ERROR("I2C-GPIO3 not initialized");
        return -1;
      }

      return 0;

    case GPIO_TYPE_I2C_DEV(4):
      options->type = GPIO_TYPE_I2C;
      if (!(options->i2c_dev = i2c_gpio_devs[4])) {
        LOG_ERROR("I2C-GPIO4 not initialized");
        return -1;
      }

      return 0;
  #endif

    default:
      LOG_ERROR("Invalid type=%x", type);
      return -1;
  }
}
