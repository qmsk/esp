#include <gpio.h>
#include "../gpio.h"
#include "gpio_host.h"

#include <logging.h>

#include <freertos/FreeRTOS.h>
#include <rom/ets_sys.h>

const struct gpio_options *gpio_intr_options[16] = {};

static IRAM_ATTR void gpio_isr (void *arg)
{
  gpio_pins_t pins = GPIO.status;

  LOG_ISR_DEBUG("pins=" GPIO_PINS_FMT, GPIO_PINS_ARGS(pins));

  for (gpio_pin_t gpio = 0; gpio < GPIO_HOST_PIN_COUNT; gpio++) {
    const struct gpio_options *options;

    if ((pins & GPIO_PINS(gpio)) && (options = gpio_intr_options[gpio])) {
      switch(options->type) {
        case GPIO_TYPE_HOST:
          gpio_host_intr_handler(options, pins);
          break;

      #if GPIO_I2C_ENABLED
        case GPIO_TYPE_I2C:
          gpio_i2c_intr_handler(options, pins);
          break;
      #endif
      }
    }
  }

  GPIO.status_w1tc = pins;
}

int gpio_intr_init()
{
  _xt_isr_attach(ETS_GPIO_INUM, gpio_isr, NULL);
  _xt_isr_unmask(1 << ETS_GPIO_INUM);

  return 0;
}

void gpio_intr_setup_pin(const struct gpio_options *options, gpio_pin_t gpio)
{
  if (gpio >= 16) {
    LOG_WARN("[%d] -> %p: invalid gpio_pin_t", gpio, options);
    return;
  }

  LOG_DEBUG("[%d] -> %p", gpio, options);

  gpio_intr_options[gpio] = options;
}
