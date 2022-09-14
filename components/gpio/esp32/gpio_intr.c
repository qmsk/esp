#include <gpio.h>
#include "../gpio.h"
#include "gpio_intr.h"

#include <esp_err.h>
#include <esp_intr_alloc.h>

#include <logging.h>

#define GPIO_INTR_ALLOC_FLAGS (0)

intr_handle_t gpio_intr_handle;

const struct gpio_options *gpio_intr_options[GPIO_HOST_PIN_COUNT] = {};

static IRAM_ATTR void gpio_isr (void *arg)
{
  int core = cpu_ll_get_core_id();
  gpio_pins_t pins = gpio_intr_status(core);

  LOG_ISR_DEBUG("core=%d pins=" GPIO_PINS_FMT, core, GPIO_PINS_ARGS(pins));

  for (gpio_pin_t gpio = 0; gpio < GPIO_HOST_PIN_COUNT; gpio++) {
    const struct gpio_options *options;

    if ((pins & GPIO_PINS(gpio)) && (options = gpio_intr_options[gpio])) {
      switch(options->type) {
        case GPIO_TYPE_HOST:
          gpio_host_intr_handler(options, pins);
          break;

        case GPIO_TYPE_I2C:
          gpio_i2c_intr_handler(options, pins);
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

void gpio_intr_setup_pin(const struct gpio_options *options, gpio_pin_t gpio)
{
  if (gpio >= GPIO_HOST_PIN_COUNT) {
    LOG_WARN("[%d] -> %p: invalid gpio_pin_t", gpio, options);
    return;
  }

  LOG_DEBUG("[%d] -> %p", gpio, options);

  gpio_intr_options[gpio] = options;
}

int gpio_intr_core()
{
  return esp_intr_get_cpu(gpio_intr_handle);
}
