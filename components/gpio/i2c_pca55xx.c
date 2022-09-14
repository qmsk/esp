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

  static int gpio_i2c_pca54xx_write(const struct gpio_i2c_options *options, enum pca55xx_cmd cmd, uint8_t value, TickType_t timeout)
  {
    uint8_t buf[] = { cmd, value };
    esp_err_t err;

    if ((err = i2c_master_write_to_device(options->port, GPIO_I2C_PCA9554_ADDR(options->addr), buf, sizeof(buf), timeout))) {
      LOG_ERROR("i2c_master_write_to_device port=%d addr=%u: %s", options->port, GPIO_I2C_PCA9554_ADDR(options->addr), esp_err_to_name(err));
      return -1;
    }

    return 0;
  }

  static int gpio_i2c_pca54xx_read(const struct gpio_i2c_options *options, enum pca55xx_cmd cmd, uint8_t *value, TickType_t timeout)
  {
    uint8_t wbuf[] = { cmd };
    uint8_t rbuf[1] = { };
    esp_err_t err;

    if ((err = i2c_master_write_read_device(options->port, GPIO_I2C_PCA9554_ADDR(options->addr), wbuf, sizeof(wbuf), rbuf, sizeof(rbuf), timeout))) {
      LOG_ERROR("i2c_master_write_to_device port=%d addr=%u: %s", options->port, GPIO_I2C_PCA9554_ADDR(options->addr), esp_err_to_name(err));
      return -1;
    }

    *value = rbuf[0];

    return 0;
  }

  static int gpio_i2c_pca54xx_input(struct gpio_i2c_dev *dev, uint8_t mask, uint8_t *valuep, TickType_t timeout)
  {
    uint8_t value;
    int err;

    if ((err = gpio_i2c_pca54xx_read(&dev->options, PCA55XX_CMD_INPUT_PORT, &value, timeout))) {
      return err;
    }

    *valuep = value & mask;

    return 0;
  }

  static int gpio_i2c_pca54xx_output(struct gpio_i2c_dev *dev, uint8_t mask, uint8_t value, TickType_t timeout)
  {
    int err = 0;

    if (!xSemaphoreTake(dev->mutex, timeout)) {
      LOG_ERROR("xSemaphoreTake");
      return 1;
    }

    dev->state.pca54xx.output = (dev->state.pca54xx.output & (~mask)) | value;

    if ((err = gpio_i2c_pca54xx_write(&dev->options, PCA55XX_CMD_OUTPUT_PORT, dev->state.pca54xx.output, timeout))) {
      LOG_ERROR("gpio_i2c_pca54xx_write");
      goto error;
    }

error:
    xSemaphoreGive(dev->mutex);

    return err;
  }

  static int gpio_i2c_pca54xx_config(struct gpio_i2c_dev *dev, uint8_t mask, uint8_t value, TickType_t timeout)
  {
    int err = 0;

    if (!xSemaphoreTake(dev->mutex, timeout)) {
      LOG_ERROR("xSemaphoreTake");
      return 1;
    }

    dev->state.pca54xx.config = (dev->state.pca54xx.config & (~mask)) | value;

    if ((err = gpio_i2c_pca54xx_write(&dev->options, PCA55XX_CMD_CONFIG_PORT, dev->state.pca54xx.config, timeout))) {
      LOG_ERROR("gpio_i2c_pca54xx_write");
      goto error;
    }

error:
    xSemaphoreGive(dev->mutex);

    return err;
  }

  int gpio_i2c_pc54xx_setup(const struct gpio_options *options)
  {
    int err;

    if ((err = gpio_i2c_pca54xx_output(options->i2c_dev, pca55x_pins(options->out_pins), pca55x_pins(0 ^ options->inverted_pins), options->i2c_timeout))) {
      LOG_ERROR("gpio_i2c_pca54xx_output");
      return err;
    }

    if ((err = gpio_i2c_pca54xx_config(options->i2c_dev, pca55x_pins(options->in_pins | options->out_pins), pca55x_pins(~options->out_pins), options->i2c_timeout))) {
      LOG_ERROR("gpio_i2c_pca54xx_config");
      return err;
    }

    gpio_i2c_setup_intr(options->i2c_dev, options);

    return 0;
  }

  int gpio_i2c_pc54xx_setup_input(const struct gpio_options *options, gpio_pins_t pins)
  {
    return gpio_i2c_pca54xx_config(options->i2c_dev, pca55x_pins(options->in_pins | options->out_pins), pca55x_pins(~(options->out_pins & ~pins)), options->i2c_timeout);
  }

  int gpio_i2c_pc54xx_get(const struct gpio_options *options, gpio_pins_t *pins)
  {
    uint8_t value;
    int err;

    if ((err = gpio_i2c_pca54xx_input(options->i2c_dev, pca55x_pins(options->in_pins), &value, options->i2c_timeout))) {
      return err;
    }

    *pins = (((gpio_pins_t) value) ^ options->inverted_pins);

    return 0;
  }

  int gpio_i2c_pc54xx_setup_output(const struct gpio_options *options, gpio_pins_t pins)
  {
    return gpio_i2c_pca54xx_config(options->i2c_dev, pca55x_pins(options->in_pins | options->out_pins), pca55x_pins(~(options->out_pins & pins)), options->i2c_timeout);
  }

  int gpio_i2c_pc54xx_clear(const struct gpio_options *options)
  {
    return gpio_i2c_pca54xx_output(options->i2c_dev, pca55x_pins(options->out_pins), pca55x_pins(0 ^ options->inverted_pins), options->i2c_timeout);
  }

  int gpio_i2c_pc54xx_set(const struct gpio_options *options, gpio_pins_t pins)
  {
    return gpio_i2c_pca54xx_output(options->i2c_dev, pca55x_pins(options->out_pins), pca55x_pins(pins ^ options->inverted_pins), options->i2c_timeout);
  }

  int gpio_i2c_pc54xx_set_all(const struct gpio_options *options)
  {
    return gpio_i2c_pca54xx_output(options->i2c_dev, pca55x_pins(options->out_pins), pca55x_pins(GPIO_I2C_PCA9554_PINS_MASK ^ options->inverted_pins), options->i2c_timeout);
  }
#endif
