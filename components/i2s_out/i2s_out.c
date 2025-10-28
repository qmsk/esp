#include "i2s_out.h"
#include "transpose.h"

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

  if (!(i2s_out->event_group = xEventGroupCreate())) {
    LOG_ERROR("xEventGroupCreate");
    return -1;
  }

  #if CONFIG_IDF_TARGET_ESP32
    portMUX_INITIALIZE(&i2s_out->mux);
  #endif

  return 0;
}

int i2s_out_new(struct i2s_out **i2s_outp, i2s_port_t port, size_t buffer_size, size_t buffer_align, unsigned repeat_data_count)
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

  if ((err = i2s_out_dma_init(i2s_out, buffer_size, buffer_align, repeat_data_count))) {
    LOG_ERROR("i2s_out_dma_init");
    goto error;
  }

  if ((err = i2s_out_pin_init(i2s_out))) {
    LOG_ERROR("i2s_out_pin_init");
    goto error;
  }

  *i2s_outp = i2s_out;

  return 0;

error:
  i2s_out_dma_free(i2s_out);
  free(i2s_out);

  return err;
}

int i2s_out_open(struct i2s_out *i2s_out, const struct i2s_out_options *options, TickType_t timeout)
{
  int err = 0;

  if (!xSemaphoreTakeRecursive(i2s_out->mutex, timeout)) {
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

  i2s_out->setup = true;

  return 0;

error:
  xSemaphoreGiveRecursive(i2s_out->mutex);

  return err;
}

static int i2s_out_write(struct i2s_out *i2s_out, const void *data, size_t size, TickType_t timeout)
{
  int ret = 0;

  if (!xSemaphoreTakeRecursive(i2s_out->mutex, timeout)) {
    LOG_ERROR("xSemaphoreTakeRecursive");
    return -1;
  }

  if (!i2s_out->setup) {
    LOG_ERROR("setup");
    return -1;
  }

  while (size) {
    if ((ret = i2s_out_dma_write(i2s_out, data, size, timeout)) < 0) {
      LOG_ERROR("i2s_out_dma_write");
      break;
    } else if (!ret) {
      LOG_WARN("i2s_out_dma_write: TX buffer full");
      ret = 1;
      break;
    } else {
      size -= ret;
      data += ret;
      ret = 0;
    }
  }

  if (!xSemaphoreGiveRecursive(i2s_out->mutex)) {
    LOG_ERROR("xSemaphoreGiveRecursive");
  }

  return ret;
}

int i2s_out_write_serial16(struct i2s_out *i2s_out, const uint16_t data[], size_t count, TickType_t timeout)
{
  return i2s_out_write(i2s_out, data, count * sizeof(*data), timeout);
}

int i2s_out_write_serial32(struct i2s_out *i2s_out, const uint32_t *data, size_t count, TickType_t timeout)
{
  int ret = 0;

  if (!xSemaphoreTakeRecursive(i2s_out->mutex, timeout)) {
    LOG_ERROR("xSemaphoreTakeRecursive");
    return -1;
  }

  if (!i2s_out->setup) {
    LOG_ERROR("setup");
    return -1;
  }

  uint32_t (*buf)[1];
  unsigned index = 0;

  while (index < count) {
    // get DMA buffer for remaining blocks
    void *ptr;
    size_t len;

    if (!(len = i2s_out_dma_buffer(i2s_out, &ptr, count - index, sizeof(*buf), timeout))) {
      LOG_WARN("i2s_out_dma_buffer: DMA buffer full");
      ret = 1;
      goto error;
    }

    // transpose each 32-bit block -> 32-bit buffer that fits into the DMA buffer
    buf = ptr;

    for (int i = 0; i < len; i++) {
      LOG_DEBUG("index=%u: buf[%d]=%p ", index, i, buf[i]);

      i2s_out_transpose_serial32(data[index++], buf[i]);
    }

    i2s_out_dma_commit(i2s_out, len, sizeof(*buf));
  }

error:
  if (!xSemaphoreGiveRecursive(i2s_out->mutex)) {
    LOG_ERROR("xSemaphoreGiveRecursive");
  }

  return ret;
}

#if I2S_OUT_PARALLEL_SUPPORTED
  int i2s_out_write_parallel8x8(struct i2s_out *i2s_out, uint8_t *data, unsigned width, TickType_t timeout)
  {
    int ret = 0;

    if (!xSemaphoreTakeRecursive(i2s_out->mutex, timeout)) {
      LOG_ERROR("xSemaphoreTakeRecursive");
      return -1;
    }

    if (!i2s_out->setup) {
      LOG_ERROR("setup");
      return -1;
    }

    uint32_t (*buf)[2];
    unsigned index = 0;

    while (index < width) {
      // get DMA buffer for remaining blocks
      void *ptr;
      size_t count;

      if (!(count = i2s_out_dma_buffer(i2s_out, &ptr, width - index, sizeof(*buf), timeout))) {
        LOG_WARN("i2s_out_dma_buffer: DMA buffer full");
        ret = 1;
        goto error;
      }

      // transpose each 8-bit block -> 2x32-bit buffer that fits into the DMA buffer
      buf = ptr;

      for (int i = 0; i < count; i++) {
        LOG_DEBUG("index=%u: buf[%d]=%p ", index, i, buf[i]);

        i2s_out_transpose_parallel8x8(data, width, index++, buf[i]);
      }

      i2s_out_dma_commit(i2s_out, count, sizeof(*buf));
    }

error:
    if (!xSemaphoreGiveRecursive(i2s_out->mutex)) {
      LOG_ERROR("xSemaphoreGiveRecursive");
    }

    return ret;
  }

  int i2s_out_write_parallel8x16(struct i2s_out *i2s_out, uint16_t *data, unsigned width, TickType_t timeout)
  {
    int ret = 0;

    if (!xSemaphoreTakeRecursive(i2s_out->mutex, timeout)) {
      LOG_ERROR("xSemaphoreTakeRecursive");
      return -1;
    }

    if (!i2s_out->setup) {
      LOG_ERROR("setup");
      return -1;
    }

    uint32_t (*buf)[4];
    unsigned index = 0;

    while (index < width) {
      // get DMA buffer for remaining blocks
      void *ptr;
      size_t count;

      if (!(count = i2s_out_dma_buffer(i2s_out, &ptr, width - index, sizeof(*buf), timeout))) {
        LOG_WARN("i2s_out_dma_buffer: DMA buffer full");
        ret = 1;
        goto error;
      }

      // transpose each 8x16-bit block -> 4x32-bit buffer that fits into the DMA buffer
      buf = ptr;

      for (int i = 0; i < count; i++) {
        LOG_DEBUG("index=%u: buf[%d]=%p ", index, i, buf[i]);

        i2s_out_transpose_parallel8x16(data, width, index++, buf[i]);
      }

      i2s_out_dma_commit(i2s_out, count, sizeof(*buf));
    }

error:
    if (!xSemaphoreGiveRecursive(i2s_out->mutex)) {
      LOG_ERROR("xSemaphoreGiveRecursive");
    }

    return ret;
  }

  int i2s_out_write_parallel8x32(struct i2s_out *i2s_out, uint32_t *data, unsigned width, TickType_t timeout)
  {
    int ret = 0;

    if (!xSemaphoreTakeRecursive(i2s_out->mutex, timeout)) {
      LOG_ERROR("xSemaphoreTakeRecursive");
      return -1;
    }

    if (!i2s_out->setup) {
      LOG_ERROR("setup");
      return -1;
    }

    uint32_t (*buf)[8];
    unsigned index = 0;

    while (index < width) {
      // get DMA buffer for remaining blocks
      void *ptr;
      size_t count;

      if (!(count = i2s_out_dma_buffer(i2s_out, &ptr, width - index, sizeof(*buf), timeout))) {
        LOG_WARN("i2s_out_dma_buffer: DMA buffer full");
        ret = 1;
        goto error;
      }

      // transpose each 8x32-bit block -> 8x32-bit buffer that fits into the DMA buffer
      buf = ptr;

      for (int i = 0; i < count; i++) {
        LOG_DEBUG("index=%u: buf[%d]=%p ", index, i, buf[i]);

        i2s_out_transpose_parallel8x32(data, width, index++, buf[i]);
      }

      i2s_out_dma_commit(i2s_out, count, sizeof(*buf));
    }

error:
    if (!xSemaphoreGiveRecursive(i2s_out->mutex)) {
      LOG_ERROR("xSemaphoreGiveRecursive");
    }

    return ret;
  }
#endif

int i2s_out_repeat(struct i2s_out *i2s_out, unsigned count)
{
  int err = 0;

  if (!xSemaphoreTakeRecursive(i2s_out->mutex, portMAX_DELAY)) {
    LOG_ERROR("xSemaphoreTakeRecursive");
    return -1;
  }

  if (!i2s_out->setup) {
    LOG_ERROR("setup");
    return -1;
  }

  if ((err = i2s_out_dma_repeat(i2s_out, count))) {
    LOG_ERROR("i2s_out_dma_repeat");
    goto error;
  }

  if (!xSemaphoreGiveRecursive(i2s_out->mutex)) {
    LOG_WARN("xSemaphoreGiveRecursive");
  }

error:
  return err;
}

int i2s_out_wait(struct i2s_out *i2s_out, TickType_t timeout)
{
  int err = 0;

  if (!xSemaphoreTakeRecursive(i2s_out->mutex, timeout)) {
    LOG_ERROR("xSemaphoreTakeRecursive");
    return -1;
  }

  if (!i2s_out->setup) {
    LOG_ERROR("setup");
    return -1;
  }

  // wait for previous start() to complete?
  if (i2s_out_dma_running(i2s_out)) {
    if ((err = i2s_out_dma_flush(i2s_out, timeout))) {
      LOG_ERROR("i2s_out_dma_flush");
      goto error;
    }

    if ((err = i2s_out_i2s_flush(i2s_out, timeout))) {
      LOG_ERROR("i2s_out_i2s_flush");
      goto error;
    }

    i2s_out_dma_stop(i2s_out);
    i2s_out_i2s_stop(i2s_out);
  }

error:
  if (!xSemaphoreGiveRecursive(i2s_out->mutex)) {
    LOG_WARN("xSemaphoreGiveRecursive");
  }

  return err;

}

int i2s_out_start(struct i2s_out *i2s_out, TickType_t timeout)
{
  int err = 0;

  if (!xSemaphoreTakeRecursive(i2s_out->mutex, timeout)) {
    LOG_ERROR("xSemaphoreTakeRecursive");
    return -1;
  }

  if (!i2s_out->setup) {
    LOG_ERROR("setup");
    return -1;
  }

  // wait for previous start() to complete?
  if ((err = i2s_out_wait(i2s_out, timeout))) {
    goto error;
  }

  // have write()?
  if (i2s_out_dma_pending(i2s_out)) {
    if ((err = i2s_out_dma_start(i2s_out))) {
      LOG_ERROR("i2s_out_dma_start");
      goto error;
    }

    i2s_out_i2s_start(i2s_out);
  }

error:
  if (!xSemaphoreGiveRecursive(i2s_out->mutex)) {
    LOG_WARN("xSemaphoreGiveRecursive");
  }

  return err;
}

int i2s_out_flush(struct i2s_out *i2s_out, TickType_t timeout)
{
  int err = 0;

  if (!xSemaphoreTakeRecursive(i2s_out->mutex, timeout)) {
    LOG_ERROR("xSemaphoreTakeRecursive");
    return -1;
  }

  if (!i2s_out->setup) {
    LOG_ERROR("setup");
    return -1;
  }

  if ((err = i2s_out_start(i2s_out, timeout))) {
    goto error;
  }

  if ((err = i2s_out_wait(i2s_out, timeout))) {
    goto error;
  }

error:
  if (!xSemaphoreGiveRecursive(i2s_out->mutex)) {
    LOG_WARN("xSemaphoreGiveRecursive");
  }

  return err;
}

int i2s_out_close(struct i2s_out *i2s_out, TickType_t timeout)
{
  int err = 0;

  if (!xSemaphoreTakeRecursive(i2s_out->mutex, timeout)) {
    LOG_ERROR("xSemaphoreTakeRecursive");
    return -1;
  }

  if (!i2s_out->setup) {
    LOG_WARN("setup");
    err = 1;
    goto error;
  }

  err = i2s_out_flush(i2s_out, timeout);

  i2s_out_dma_stop(i2s_out);
  i2s_out_i2s_stop(i2s_out);
  i2s_out_pin_teardown(i2s_out);

  i2s_out->setup = false;

  if (!xSemaphoreGiveRecursive(i2s_out->mutex)) {
    LOG_WARN("xSemaphoreGiveRecursive");
  }

error:
  // from open()
  if (!xSemaphoreGiveRecursive(i2s_out->mutex)) {
    LOG_WARN("xSemaphoreGiveRecursive");
  }

  return err;
}

int i2s_out_teardown(struct i2s_out *i2s_out)
{
  int err = 0;

  if (!xSemaphoreTakeRecursive(i2s_out->mutex, portMAX_DELAY)) {
    LOG_ERROR("xSemaphoreTakeRecursive");
    return -1;
  }

  if (i2s_out->setup) {
    LOG_WARN("setup");
    err = 1;
    goto error;
  }

  i2s_out_intr_teardown(i2s_out);
  i2s_out_dev_teardown(i2s_out);

error:
  if (!xSemaphoreGiveRecursive(i2s_out->mutex)) {
    LOG_WARN("xSemaphoreGiveRecursive");
  }

  return err;
}
