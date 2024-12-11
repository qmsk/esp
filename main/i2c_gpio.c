#include "i2c_config.h"
#include "i2c_master.h"
#include "i2c_gpio.h"

#include <logging.h>

#if GPIO_I2C_ENABLED
  const struct config_enum i2c_gpio_type_enum[] = {
    { "",         .value = GPIO_I2C_TYPE_NONE        },
    { "PCA9534",  .value = GPIO_I2C_TYPE_PCA9534     },
    { "PCA9554",  .value = GPIO_I2C_TYPE_PCA9554     },
    {},
  };
#endif

#if CONFIG_I2C_GPIO_ENABLED
  // sdkconfig
  const struct gpio_i2c_options i2c_gpio_options0 = {
  #if CONFIG_I2C_GPIO_TYPE_NONE
    .type = GPIO_I2C_TYPE_NONE,
  #elif CONFIG_I2C_GPIO_TYPE_PCA9534
    .type = GPIO_I2C_TYPE_PCA9534,
    .port = I2C_MASTER_PORT,
    .addr = CONFIG_I2C_GPIO_ADDR_PCA9534,
    .timeout = (CONFIG_I2C_GPIO_TIMEOUT / portTICK_RATE_MS),
    .int_pin = CONFIG_I2C_GPIO_INT_PIN,
  #elif CONFIG_I2C_GPIO_TYPE_PCA9554
    .type = GPIO_I2C_TYPE_PCA9554,
    .port = I2C_MASTER_PORT,
    .addr = CONFIG_I2C_GPIO_ADDR_PCA9554,
    .timeout = (CONFIG_I2C_GPIO_TIMEOUT / portTICK_RATE_MS),
    .int_pin = CONFIG_I2C_GPIO_INT_PIN,
  #else
    #error "Invalid I2C_GPIO_TYPE configured"
  #endif
  };

  // config
  struct i2c_gpio_config i2c_gpio_configs[I2C_GPIO_COUNT] = {};

  #define I2C_GPIO_CONFIGTAB i2c_gpio_configtab0
  #define I2C_GPIO_CONFIG i2c_gpio_configs[0]
  #include "i2c_gpio_configtab.i"
  #undef I2C_GPIO_CONFIGTAB
  #undef I2C_GPIO_CONFIG

  #define I2C_GPIO_CONFIGTAB i2c_gpio_configtab1
  #define I2C_GPIO_CONFIG i2c_gpio_configs[1]
  #include "i2c_gpio_configtab.i"
  #undef I2C_GPIO_CONFIGTAB
  #undef I2C_GPIO_CONFIG

  #define I2C_GPIO_CONFIGTAB i2c_gpio_configtab2
  #define I2C_GPIO_CONFIG i2c_gpio_configs[2]
  #include "i2c_gpio_configtab.i"
  #undef I2C_GPIO_CONFIGTAB
  #undef I2C_GPIO_CONFIG

  #define I2C_GPIO_CONFIGTAB i2c_gpio_configtab3
  #define I2C_GPIO_CONFIG i2c_gpio_configs[3]
  #include "i2c_gpio_configtab.i"
  #undef I2C_GPIO_CONFIGTAB
  #undef I2C_GPIO_CONFIG

  const struct configtab *i2c_gpio_configtabs[I2C_GPIO_COUNT] = {
    i2c_gpio_configtab0,
    i2c_gpio_configtab1,
    i2c_gpio_configtab2,
    i2c_gpio_configtab3,
  };

  // state
  struct gpio_i2c_dev *i2c_gpio_devs[I2C_GPIO_COUNT + 1];

  int init_i2c_gpio()
  {
    int err;

    LOG_INFO("i2c-gpio0: type=%s port=%u addr=%u int_pin=%u",
      config_enum_to_string(i2c_gpio_type_enum, i2c_gpio_options0.type) ?: "?",
      i2c_gpio_options0.port,
      i2c_gpio_options0.addr,
      i2c_gpio_options0.int_pin
    );

    if ((err = gpio_i2c_new(&i2c_gpio_devs[0], &i2c_gpio_options0))) {
      LOG_ERROR("gpio_i2c_new");
      return err;
    }

    return 0;
  }

  int start_i2c_gpio()
  {
    int err;

    for (unsigned i = 0; i < I2C_GPIO_COUNT; i++) {
      const struct i2c_gpio_config *config = &i2c_gpio_configs[i];
      const struct gpio_i2c_options options = {
        .type     = config->type,
        .port     = I2C_MASTER_PORT,
        .addr     = config->addr,
        .timeout = (CONFIG_I2C_GPIO_TIMEOUT / portTICK_RATE_MS),
        .int_pin  = config->int_pin,
      };

      if (!config->type) {
        continue;
      }

      LOG_INFO("i2c-gpio%u: type=%s port=%u addr=%u int_pin=%u", i + 1,
        config_enum_to_string(i2c_gpio_type_enum, options.type) ?: "?",
        options.port,
        options.addr,
        options.int_pin
      );

      if ((err = gpio_i2c_new(&i2c_gpio_devs[i + 1], &options))) {
        LOG_ERROR("gpio_i2c_new[%u]", i + 1);
        return err;
      }
    }

    return 0;
  }

#endif
