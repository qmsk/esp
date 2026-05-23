#include "../i2s_out.h"
#include "dma.h"
#include "intr.h"

#include <logging.h>

#include <esp_err.h>
#include <esp_intr_alloc.h>
#include <soc/soc.h>


// use a non-shared, IRAM-safe intr
#define I2S_INTR_ALLOC_FLAGS (ESP_INTR_FLAG_IRAM)

static const int i2s_irq[I2S_PORT_MAX] = {
  [I2S_PORT_0]  = ETS_I2S0_INTR_SOURCE,
  [I2S_PORT_1]  = ETS_I2S1_INTR_SOURCE,
};

int i2s_out_intr_setup(struct i2s_out *i2s_out, const struct i2s_out_options *options)
{
  esp_err_t err = 0;

  if (i2s_out->intr) {
    return 0;
  }

  taskENTER_CRITICAL(&i2s_out->mux);

  i2s_intr_disable_all(i2s_out->dev);
  err = esp_intr_alloc(i2s_irq[i2s_out->port], I2S_INTR_ALLOC_FLAGS, i2s_intr_handler, i2s_out, &i2s_out->intr);

  taskEXIT_CRITICAL(&i2s_out->mux);

  if (err) {
    LOG_ERROR("esp_intr_alloc: %s", esp_err_to_name(err));
    return -1;
  } else {
    LOG_DEBUG("intr=%p", i2s_out->intr);
  }

  return 0;
}

void i2s_out_intr_teardown(struct i2s_out *i2s_out)
{
  esp_err_t err;

  LOG_DEBUG("intr=%p", i2s_out->intr);

  taskENTER_CRITICAL(&i2s_out->mux);

  err = esp_intr_free(i2s_out->intr);

  i2s_out->intr = NULL;

  taskEXIT_CRITICAL(&i2s_out->mux);

  if (err) {
    LOG_WARN("esp_intr_free: %s", esp_err_to_name(err));
  }
}
