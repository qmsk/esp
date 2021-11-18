#include <uart1.h>
#include "uart1.h"
#include <logging.h>

#include <stdlib.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <rom/ets_sys.h>

static int uart1_init(struct uart1 *uart1, size_t tx_buffer_size)
{
  LOG_DEBUG("tx_buffer_size=%u", tx_buffer_size);

  if (!(uart1->mutex = xSemaphoreCreateMutex())) {
    LOG_ERROR("xSemaphoreCreateMutex");
  }

  if (!(uart1->tx_buffer = xStreamBufferCreate(tx_buffer_size, 1))) {
    LOG_ERROR("xStreamBufferCreate");
    return -1;
  }

  return 0;
}

int uart1_new(struct uart1 **uart1p, struct uart1_options options, size_t tx_buffer_size)
{
  struct uart1 *uart1;
  int err;

  if (!(uart1 = calloc(1, sizeof(*uart1)))) {
    LOG_ERROR("calloc");
    return -1;
  }

  if ((err = uart1_init(uart1, tx_buffer_size))) {
    LOG_ERROR("uart1_init");
    goto error;
  }

  uart1_tx_setup(options);
  uart1_intr_setup(options);

  if ((err = uart1_intr_start(uart1))) {
    LOG_ERROR("uart1_intr_start");
    goto error;
  }

  *uart1p = uart1;

  return 0;

error:
  free(uart1);

  return err;
}

int uart1_open(struct uart1 *uart1, struct uart1_options options)
{
  int err;

  if (!xSemaphoreTake(uart1->mutex, portMAX_DELAY)) {
    LOG_ERROR("xSemaphoreTake");
    return -1;
  }

  if ((err = uart1_tx_flush(uart1))) {
    LOG_ERROR("uart1_tx_flush");
    return err;
  }

  uart1_tx_setup(options);

  return 0;
}

int uart1_putc(struct uart1 *uart1, int ch)
{
  int err;

  LOG_DEBUG("ch=%#02x", ch);

  if ((err = uart1_tx_one(uart1, ch))) {
    return -1;
  }

  return ch;
}

ssize_t uart1_write(struct uart1 *uart1, const void *buf, size_t len)
{
  size_t write = 0;

  // fastpath via FIFO queue or TX buffer
  write = uart1_tx_fast(uart1, buf, len);

  LOG_DEBUG("tx fast len=%u: write=%u", len, write);

  buf += write;
  len -= write;

  if (!write) {
    // blocking slowpath via buffer + ISR
    write = uart1_tx_slow(uart1, buf, len);

    LOG_DEBUG("tx slow len=%u: write=%u", len, write);

    buf += write;
    len -= write;
  }

  return write;
}

ssize_t uart1_write_all(struct uart1 *uart1, const void *buf, size_t len)
{
  size_t write;

  while (len > 0) {
    // fastpath via FIFO queue or TX buffer
    write = uart1_tx_fast(uart1, buf, len);

    LOG_DEBUG("tx fast len=%u: write=%u", len, write);

    buf += write;
    len -= write;

    if (len > 0) {
      // blocking slowpath via buffer + ISR
      write = uart1_tx_slow(uart1, buf, len);

      LOG_DEBUG("tx slow len=%u: write=%u", len, write);

      buf += write;
      len -= write;
    }
  }
  return 0;
}

ssize_t uart1_write_buffered(struct uart1 *uart1, const void *buf, size_t len)
{
  size_t write;

  while (len > 0) {
    // fastpath via TX buffer, without interrupts
    write = uart1_tx_buf(uart1, buf, len);

    LOG_DEBUG("tx buf len=%u: write=%u", len, write);

    buf += write;
    len -= write;

    if (len > 0) {
      // blocking slowpath via buffer + ISR
      write = uart1_tx_slow(uart1, buf, len);

      LOG_DEBUG("tx slow len=%u: write=%u", len, write);

      buf += write;
      len -= write;
    }
  }

  return 0;
}

int uart1_flush(struct uart1 *uart1)
{
  return uart1_tx_flush(uart1);
}

static inline void uart1_wait(unsigned us)
{
  // ESP8266_RTOS_SDK timers are shit:
  // * FreeRTOS timers only do 10ms ticks
  // * esp_timer is just a wrapper for FreeRTOS timers
  // * os_timer only does msec
  // * there's no shared timer implementation for FRC1/2
  // * there's no FRC2?
  // so we just busyloop, 'cause that's what everybody else does. It's really dumb.
  ets_delay_us(us);
}

int uart1_break(struct uart1 *uart1, unsigned break_us, unsigned mark_us)
{
  int err;

  LOG_DEBUG("break_us=%u mark_us=%u", break_us, mark_us);

  uart1_tx_break(uart1);
  if ((err = uart1_tx_flush(uart1))) {
    LOG_ERROR("uart1_tx_flush");
    uart1_tx_mark(uart1);
    return err;
  }
  uart1_wait(break_us);

  uart1_tx_mark(uart1);
  uart1_wait(mark_us);

  LOG_DEBUG("done");

  return 0;
}

int uart1_mark(struct uart1 *uart1, unsigned mark_us)
{
  int err;

  LOG_DEBUG("mark_us=%u", mark_us);

  if ((err = uart1_tx_flush(uart1))) {
    LOG_ERROR("uart1_tx_flush");
    return err;
  }

  uart1_wait(mark_us);

  LOG_DEBUG("done");

  return 0;
}

int uart1_close(struct uart1 *uart1)
{
  int err;

  if ((err = uart1_tx_flush(uart1))) {
    LOG_ERROR("uart1_tx_flush");
    return err;
  }

  if (!xSemaphoreGive(uart1->mutex)) {
    LOG_ERROR("xSemaphoreGive");
    return -1;
  }

  return 0;
}
