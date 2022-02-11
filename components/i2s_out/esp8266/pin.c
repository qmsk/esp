#include "../i2s_out.h"
#include "pin.h"

#include <logging.h>

int i2s_out_pin_setup(struct i2s_out *i2s_out, struct i2s_out_options options)
{
  if (options.pin_mutex) {
    if (!xSemaphoreTake(options.pin_mutex, portMAX_DELAY)) {
      LOG_ERROR("xSemaphoreTake");
      return -1;
    } else {
      i2s_out->pin_mutex = options.pin_mutex;
    }
  }

  taskENTER_CRITICAL();

  // configure data out pin
  IDEMPOTENT_PIN_FUNC_SELECT(PERIPHS_IO_MUX_U0RXD_U, FUNC_I2SO_DATA);

  taskEXIT_CRITICAL();

  return 0;
}

void i2s_out_pin_teardown(struct i2s_out *i2s_out)
{
  if (i2s_out->pin_mutex) {
    xSemaphoreGive(i2s_out->pin_mutex);

    i2s_out->pin_mutex = NULL;
  }
}
