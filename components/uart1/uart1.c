#include <uart1.h>
#include "uart1.h"
#include <logging.h>

#include <stdlib.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

static int uart1_init(struct uart1 *uart1, struct uart1_options options)
{
  if (!(uart1->tx_buffer = xStreamBufferCreate(options.tx_buffer_size, 1))) {
    LOG_ERROR("xStreamBufferCreate");
    return -1;
  }

  return 0;
}

int uart1_new(struct uart1 **uart1p, struct uart1_options options)
{
  struct uart1 *uart1;
  int err;

  if (!(uart1 = calloc(1, sizeof(*uart1)))) {
    LOG_ERROR("calloc");
    return -1;
  }

  if ((err = uart1_init(uart1, options))) {
    LOG_ERROR("uart1_init");
    goto error;
  }

  taskENTER_CRITICAL();
  uart1_tx_setup(options);
  uart1_intr_setup(options);
  taskEXIT_CRITICAL();

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

ssize_t uart1_write(struct uart1 *uart1, const void *buf, size_t len)
{
  size_t write = 0;

  // fastpath via FIFO
  taskENTER_CRITICAL();
  write = uart1_tx_fast(uart1, buf, len);
  taskEXIT_CRITICAL();

  buf += write;
  len -= write;

  if (len > 0) {
    // slowpath via ISR
    taskENTER_CRITICAL();
    write += uart1_tx_slow(uart1, buf, len);
    taskEXIT_CRITICAL();
  }

  return write;
}
