#include "i2s_out.h"

#include <logging.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <stdlib.h>

int i2s_out_init(struct i2s_out *i2s_out, i2s_port_t port)
{
  if (port < 0 || port >= I2S_PORT_MAX) {
    LOG_ERROR("invalid port=%x", port);
    return -1;
  } else {
    i2s_out->port = port;
  }

  if (!(i2s_out->mutex = xSemaphoreCreateRecursiveMutex())) {
    LOG_ERROR("xSemaphoreCreateRecursiveMutex");
    return -1;
  }

  return 0;
}

int i2s_out_new(struct i2s_out **i2s_outp, i2s_port_t port, size_t buffer_size)
{
  struct i2s_out *i2s_out = NULL;
  int err;

  if (!(i2s_out = calloc(1, sizeof(*i2s_out)))) {
    LOG_ERROR("calloc");
    return -1;
  }

  if ((err = i2s_out_init(i2s_out, port))) {
    LOG_ERROR("i2s_out_init");
    goto error;
  }

  if ((err = i2s_out_i2s_init(i2s_out))) {
    LOG_ERROR("i2s_out_i2s_init");
    goto error;
  }

  if ((err = i2s_out_dma_init(i2s_out, buffer_size))) {
    LOG_ERROR("i2s_out_dma_init");
    goto error;
  }

  *i2s_outp = i2s_out;

  return 0;

error:
  free(i2s_out->dma_eof_buf);
  free(i2s_out->dma_rx_buf);
  free(i2s_out->dma_rx_desc);
  free(i2s_out->dma_eof_desc);
  free(i2s_out);

  return err;
}

int i2s_out_open(struct i2s_out *i2s_out, struct i2s_out_options options)
{
  int err = 0;

  if (!xSemaphoreTakeRecursive(i2s_out->mutex, portMAX_DELAY)) {
    LOG_ERROR("xSemaphoreTakeRecursive");
    return -1;
  }

  if ((err = i2s_out_dev_setup(i2s_out, options))) {
    LOG_ERROR("i2s_out_dev_setup");
    goto error;
  }

  if ((err = i2s_out_dma_setup(i2s_out, options))) {
    LOG_ERROR("i2s_out_dma_setup");
    goto error;
  }

  if ((err = i2s_out_i2s_setup(i2s_out, options))) {
    LOG_ERROR("i2s_out_i2s_setup");
    goto error;
  }

  if ((err = i2s_out_intr_setup(i2s_out, options))) {
    LOG_ERROR("i2s_out_intr_setup");
    goto error;
  }

  if ((err = i2s_out_pin_setup(i2s_out, options))) {
    LOG_ERROR("i2s_out_pin_setup");
    goto error;
  }

  return 0;

error:
  xSemaphoreGiveRecursive(i2s_out->mutex);

  return err;
}

int i2s_out_write(struct i2s_out *i2s_out, void *data, size_t len)
{
  int ret;

  if (!xSemaphoreTakeRecursive(i2s_out->mutex, portMAX_DELAY)) {
    LOG_ERROR("xSemaphoreTakeRecursive");
    return -1;
  }

  ret = i2s_out_dma_write(i2s_out, data, len);

  if (!xSemaphoreGiveRecursive(i2s_out->mutex)) {
    LOG_ERROR("xSemaphoreGiveRecursive");
  }

  return ret;
}

int i2s_out_write_all(struct i2s_out *i2s_out, void *data, size_t len)
{
  int ret = 0;

  if (!xSemaphoreTakeRecursive(i2s_out->mutex, portMAX_DELAY)) {
    LOG_ERROR("xSemaphoreTakeRecursive");
    return -1;
  }

  while (len) {
    if ((ret = i2s_out_dma_write(i2s_out, data, len)) < 0) {
      LOG_ERROR("i2s_out_dma_write");
      break;
    } else if (!ret) {
      LOG_WARN("i2s_out_dma_write: TX buffer full");
      ret = 1;
      break;
    } else {
      len -= ret;
      ret = 0;
    }
  }

  if (!xSemaphoreGiveRecursive(i2s_out->mutex)) {
    LOG_ERROR("xSemaphoreGiveRecursive");
  }

  return ret;
}

int i2s_out_flush(struct i2s_out *i2s_out)
{
  int err = 0;

  if (!xSemaphoreTakeRecursive(i2s_out->mutex, portMAX_DELAY)) {
    LOG_ERROR("xSemaphoreTakeRecursive");
    return -1;
  }

  if (i2s_out_dma_ready(i2s_out)) {
    i2s_out_dma_start(i2s_out);
    i2s_out_i2s_start(i2s_out);
  }

  // wait for TX DMA EOF
  if ((err = i2s_out_dma_flush(i2s_out))) {
    LOG_ERROR("i2s_out_dma_flush");
    goto error;
  }

  // wait for I2S TX done
  if ((err = i2s_out_i2s_flush(i2s_out))) {
    LOG_ERROR("i2s_out_i2s_flush");
    goto error;
  }

error:
  if (!xSemaphoreGiveRecursive(i2s_out->mutex)) {
    LOG_WARN("xSemaphoreGiveRecursive");
  }

  return err;
}

int i2s_out_close(struct i2s_out *i2s_out)
{
  int err = i2s_out_flush(i2s_out);

  i2s_out_pin_teardown(i2s_out);
  i2s_out_intr_teardown(i2s_out);
  i2s_out_dev_teardown(i2s_out);

  if (!xSemaphoreGiveRecursive(i2s_out->mutex)) {
    LOG_WARN("xSemaphoreGiveRecursive");
  }

  return err;
}
