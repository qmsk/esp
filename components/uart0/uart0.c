#include <uart0.h>
#include "uart0.h"
#include <logging.h>

#include <stdlib.h>

static int uart0_init(struct uart0 *uart0)
{
  if (!(uart0->mutex = xSemaphoreCreateRecursiveMutex())) {
    LOG_ERROR("xSemaphoreCreateMutex");
  }

  return 0;
}

int uart0_new(struct uart0 **uart0p, size_t rx_buffer_size)
{
  struct uart0 *uart0;
  int err;

  if (!(uart0 = calloc(1, sizeof(*uart0)))) {
    LOG_ERROR("calloc");
    return -1;
  }

  if ((err = uart0_init(uart0))) {
    LOG_ERROR("uart0_init");
    goto error;
  }


  if ((err = uart0_rx_init(uart0, rx_buffer_size))) {
    LOG_ERROR("uart0_rx_init");
    goto error;
  }

  if ((err = uart0_intr_init(uart0))) {
    LOG_ERROR("uart0_intr_init");
    goto error;
  }

  *uart0p = uart0;

  return 0;

error:
  free(uart0);

  return err;
}

int uart0_open(struct uart0 *uart0, struct uart0_options options)
{
  if (!xSemaphoreTakeRecursive(uart0->mutex, portMAX_DELAY)) {
    LOG_ERROR("xSemaphoreTakeRecursive");
    return -1;
  }

  uart0_setup(options);
  uart0_intr_setup(options);
  uart0_rx_setup(uart0);

  return 0;
}

int uart0_read(struct uart0 *uart0, void *buf, size_t size)
{
  int ret;

  if (!xSemaphoreTakeRecursive(uart0->mutex, portMAX_DELAY)) {
    LOG_ERROR("xSemaphoreTakeRecursive");
    return -1;
  }

  ret = uart0_rx_read(uart0, buf, size);

  xSemaphoreGiveRecursive(uart0->mutex);

  return ret;
}

int uart0_close(struct uart0 *uart0)
{
  if (!xSemaphoreGiveRecursive(uart0->mutex)) {
    LOG_ERROR("xSemaphoreGiveRecursive");
    return -1;
  }

  return 0;
}
