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

  if ((err = i2s_out_pin_init(i2s_out))) {
    LOG_ERROR("i2s_out_pin_init");
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

static int i2s_out_write(struct i2s_out *i2s_out, const void *data, size_t size)
{
  int ret = 0;

  if (!xSemaphoreTakeRecursive(i2s_out->mutex, portMAX_DELAY)) {
    LOG_ERROR("xSemaphoreTakeRecursive");
    return -1;
  }

  while (size) {
    if ((ret = i2s_out_dma_write(i2s_out, data, size)) < 0) {
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

int i2s_out_write_serial16(struct i2s_out *i2s_out, const uint16_t data[], size_t count)
{
  return i2s_out_write(i2s_out, data, count * sizeof(*data));
}

int i2s_out_write_serial32(struct i2s_out *i2s_out, const uint32_t data[], size_t count)
{
  return i2s_out_write(i2s_out, data, count * sizeof(*data));
}

#if I2S_OUT_PARALLEL_SUPPORTED
  int i2s_out_write_parallel8x8(struct i2s_out *i2s_out, uint8_t data[8])
  {
    uint32_t buf[2];

    // 8x8-bit -> 2x32-bit
    i2s_out_transpose_parallel8x8(data, buf);

    return i2s_out_write(i2s_out, buf, sizeof(buf));
  }

  int i2s_out_write_parallel8x16(struct i2s_out *i2s_out, uint16_t *data, unsigned width)
  {
    int ret = 0;

    if (!xSemaphoreTakeRecursive(i2s_out->mutex, portMAX_DELAY)) {
      LOG_ERROR("xSemaphoreTakeRecursive");
      return -1;
    }

    for (unsigned index = 0; index < width; ) {
      void *ptr;
      size_t size = (width - index) * 16;

      // XXX: split write when misaligned on end of DMA buffer?
      if ((size = i2s_out_dma_buffer(i2s_out, &ptr, size)) < 16) {
        LOG_WARN("i2s_out_dma_buffer: DMA buffer full");
        ret = 1;
        goto error;
      }

      // transpose each 8x16-bit -> 4x32-bit block that fits
      uint32_t *buf = ptr;
      size_t len = 0;

      while (size >= 16) {
        i2s_out_transpose_parallel8x16(data, width, index++, buf);

        buf += 4;
        size -= 16;
        len += 16;
      }

      i2s_out_dma_commit(i2s_out, len);
    }

error:
    if (!xSemaphoreGiveRecursive(i2s_out->mutex)) {
      LOG_ERROR("xSemaphoreGiveRecursive");
    }

    return ret;

  }
#endif

int i2s_out_flush(struct i2s_out *i2s_out)
{
  int err = 0;

  if (!xSemaphoreTakeRecursive(i2s_out->mutex, portMAX_DELAY)) {
    LOG_ERROR("xSemaphoreTakeRecursive");
    return -1;
  }

  if (i2s_out_dma_pending(i2s_out)) {
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

  i2s_out_i2s_stop(i2s_out);
  i2s_out_pin_teardown(i2s_out);

  if (!xSemaphoreGiveRecursive(i2s_out->mutex)) {
    LOG_WARN("xSemaphoreGiveRecursive");
  }

  return err;
}

int i2s_out_teardown(struct i2s_out *i2s_out)
{
  if (!xSemaphoreTakeRecursive(i2s_out->mutex, portMAX_DELAY)) {
    LOG_ERROR("xSemaphoreTakeRecursive");
    return -1;
  }

  i2s_out_intr_teardown(i2s_out);
  i2s_out_dev_teardown(i2s_out);

  if (!xSemaphoreGiveRecursive(i2s_out->mutex)) {
    LOG_WARN("xSemaphoreGiveRecursive");
  }

  return 0;
}
