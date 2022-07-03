#include "../i2s_out.h"

#include <logging.h>

// no-op, only one I2S dev which is hardcoded
int i2s_out_dev_setup(struct i2s_out *i2s_out, const struct i2s_out_options *options)
{
  if (options->dev_mutex) {
    LOG_DEBUG("take dev_mutex=%p", options->dev_mutex);

    if (!xSemaphoreTake(options->dev_mutex, portMAX_DELAY)) {
      LOG_ERROR("xSemaphoreTake");
      return -1;
    } else {
      i2s_out->dev_mutex = options->dev_mutex;

      LOG_DEBUG("have dev_mutex=%p", i2s_out->dev_mutex);
    }
  }

  return 0;
}

void i2s_out_dev_teardown(struct i2s_out *i2s_out)
{
  if (i2s_out->dev_mutex) {
    LOG_DEBUG("give dev_mutex=%p", i2s_out->dev_mutex);

    xSemaphoreGive(i2s_out->dev_mutex);

    i2s_out->dev_mutex = NULL;
  }
}
