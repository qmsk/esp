#include <gpio.h>
#include "../gpio.h"
#include "gpio_intr.h"

#include <esp_err.h>
#include <esp_intr_alloc.h>

#include <logging.h>

#define GPIO_INTR_ALLOC_FLAGS (0)

intr_handle_t gpio_intr_handle;

struct gpio_intr_options {
  enum gpio_intr_type {
    GPIO_INTR_TYPE_NONE   = 0,
    GPIO_INTR_TYPE_HOST,
    GPIO_INTR_TYPE_I2C,
  } type;

  union {
    const struct gpio_options *host;
    const struct gpio_i2c_dev *i2c_dev;
  };
} gpio_intr_options[GPIO_HOST_PIN_COUNT] = {};

static IRAM_ATTR void gpio_isr (void *arg)
{
  int core = cpu_ll_get_core_id();
  gpio_pins_t pins = gpio_intr_status(core);

  LOG_ISR_DEBUG("core=%d pins=" GPIO_PINS_FMT, core, GPIO_PINS_ARGS(pins));

  for (gpio_pin_t gpio = 0; gpio < GPIO_HOST_PIN_COUNT; gpio++) {
    if ((pins & GPIO_PINS(gpio))) {
      const struct gpio_intr_options *options = &gpio_intr_options[gpio];

      switch(options->type) {
        case GPIO_INTR_TYPE_NONE:
          break;

        case GPIO_INTR_TYPE_HOST:
          LOG_ISR_DEBUG("pin=%d host options=%p", gpio, options->host);
          gpio_host_intr_handler(options->host, pins);
          break;

        case GPIO_INTR_TYPE_I2C:
          LOG_ISR_DEBUG("pin=%d i2c dev=%p", gpio, options->i2c_dev);
          gpio_i2c_intr_handler(options->i2c_dev, pins);
          break;
      }
    }
  }

  gpio_intr_clear(pins);
}

int gpio_intr_init()
{
  esp_err_t err;

  if ((err = esp_intr_alloc(ETS_GPIO_INTR_SOURCE, GPIO_INTR_ALLOC_FLAGS, gpio_isr, NULL, &gpio_intr_handle))) {
    LOG_ERROR("esp_intr_alloc: %s", esp_err_to_name(err));
    return -1;
  } else {
    LOG_DEBUG("gpio_intr_handle=%p core=%d", gpio_intr_handle, esp_intr_get_cpu(gpio_intr_handle));
  }

  return 0;
}

void gpio_intr_setup_host_pin(gpio_pin_t gpio, const struct gpio_options *options)
{
  if (gpio >= GPIO_HOST_PIN_COUNT) {
    LOG_FATAL("[%d] invalid gpio_pin_t", gpio);
  }

  if (gpio_intr_options[gpio].type) {
    LOG_WARN("[%d] intr pin conflict type=%u", gpio, gpio_intr_options[gpio].type);
  }

  LOG_DEBUG("[%d] host options=%p", gpio, options);

  gpio_intr_options[gpio] = (struct gpio_intr_options) {
    .type = GPIO_INTR_TYPE_HOST,
    .host = options,
  };
}

void gpio_intr_setup_i2c_pin(gpio_pin_t gpio, const struct gpio_i2c_dev *i2c_dev)
{
  if (gpio >= GPIO_HOST_PIN_COUNT) {
    LOG_FATAL("[%d] invalid gpio_pin_t", gpio);
  }

  if (gpio_intr_options[gpio].type) {
    LOG_WARN("[%d] intr pin conflict type=%u", gpio, gpio_intr_options[gpio].type);
  }

  LOG_DEBUG("[%d] i2c dev=%p", gpio, i2c_dev);

  gpio_intr_options[gpio] = (struct gpio_intr_options) {
    .type     = GPIO_INTR_TYPE_I2C,
    .i2c_dev  = i2c_dev,
  };
}

int gpio_intr_core()
{
  return esp_intr_get_cpu(gpio_intr_handle);
}
