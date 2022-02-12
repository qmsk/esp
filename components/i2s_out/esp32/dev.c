#include "../i2s_out.h"

#include <driver/periph_ctrl.h>

#include <logging.h>

static i2s_dev_t *i2s_dev[I2S_PORT_MAX] = {
  [I2S_PORT_0]  = &I2S0,
  [I2S_PORT_1]  = &I2S1,
};

static const periph_module_t i2s_periph_module[I2S_PORT_MAX] = {
  [I2S_PORT_0]  = PERIPH_I2S0_MODULE,
  [I2S_PORT_1]  = PERIPH_I2S1_MODULE,
};

int i2s_out_dev_setup(struct i2s_out *i2s_out, struct i2s_out_options options)
{
  if (options.dev_mutex) {
    LOG_DEBUG("take dev_mutex=%p", options.dev_mutex);

    if (!xSemaphoreTake(options.dev_mutex, portMAX_DELAY)) {
      LOG_ERROR("xSemaphoreTake");
      return -1;
    } else {
      i2s_out->dev_mutex = options.dev_mutex;

      LOG_DEBUG("have dev_mutex=%p", i2s_out->dev_mutex);
    }
  }

  if (!(i2s_out->dev = i2s_dev[i2s_out->port])) {
    LOG_ERROR("invalid i2s_dev[%d]", i2s_out->port);
    return -1;
  }

  LOG_DEBUG("port=%x",
    i2s_out->port
  );

  taskENTER_CRITICAL(&i2s_out->mux);

  periph_module_enable(i2s_periph_module[i2s_out->port]);

  // TODO: CONFIG_PM_ENABLE?

  taskEXIT_CRITICAL(&i2s_out->mux);

  return 0;
}

void i2s_out_dev_teardown(struct i2s_out *i2s_out)
{
  LOG_DEBUG("");

  taskENTER_CRITICAL(&i2s_out->mux);

  periph_module_disable(i2s_periph_module[i2s_out->port]);

  taskEXIT_CRITICAL(&i2s_out->mux);

  // release
  i2s_out->dev = NULL;

  if (i2s_out->dev_mutex) {
    LOG_DEBUG("give dev_mutex=%p", i2s_out->dev_mutex);

    xSemaphoreGive(i2s_out->dev_mutex);

    i2s_out->dev_mutex = NULL;
  }
}
