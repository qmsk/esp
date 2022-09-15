#include <gpio.h>
#include "gpio.h"

#include <logging.h>

#include <stdlib.h>

IRAM_ATTR void gpio_i2c_intr_handler (const struct gpio_options *options, gpio_pins_t pins)
{
  struct gpio_i2c_dev *dev = options->i2c_dev;

  for (unsigned i = 0; i < GPIO_I2C_PINS_MAX; i++) {
    if (!(options = dev->intr_pins[i])) {
      continue;
    }

    if (!options->interrupt_pins) {
      continue;
    }

    if (options->interrupt_func) {
      // external GPIO interrupt, unknown pins
      options->interrupt_func(options->interrupt_pins, options->interrupt_arg);
    }
  }
}

int gpio_i2c_init (struct gpio_i2c_dev *dev, const struct gpio_i2c_options *options)
{
  int err;

  dev->options = *options;

  if (!(dev->mutex = xSemaphoreCreateMutex())) {
    LOG_ERROR("xSemaphoreCreateMutex");
    return -1;
  }

  if (options->int_pin > 0) {
    if ((err = gpio_host_setup_intr_pin(options->int_pin, GPIO_INTR_NEGEDGE))) {
      LOG_ERROR("gpio_host_setup_intr_pin");
      return err;
    }
  }

  switch(options->type) {
    case GPIO_I2C_TYPE_PCA9534:
    case GPIO_I2C_TYPE_PCA9554:
      return gpio_i2c_pca54xx_init(&dev->state.pca54xx);

    default:
      LOG_FATAL("unsupported type=%d", options->type);
  }

  return 0;
}

int gpio_i2c_new(struct gpio_i2c_dev **devp, const struct gpio_i2c_options *options)
{
  struct gpio_i2c_dev *dev;
  int err;

  if (!(dev = calloc(1, sizeof(*dev)))) {
    LOG_ERROR("calloc");
    return -1;
  }

  if ((err = gpio_i2c_init(dev, options))) {
    LOG_ERROR("gpio_i2c_init");
    err = -1;
    goto error;
  }

  *devp = dev;

  return 0;

error:
  free(dev);
  return err;
}

const struct gpio_i2c_options *gpio_i2c_options(struct gpio_i2c_dev *dev)
{
  return &dev->options;
}

int gpio_i2c_setup(const struct gpio_options *options)
{
  if (!options->i2c_dev) {
    LOG_ERROR("Invalid i2c_dev=NULL");
    return -1;
  }

  if (options->interrupt_pins) {
    if (options->i2c_dev->options.int_pin > 0) {
      // XXX: multiple gpio_i2c_dev sharing the same int_pin?
      gpio_intr_setup_pin(options, options->i2c_dev->options.int_pin);
    } else {
      LOG_ERROR("interrupt_pins without i2c_dev int_pin");
      return -1;
    }

    for (gpio_pin_t pin = 0; pin < GPIO_I2C_PINS_MAX; pin++) {
      if (options->interrupt_pins & GPIO_PINS(pin)) {
        options->i2c_dev->intr_pins[pin] = options;
      }
    }
  }

  switch(options->i2c_dev->options.type) {
    case GPIO_I2C_TYPE_PCA9534:
    case GPIO_I2C_TYPE_PCA9554:
      return gpio_i2c_pca54xx_setup(options);

    default:
      LOG_FATAL("unsupported type=%d", options->i2c_dev->options.type);
  }
}

int gpio_i2c_setup_input(const struct gpio_options *options, gpio_pins_t pins)
{
  switch(options->i2c_dev->options.type) {
    case GPIO_I2C_TYPE_PCA9534:
    case GPIO_I2C_TYPE_PCA9554:
      return gpio_i2c_pca54xx_setup_input(options, pins);

    default:
      LOG_FATAL("unsupported type=%d", options->i2c_dev->options.type);
  }
}

int gpio_i2c_get(const struct gpio_options *options, gpio_pins_t *pins)
{
  switch(options->i2c_dev->options.type) {
    case GPIO_I2C_TYPE_PCA9534:
    case GPIO_I2C_TYPE_PCA9554:
      return gpio_i2c_pca54xx_get(options, pins);

    default:
      LOG_FATAL("unsupported type=%d", options->i2c_dev->options.type);
  }
}

int gpio_i2c_setup_output(const struct gpio_options *options, gpio_pins_t pins)
{
  switch(options->i2c_dev->options.type) {
    case GPIO_I2C_TYPE_PCA9534:
    case GPIO_I2C_TYPE_PCA9554:
      return gpio_i2c_pca54xx_setup_output(options, pins);

    default:
      LOG_FATAL("unsupported type=%d", options->i2c_dev->options.type);
  }
}

int gpio_i2c_set(const struct gpio_options *options, gpio_pins_t pins)
{
  switch(options->i2c_dev->options.type) {
    case GPIO_I2C_TYPE_PCA9534:
    case GPIO_I2C_TYPE_PCA9554:
      return gpio_i2c_pca54xx_set(options, pins);

    default:
      LOG_FATAL("unsupported type=%d", options->i2c_dev->options.type);
  }
}
