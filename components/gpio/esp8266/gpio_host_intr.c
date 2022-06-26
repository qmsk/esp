#include <gpio.h>
#include "../gpio.h"
#include "gpio_host.h"

#include <logging.h>

#include <freertos/FreeRTOS.h>
#include <rom/ets_sys.h>

const struct gpio_options *gpio_host_intr_options[16] = {};

static IRAM_ATTR void gpio_host_isr (void *arg)
{
  gpio_pins_t intr_pins = GPIO.status;

  LOG_ISR_DEBUG("intr_pins=" GPIO_PINS_FMT, GPIO_PINS_ARGS(intr_pins));

  for (gpio_pin_t gpio = 0; gpio < GPIO_HOST_PIN_COUNT; gpio++) {
    if (intr_pins & GPIO_PINS(gpio)) {
      const struct gpio_options *options = gpio_host_intr_options[gpio];
      gpio_pins_t pins;

      if (!options) {
        continue;
      }

      switch(options->type) {
        case GPIO_TYPE_HOST:
          pins = intr_pins & options->interrupt_pins;
          break;

        default:
          // external GPIO interrupt, unknown pins
          pins = options->interrupt_pins;
          break;
      }

      if (options->interrupt_func) {
        options->interrupt_func(pins, options->interrupt_arg);
      }
    }
  }

  GPIO.status_w1tc = intr_pins;
}

int gpio_host_intr_init()
{
  _xt_isr_attach(ETS_GPIO_INUM, gpio_host_isr, NULL);
  _xt_isr_unmask(1 << ETS_GPIO_INUM);

  return 0;
}

void gpio_host_intr_setup_pin(const struct gpio_options *options, gpio_pin_t gpio)
{
  if (gpio >= 16) {
    LOG_WARN("[%d] -> %p: invalid gpio_pin_t", gpio, options);
    return;
  }

  LOG_DEBUG("[%d] -> %p", gpio, options);

  gpio_host_intr_options[gpio] = options;
}
