#include <uart.h>
#include "uart.h"

#include <logging.h>

#include <stdlib.h>

static uart_dev_t *uart_dev[UART_MAX] = {
  [UART_0]  = &uart0,
  [UART_1]  = &uart1,
};

static int uart_init(struct uart *uart, enum uart_port port)
{
  if ((port & UART_PORT_MASK) >= UART_MAX) {
    LOG_ERROR("invalid port=%x", port);
    return -1;
  }

  if (!uart_dev[port & UART_PORT_MASK]) {
    LOG_ERROR("invalid uart_dev[%d]", (port & UART_PORT_MASK));
    return -1;
  }

  uart->port = port;
  uart->dev = uart_dev[port & UART_PORT_MASK];

  if (!(uart->mutex = xSemaphoreCreateRecursiveMutex())) {
    LOG_ERROR("xSemaphoreCreateMutex");
    return -1;
  }

  return 0;
}

int uart_new(struct uart **uartp, enum uart_port port, size_t rx_buffer_size, size_t tx_buffer_size)
{
  struct uart *uart;
  int err;

  if (!(uart = calloc(1, sizeof(*uart)))) {
    LOG_ERROR("calloc");
    return -1;
  }

  if ((err = uart_init(uart, port))) {
    LOG_ERROR("uart_init");
    goto error;
  }

  if ((err = uart_rx_init(uart, rx_buffer_size))) {
    LOG_ERROR("uart_rx_init");
    goto error;
  }

  if ((err = uart_tx_init(uart, tx_buffer_size))) {
    LOG_ERROR("uart_tx_init");
    goto error;
  }

  if ((err = uart_intr_init(uart))) {
    LOG_ERROR("uart_intr_init");
    goto error;
  }

  *uartp = uart;

  return 0;

error:
  free(uart);

  return err;
}

int uart_open(struct uart *uart, struct uart_options options)
{
  int err;

  if (!xSemaphoreTakeRecursive(uart->mutex, portMAX_DELAY)) {
    LOG_ERROR("xSemaphoreTakeRecursive");
    return -1;
  }

  if ((err = uart_tx_flush(uart))) {
    LOG_ERROR("uart_tx_flush");
    goto error;
  }

  uart_setup(uart, options);
  uart_rx_setup(uart, options);

  return 0;

error:
  xSemaphoreGiveRecursive(uart->mutex);

  return err;
}

int uart_read(struct uart *uart, void *buf, size_t size)
{
  int ret = 0;

  if (!xSemaphoreTakeRecursive(uart->mutex, portMAX_DELAY)) {
    LOG_ERROR("xSemaphoreTakeRecursive");
    return -1;
  }

  while (!ret) {
    // report any error/special cases
    switch (uart_rx_event(uart)) {
      case UART_RX_DATA:
        // RX buffer has data available
        break;

      case UART_RX_OVERFLOW:
        LOG_WARN("RX overflow detected");
        ret = -1;
        goto ret;

      case UART_RX_ERROR:
        LOG_WARN("RX error detected");
        ret = -1;
        goto ret;

      case UART_RX_BREAK:
        LOG_DEBUG("RX break detected");
        ret = 0;
        goto ret;

      case UART_RX_NONE:
        // block on RX buffer
        break;

      default:
        LOG_ERROR("unknown event");
        ret = -1;
        goto ret;
    }

    ret = uart_rx_read(uart, buf, size);
  }

ret:
  xSemaphoreGiveRecursive(uart->mutex);

  return ret;
}

int uart_putc(struct uart *uart, int ch)
{
  int ret;

  if (!xSemaphoreTakeRecursive(uart->mutex, 0)) {
    LOG_ERROR("xSemaphoreTakeRecursive: busy");
    return -1;
  }

  LOG_DEBUG("ch=%#02x", ch);

  if ((ret = uart_tx_one(uart, ch))) {
    goto error;
  } else {
    ret = ch;
  }

error:
  xSemaphoreGiveRecursive(uart->mutex);

  return ret;
}

ssize_t uart_write(struct uart *uart, const void *buf, size_t len)
{
  size_t write = 0;

  if (!xSemaphoreTakeRecursive(uart->mutex, 0)) {
    LOG_ERROR("xSemaphoreTakeRecursive: busy");
    return -1;
  }

  // fastpath via FIFO queue or TX buffer
  write = uart_tx_fast(uart, buf, len);

  LOG_DEBUG("tx fast len=%u: write=%u", len, write);

  buf += write;
  len -= write;

  if (!write) {
    // blocking slowpath via buffer + ISR
    write = uart_tx_slow(uart, buf, len);

    LOG_DEBUG("tx slow len=%u: write=%u", len, write);

    buf += write;
    len -= write;
  }

  xSemaphoreGiveRecursive(uart->mutex);

  return write;
}

ssize_t uart_write_all(struct uart *uart, const void *buf, size_t len)
{
  size_t write;

  if (!xSemaphoreTakeRecursive(uart->mutex, 0)) {
    LOG_ERROR("xSemaphoreTakeRecursive: busy");
    return -1;
  }

  while (len > 0) {
    // fastpath via FIFO queue or TX buffer
    write = uart_tx_fast(uart, buf, len);

    LOG_DEBUG("tx fast len=%u: write=%u", len, write);

    buf += write;
    len -= write;

    if (len > 0) {
      // blocking slowpath via buffer + ISR
      write = uart_tx_slow(uart, buf, len);

      LOG_DEBUG("tx slow len=%u: write=%u", len, write);

      buf += write;
      len -= write;
    }
  }

  xSemaphoreGiveRecursive(uart->mutex);

  return 0;
}

ssize_t uart_write_buffered(struct uart *uart, const void *buf, size_t len)
{
  size_t write;

  if (!xSemaphoreTakeRecursive(uart->mutex, 0)) {
    LOG_ERROR("xSemaphoreTakeRecursive: busy");
    return -1;
  }

  while (len > 0) {
    // fastpath via TX buffer, without interrupts
    write = uart_tx_buffered(uart, buf, len);

    LOG_DEBUG("tx buf len=%u: write=%u", len, write);

    buf += write;
    len -= write;

    if (len > 0) {
      // blocking slowpath via buffer + ISR
      write = uart_tx_slow(uart, buf, len);

      LOG_DEBUG("tx slow len=%u: write=%u", len, write);

      buf += write;
      len -= write;
    }
  }

  xSemaphoreGiveRecursive(uart->mutex);

  return 0;
}

int uart_flush_write(struct uart *uart)
{
  int err;

  if (!xSemaphoreTakeRecursive(uart->mutex, 0)) {
    LOG_ERROR("xSemaphoreTakeRecursive: busy");
    return -1;
  }

  err = uart_tx_flush(uart);

  xSemaphoreGiveRecursive(uart->mutex);

  return err;
}

static inline void uart_wait(unsigned us)
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

int uart_break(struct uart *uart, unsigned break_us, unsigned mark_us)
{
  int err;

  if (!xSemaphoreTakeRecursive(uart->mutex, 0)) {
    LOG_ERROR("xSemaphoreTakeRecursive: busy");
    return -1;
  }

  LOG_DEBUG("break_us=%u mark_us=%u", break_us, mark_us);

  uart_tx_break(uart);
  if ((err = uart_tx_flush(uart))) {
    LOG_ERROR("uart_tx_flush");
    uart_tx_mark(uart);
    goto error;
  }
  uart_wait(break_us);

  uart_tx_mark(uart);
  uart_wait(mark_us);

  LOG_DEBUG("done");

error:
  xSemaphoreGiveRecursive(uart->mutex);

  return err;
}

int uart_mark(struct uart *uart, unsigned mark_us)
{
  int err;

  if (!xSemaphoreTakeRecursive(uart->mutex, 0)) {
    LOG_ERROR("xSemaphoreTakeRecursive: busy");
    return -1;
  }

  LOG_DEBUG("mark_us=%u", mark_us);

  if ((err = uart_tx_flush(uart))) {
    LOG_ERROR("uart_tx_flush");
    goto error;
  }

  uart_wait(mark_us);

  LOG_DEBUG("done");

error:
  xSemaphoreGiveRecursive(uart->mutex);

  return err;
}

int uart_close(struct uart *uart)
{
  int err;

  if ((err = uart_tx_flush(uart))) {
    LOG_ERROR("uart_tx_flush");
    goto error;
  }

error:
  if (!xSemaphoreGiveRecursive(uart->mutex)) {
    LOG_ERROR("xSemaphoreGiveRecursive");
    return -1;
  }

  return 0;
}
