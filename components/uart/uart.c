#include <uart.h>
#include "uart.h"

#include <logging.h>

#include <errno.h>
#include <stdlib.h>

static int uart_init(struct uart *uart, uart_port_t port)
{
  if ((uart->port & UART_PORT_MASK) >= UART_PORT_MAX) {
    LOG_ERROR("invalid port=%x", uart->port);
    return -1;
  }

  if (!(uart->rx_mutex = xSemaphoreCreateRecursiveMutex())) {
    LOG_ERROR("xSemaphoreCreateMutex");
    return -1;
  }
  if (!(uart->tx_mutex = xSemaphoreCreateRecursiveMutex())) {
    LOG_ERROR("xSemaphoreCreateMutex");
    return -1;
  }

  uart->port = port;

  return 0;
}

int uart_new(struct uart **uartp, uart_port_t port, size_t rx_buffer_size, size_t tx_buffer_size)
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

  *uartp = uart;

  return 0;

error:
  free(uart);

  return err;
}

int uart_setup(struct uart *uart, struct uart_options options)
{
  int err = -1;

  if (!xSemaphoreTakeRecursive(uart->rx_mutex, portMAX_DELAY)) {
    LOG_ERROR("xSemaphoreTakeRecursive");
    goto rx_error;
  }
  if (!xSemaphoreTakeRecursive(uart->tx_mutex, portMAX_DELAY)) {
    LOG_ERROR("xSemaphoreTakeRecursive");
    goto tx_error;
  }

  if ((err = uart_dev_setup(uart, options))) {
    LOG_ERROR("uart_dev_setup");
    goto dev_error;
  }

  if ((err = uart_pin_setup(uart, options))) {
    LOG_ERROR("uart_pin_setup");
    goto pin_error;
  }

  if ((err = uart_intr_setup(uart))) {
    LOG_ERROR("uart_intr_setup");
    goto intr_error;
  }

  // wait TX idle
  if ((err = uart_tx_flush(uart))) {
    LOG_ERROR("uart_tx_flush");
    goto error;
  }

  uart_rx_setup(uart, options);

  uart->read_timeout = options.read_timeout ? options.read_timeout : portMAX_DELAY;

  xSemaphoreGiveRecursive(uart->tx_mutex);
  xSemaphoreGiveRecursive(uart->rx_mutex);

  return 0;

error:
  uart_intr_teardown(uart);
intr_error:
  uart_pin_teardown(uart);
pin_error:
  uart_dev_teardown(uart);
dev_error:
  xSemaphoreGiveRecursive(uart->tx_mutex);
tx_error:
  xSemaphoreGiveRecursive(uart->rx_mutex);

rx_error:
  return err;
}

int uart_open(struct uart *uart, struct uart_options options)
{
  int err;

  if (!xSemaphoreTakeRecursive(uart->rx_mutex, portMAX_DELAY)) {
    LOG_ERROR("xSemaphoreTakeRecursive");
    err = -1;
    goto rx_error;
  }
  if (!xSemaphoreTakeRecursive(uart->tx_mutex, portMAX_DELAY)) {
    LOG_ERROR("xSemaphoreTakeRecursive");
    err = -1;
    goto tx_error;
  }

  if ((err = uart_setup(uart, options))) {
    LOG_ERROR("uart_setup");
    goto error;
  }

  // keep locked
  return 0;

error:
  xSemaphoreGiveRecursive(uart->tx_mutex);
tx_error:
  xSemaphoreGiveRecursive(uart->rx_mutex);

rx_error:
  return err;
}

/* RX */
int uart_open_rx(struct uart *uart)
{
  if (!xSemaphoreTakeRecursive(uart->rx_mutex, portMAX_DELAY)) {
    LOG_ERROR("xSemaphoreTakeRecursive");
    return -1;
  }

  if (!uart->dev) {
    LOG_ERROR("dev not setup");
    goto error;
  }

  return 0;

error:
  xSemaphoreGiveRecursive(uart->rx_mutex);

  return 1;
}

int uart_set_read_timeout(struct uart *uart, TickType_t timeout)
{
  int ret = 0;

  if (!xSemaphoreTakeRecursive(uart->rx_mutex, portMAX_DELAY)) {
    LOG_ERROR("xSemaphoreTakeRecursive");
    return -1;
  }

  uart->read_timeout = timeout;

  xSemaphoreGiveRecursive(uart->rx_mutex);

  return ret;
}

static int uart_acquire_rx(struct uart *uart)
{
  if (!xSemaphoreTakeRecursive(uart->rx_mutex, portMAX_DELAY)) {
    LOG_ERROR("xSemaphoreTakeRecursive");
    return -1;
  }

  if (!uart->dev) {
    LOG_ERROR("dev not setup");
    goto error;
  }

  return 0;

error:
  xSemaphoreGiveRecursive(uart->rx_mutex);

  return -1;
}

static void uart_release_rx(struct uart *uart)
{
  xSemaphoreGiveRecursive(uart->rx_mutex);
}

int uart_read(struct uart *uart, void *buf, size_t size)
{
  int ret = 0;
  bool read = false;

  if ((ret = uart_acquire_rx(uart))) {
    return ret;
  }

  while (!ret) {
    // report any error/special cases
    switch (uart_rx_event(uart)) {
      case UART_RX_NONE:
        if (read) {
          LOG_DEBUG("RX timeout");
          ret = 0;
          goto ret;
        }

        // block on RX buffer
        break;

      case UART_RX_DATA:
        // RX buffer has data available
        break;

      case UART_RX_OVERFLOW:
        LOG_DEBUG("RX overflow detected");
        ret = -EOVERFLOW;
        goto ret;

      case UART_RX_ERROR:
        LOG_DEBUG("RX error detected");
        ret = -EBADMSG;
        goto ret;

      case UART_RX_BREAK:
        LOG_DEBUG("RX break detected");
        ret = 0;
        goto ret;

      case UART_RX_BREAK_OVERFLOW:
        LOG_DEBUG("RX break + overflow detected");
        ret = -ESPIPE;
        goto ret;

      case UART_RX_ABORT:
        LOG_DEBUG("RX abort detected");
        ret = -EINTR;
        goto ret;

      default:
        LOG_ERROR("unknown event");
        ret = -EINVAL;
        goto ret;
    }

    ret = uart_rx_read(uart, buf, size, uart->read_timeout);
    read = true;
  }

ret:
  uart_release_rx(uart);

  return ret;
}

int uart_abort_read(struct uart *uart)
{
  if (!uart->dev) {
    LOG_ERROR("dev not setup");
    return -1;
  }

  uart_rx_abort(uart);

  return 0;
}

int uart_close_rx(struct uart *uart)
{
  if (!xSemaphoreGiveRecursive(uart->rx_mutex)) {
    LOG_ERROR("xSemaphoreGiveRecursive");
    return -1;
  }

  return 0;
}

/* TX */
int uart_open_tx(struct uart *uart)
{
  if (!xSemaphoreTakeRecursive(uart->tx_mutex, portMAX_DELAY)) {
    LOG_ERROR("xSemaphoreTakeRecursive");
    return -1;
  }

  if (!uart->dev) {
    LOG_ERROR("dev not setup");
    goto error;
  }

  return 0;

error:
  xSemaphoreGiveRecursive(uart->tx_mutex);

  return 1;
}

static int uart_acquire_tx(struct uart *uart)
{
  if (!xSemaphoreTakeRecursive(uart->tx_mutex, portMAX_DELAY)) {
    LOG_ERROR("xSemaphoreTakeRecursive");
    return -1;
  }

  if (!uart->dev) {
    LOG_ERROR("dev not setup");
    goto error;
  }

  return 0;

error:
  xSemaphoreGiveRecursive(uart->tx_mutex);

  return -1;
}

static void uart_release_tx(struct uart *uart)
{
  xSemaphoreGiveRecursive(uart->tx_mutex);
}

int uart_putc(struct uart *uart, int ch)
{
  int ret;

  if ((ret = uart_acquire_tx(uart))) {
    return ret;
  }

  LOG_DEBUG("ch=%#02x", ch);

  if ((ret = uart_tx_one(uart, ch))) {
    goto error;
  } else {
    ret = ch;
  }

error:
  uart_release_tx(uart);

  return ret;
}

ssize_t uart_write(struct uart *uart, const void *buf, size_t len)
{
  size_t write = 0;
  int err;

  if ((err = uart_acquire_tx(uart))) {
    return err;
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

  uart_release_tx(uart);

  return write;
}

ssize_t uart_write_all(struct uart *uart, const void *buf, size_t len)
{
  size_t write;
  int err;

  if ((err = uart_acquire_tx(uart))) {
    return err;
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

  uart_release_tx(uart);

  return 0;
}

ssize_t uart_write_buffered(struct uart *uart, const void *buf, size_t len)
{
  size_t write;
  int err;

  if ((err = uart_acquire_tx(uart))) {
    return err;
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

  uart_release_tx(uart);

  return 0;
}

int uart_flush_write(struct uart *uart)
{
  int err;

  if ((err = uart_acquire_tx(uart))) {
    return err;
  }

  err = uart_tx_flush(uart);

  uart_release_tx(uart);

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

  if ((err = uart_acquire_tx(uart))) {
    return err;
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
uart_release_tx(uart);

  return err;
}

int uart_mark(struct uart *uart, unsigned mark_us)
{
  int err;

  if ((err = uart_acquire_tx(uart))) {
    return err;
  }

  LOG_DEBUG("mark_us=%u", mark_us);

  if ((err = uart_tx_flush(uart))) {
    LOG_ERROR("uart_tx_flush");
    goto error;
  }

  uart_wait(mark_us);

  LOG_DEBUG("done");

error:
  uart_release_tx(uart);

  return err;
}

int uart_close_tx(struct uart *uart)
{
  int err;

  if ((err = uart_acquire_tx(uart))) {
    return err;
  }

  if ((err = uart_tx_flush(uart))) {
    LOG_ERROR("uart_tx_flush");
    goto error;
  }

error:
  uart_release_tx(uart);

  if (!xSemaphoreGiveRecursive(uart->tx_mutex)) {
    LOG_ERROR("xSemaphoreGiveRecursive");
  }

  return err;
}

/* setup/open */
int uart_close(struct uart *uart)
{
  int err;

  if ((err = uart_acquire_tx(uart))) {
    return err;
  }

  if ((err = uart_tx_flush(uart))) {
    LOG_ERROR("uart_tx_flush");
    goto error;
  }

error:
  uart_release_tx(uart);

  if (!xSemaphoreGiveRecursive(uart->tx_mutex)) {
    LOG_ERROR("xSemaphoreGiveRecursive");
  }
  if (!xSemaphoreGiveRecursive(uart->rx_mutex)) {
    LOG_ERROR("xSemaphoreGiveRecursive");
  }

  return err;
}

int uart_teardown(struct uart *uart)
{
  int err = -1;

  if (!xSemaphoreTakeRecursive(uart->rx_mutex, portMAX_DELAY)) {
    LOG_ERROR("xSemaphoreTakeRecursive");
    goto rx_error;
  }
  if (!xSemaphoreTakeRecursive(uart->tx_mutex, portMAX_DELAY)) {
    LOG_ERROR("xSemaphoreTakeRecursive");
    goto tx_error;
  }

  if (!uart->dev) {
    LOG_ERROR("dev not setup");
    goto error;
  }

  err = 0;

  uart_intr_teardown(uart);
  uart_pin_teardown(uart);
  uart_dev_teardown(uart);

error:
  xSemaphoreGiveRecursive(uart->tx_mutex);
tx_error:
  xSemaphoreGiveRecursive(uart->rx_mutex);

rx_error:
  return err;
}
