#include <gpio.h>
#include "gpio.h"

#if GPIO_I2C_ENABLED
  #include <driver/i2c.h>
  #include <esp_err.h>

  #include <logging.h>

  enum pca55xx_cmd {
    PCA55XX_CMD_INPUT_PORT      = 0x00,
    PCA55XX_CMD_OUTPUT_PORT     = 0x01,
    PCA55XX_CMD_INVERSION_PORT  = 0x02,
    PCA55XX_CMD_CONFIG_PORT     = 0x03,
  };

  static inline uint8_t pca55x_pins(gpio_pins_t pins)
  {
    return (pins) & GPIO_I2C_PCA9554_PINS_MASK;
  }

  static int gpio_i2c_pc54xx_write(const struct gpio_i2c_options *options, enum pca55xx_cmd cmd, uint8_t value)
  {
    uint8_t buf[] = { cmd, value };
    esp_err_t err;

    if ((err = i2c_master_write_to_device(options->port, GPIO_I2C_PCA9554_ADDR(options->addr), buf, sizeof(buf), options->timeout))) {
      LOG_ERROR("i2c_master_write_to_device port=%d addr=%u: %s", options->port, GPIO_I2C_PCA9554_ADDR(options->addr), esp_err_to_name(err));
      return -1;
    }

    return 0;
  }

  static int gpio_i2c_pc54xx_read(const struct gpio_i2c_options *options, enum pca55xx_cmd cmd, uint8_t *value)
  {
    uint8_t wbuf[] = { cmd };
    uint8_t rbuf[1] = { };
    esp_err_t err;

    if ((err = i2c_master_write_read_device(options->port, GPIO_I2C_PCA9554_ADDR(options->addr), wbuf, sizeof(wbuf), rbuf, sizeof(rbuf), options->timeout))) {
      LOG_ERROR("i2c_master_write_to_device port=%d addr=%u: %s", options->port, GPIO_I2C_PCA9554_ADDR(options->addr), esp_err_to_name(err));
      return -1;
    }

    *value = rbuf[0];

    return 0;
  }

  int gpio_i2c_pc54xx_setup(const struct gpio_options *options)
  {
    int err;


    if ((err = gpio_i2c_pc54xx_write(&options->i2c, PCA55XX_CMD_OUTPUT_PORT, 0 ^ options->inverted_pins))) {
      LOG_ERROR("gpio_i2c_pc54xx_write");
      return err;
    }

    if ((err = gpio_i2c_pc54xx_write(&options->i2c, PCA55XX_CMD_CONFIG_PORT, pca55x_pins(~options->out_pins)))) {
      LOG_ERROR("gpio_i2c_pc54xx_write");
      return err;
    }

    if (options->i2c.int_pin > 0) {
      if ((err = gpio_host_setup_intr_pin(options, options->i2c.int_pin, GPIO_INTR_NEGEDGE))) {
        LOG_ERROR("gpio_host_setup_intr_pin");
        return err;
      }
    }

    return 0;
  }

  int gpio_i2c_pc54xx_setup_input(const struct gpio_options *options, gpio_pins_t pins)
  {
    return gpio_i2c_pc54xx_write(&options->i2c, PCA55XX_CMD_CONFIG_PORT, pca55x_pins(~(options->out_pins & ~pins)));
  }

  int gpio_i2c_pc54xx_get(const struct gpio_options *options, gpio_pins_t *pins)
  {
    uint8_t value;
    int err;

    if ((err = gpio_i2c_pc54xx_read(&options->i2c, PCA55XX_CMD_INPUT_PORT, &value))) {
      return err;
    }

    *pins = (((gpio_pins_t) value) ^ options->inverted_pins) & options->in_pins;

    return 0;
  }

  int gpio_i2c_pc54xx_setup_output(const struct gpio_options *options, gpio_pins_t pins)
  {
    return gpio_i2c_pc54xx_write(&options->i2c, PCA55XX_CMD_CONFIG_PORT, pca55x_pins(~(options->out_pins & pins)));
  }

  int gpio_i2c_pc54xx_clear(const struct gpio_options *options)
  {
    return gpio_i2c_pc54xx_write(&options->i2c, PCA55XX_CMD_OUTPUT_PORT, pca55x_pins(0 ^ options->inverted_pins));
  }

  int gpio_i2c_pc54xx_set(const struct gpio_options *options, gpio_pins_t pins)
  {
    return gpio_i2c_pc54xx_write(&options->i2c, PCA55XX_CMD_OUTPUT_PORT, pca55x_pins(pins ^ options->inverted_pins));
  }

  int gpio_i2c_pc54xx_set_all(const struct gpio_options *options)
  {
    return gpio_i2c_pc54xx_write(&options->i2c, PCA55XX_CMD_OUTPUT_PORT, pca55x_pins(GPIO_I2C_PCA9554_PINS_MASK ^ options->inverted_pins));
  }
#endif
