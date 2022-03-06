#include "../i2s_out.h"

#include <esp_rom_gpio.h>
#include <hal/gpio_ll.h>
#include <soc/gpio_periph.h>

#include <logging.h>

static const uint8_t i2s_data_out_sig[I2S_PORT_MAX] = {
  [I2S_PORT_0]  = I2S0O_DATA_OUT23_IDX,
  [I2S_PORT_1]  = I2S1O_DATA_OUT23_IDX,
};

int i2s_out_pin_init(struct i2s_out *i2s_out)
{
  i2s_out->data_gpio = -1;

  return 0;
}

int i2s_out_pin_setup(struct i2s_out *i2s_out, struct i2s_out_options options)
{
  if (options.pin_mutex) {
    if (!xSemaphoreTake(options.pin_mutex, options.pin_timeout)) {
      LOG_ERROR("xSemaphoreTake");
      return -1;
    } else {
      i2s_out->pin_mutex = options.pin_mutex;
    }
  }

  i2s_out->data_gpio = options.data_gpio;

  LOG_DEBUG("data_gpio=%d", i2s_out->data_gpio);

  taskENTER_CRITICAL(&i2s_out->mux);

  gpio_ll_iomux_func_sel(GPIO_PIN_MUX_REG[i2s_out->data_gpio], PIN_FUNC_GPIO);
  gpio_ll_input_disable(&GPIO, i2s_out->data_gpio);
  gpio_ll_output_enable(&GPIO, i2s_out->data_gpio);

  esp_rom_gpio_connect_out_signal(i2s_out->data_gpio, i2s_data_out_sig[i2s_out->port], false, false);

  taskEXIT_CRITICAL(&i2s_out->mux);

  return 0;
}

void i2s_out_pin_teardown(struct i2s_out *i2s_out)
{
  LOG_DEBUG("");

  taskENTER_CRITICAL(&i2s_out->mux);

  // place output into a safe state
  if (i2s_out->data_gpio >= 0) {
    gpio_ll_output_disable(&GPIO, i2s_out->data_gpio);

    i2s_out->data_gpio = -1;
  }

  taskEXIT_CRITICAL(&i2s_out->mux);

  if (i2s_out->pin_mutex) {
    xSemaphoreGive(i2s_out->pin_mutex);

    i2s_out->pin_mutex = NULL;
  }
}
