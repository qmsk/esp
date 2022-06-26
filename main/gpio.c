#include "gpio.h"

#include <gpio.h>

#include <logging.h>

int init_gpio()
{
  int err;

  LOG_INFO("init gpio interrupts");

  if ((err = gpio_intr_init())) {
    LOG_ERROR("gpio_intr_init");
    return err;
  }

  return 0;
}
