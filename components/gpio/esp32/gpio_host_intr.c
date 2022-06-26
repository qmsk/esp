#define DEBUG

#include <gpio.h>
#include "../gpio.h"
#include "gpio_host.h"

#include <esp_err.h>
#include <esp_intr_alloc.h>

#include <logging.h>

#define GPIO_HOST_INTR_ALLOC_FLAGS (0)

intr_handle_t gpio_host_intr_handle;

const struct gpio_options *gpio_host_intr_options[GPIO_HOST_PIN_COUNT] = {};

static IRAM_ATTR void gpio_host_isr (void *arg)
{
  int core = cpu_ll_get_core_id();
  gpio_pins_t pins = gpio_host_intr_status(core);

  LOG_ISR_DEBUG("core=%d pins=" GPIO_PINS_FMT, core, GPIO_PINS_ARGS(pins));

  for (gpio_pin_t gpio = 0; gpio < GPIO_HOST_PIN_COUNT; gpio++) {
    if (pins & GPIO_PINS(gpio)) {
      const struct gpio_options *options = gpio_host_intr_options[gpio];

      if (options && (pins & options->interrupt_pins) && options->interrupt_func) {
        options->interrupt_func(pins & options->interrupt_pins, options->interrupt_arg);
      }
    }
  }

  gpio_host_intr_clear(pins);
}

int gpio_host_intr_init()
{
  esp_err_t err;

  if ((err = esp_intr_alloc(ETS_GPIO_INTR_SOURCE, GPIO_HOST_INTR_ALLOC_FLAGS, gpio_host_isr, NULL, &gpio_host_intr_handle))) {
    LOG_ERROR("esp_intr_alloc: %s", esp_err_to_name(err));
    return -1;
  } else {
    LOG_DEBUG("gpio_host_intr_handle=%p core=%d", gpio_host_intr_handle, esp_intr_get_cpu(gpio_host_intr_handle));
  }

  return 0;
}

void gpio_host_intr_setup(const struct gpio_options *options)
{
  for (gpio_pin_t gpio = 0; gpio < GPIO_HOST_PIN_COUNT; gpio++) {
    if (options->interrupt_pins & GPIO_PINS(gpio)) {
      LOG_DEBUG("[%d] -> %p", gpio, options);

      gpio_host_intr_options[gpio] = options;
    }
  }
}

int gpio_host_intr_core()
{
  return esp_intr_get_cpu(gpio_host_intr_handle);
}