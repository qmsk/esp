#include <uart.h>
#include "../uart.h"
#include "rx.h"

#include <logging.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#define UART_RX_TIMEOUT 1 // immediately
#define UART_RX_BUFFERED (UART_RXBUF_SIZE) // half of FIFO size, matching RX buffer on ISR stack

int uart_rx_init(struct uart *uart, size_t rx_buffer_size)
{
  LOG_DEBUG("rx_buffer_size=%u", rx_buffer_size);

  if (!(uart->rx_buffer = xStreamBufferCreate(rx_buffer_size, 1))) {
    LOG_ERROR("xStreamBufferCreate");
    return -1;
  }

  return 0;
}

int uart_rx_setup(struct uart *uart, struct uart_options options)
{
  int reset;
  taskENTER_CRITICAL();

  uart_rx_fifo_reset(uart->dev);

  reset = xStreamBufferReset(uart->rx_buffer);

  uart->rx_overflow = false;
  uart->rx_break = false;
  uart->rx_error = false;
  uart->rx_abort = false;

  if (options.rx_buffered) {
    uart_rx_conf_full_threshold(uart->dev, options.rx_buffered);
  } else {
    uart_rx_conf_full_threshold(uart->dev, UART_RX_BUFFERED); // default
  }

  if (options.rx_timeout) {
    uart_rx_conf_tout_threshold(uart->dev, options.rx_timeout, true);
  } else {
    uart_rx_conf_tout_threshold(uart->dev, UART_RX_TIMEOUT, true); // default
  }

  uart_rx_intr_enable(uart->dev);

  taskEXIT_CRITICAL();

  if (!reset) {
    LOG_WARN("xStreamBufferReset: RX buffer busy");
  }

  return 0;
}

enum uart_rx_event uart_rx_event(struct uart *uart)
{
  enum uart_rx_event event;

  taskENTER_CRITICAL();

  if (xStreamBufferBytesAvailable(uart->rx_buffer)) {
    event = UART_RX_DATA;

  } else if (uart->rx_overflow && uart->rx_break) {
    uart->rx_overflow = false;
    uart->rx_break = false;

    event = UART_RX_BREAK_OVERFLOW;

  } else if (uart->rx_overflow) {
    uart->rx_overflow = false;

    event = UART_RX_OVERFLOW;

  } else if (uart->rx_error) {
    uart->rx_error = false;

    event = UART_RX_ERROR;

  } else if (uart->rx_break) {
    uart->rx_break = false;

    event = UART_RX_BREAK;

  } else if (uart->rx_abort) {
    uart->rx_abort = false;

    event = UART_RX_ABORT;

  } else {
    event = UART_RX_NONE;

    // RX buffer emptied and flags cleared, resume copying from RX FIFO if disabled
    uart_rx_intr_resume(uart->dev);
  }

  taskEXIT_CRITICAL();

  return event;
}

int uart_rx_read(struct uart *uart, void *buf, size_t size, TickType_t timeout)
{
  // block on ISR RX FIFO -> buffer copy
  return xStreamBufferReceive(uart->rx_buffer, buf, size, timeout);
}

void uart_rx_abort(struct uart *uart)
{
  taskENTER_CRITICAL();

  uart_rx_intr_pause(uart->dev);

  uart->rx_abort = true;

  // wakeup uart_rx_read()
  vStreamBufferSendCompleted(uart->rx_buffer);

  taskEXIT_CRITICAL();
}
