#include <uart.h>
#include "../uart.h"
#include "rx.h"

#include <logging.h>

#include <hal/uart_ll.h>

#define UART_RX_TOUT_DEFAULT (1) // immediate

int uart_rx_init(struct uart *uart, size_t rx_buffer_size)
{
  LOG_DEBUG("rx_buffer_size=%u", rx_buffer_size);

  if (rx_buffer_size == 0) {
    uart->rx_buffer = NULL;

  } else if (!(uart->rx_buffer = xStreamBufferCreate(rx_buffer_size, 1))) {
    LOG_ERROR("xStreamBufferCreate");
    return -1;
  }

  return 0;
}

static uint8_t uart_symbol_bits(struct uart_options options)
{
  uint8_t bits = 0;

  // start bit
  bits += 1;

  switch(options.data_bits) {
    case UART_DATA_5_BITS:
      bits += 5;
      break;

    case UART_DATA_6_BITS:
      bits += 6;
      break;

    case UART_DATA_7_BITS:
      bits += 7;
      break;

    case UART_DATA_8_BITS:
      bits += 8;
      break;

    default:
      break;
  }

  switch (options.parity_bits) {
    case UART_PARITY_DISABLE:
      break;

    case UART_PARITY_ODD:
    case UART_PARITY_EVEN:
      bits += 1;
      break;
  }

  switch (options.stop_bits) {
    case UART_STOP_BITS_1:
      bits += 1;
      break;

    case UART_STOP_BITS_1_5:
    case UART_STOP_BITS_2:
      bits += 2;
      break;

    default:
      break;
  }

  return bits;
}

int uart_rx_setup(struct uart *uart, struct uart_options options)
{
  int reset;

  LOG_DEBUG("rx_buffer=%p rx_buffered=%d rx_timeout=%d", uart->rx_buffer, options.rx_buffered, options.rx_timeout);

  taskENTER_CRITICAL(&uart->mux);

  uart_ll_rxfifo_rst(uart->dev);

  if (uart->rx_buffer) {
    reset = xStreamBufferReset(uart->rx_buffer);
  } else {
    reset = 1;
  }

  uart->rx_overflow = false;
  uart->rx_break = false;
  uart->rx_error = false;
  uart->rx_abort = false;

  if (options.rx_buffered) {
    uart_ll_set_rxfifo_full_thr(uart->dev, options.rx_buffered);
  } else {
    uart_ll_set_rxfifo_full_thr(uart->dev, UART_RX_FULL_THRD_DEFAULT);
  }

  if (options.rx_timeout) {
    uart_ll_set_rx_tout(uart->dev, options.rx_timeout * uart_symbol_bits(options));
  } else {
    uart_ll_set_rx_tout(uart->dev, UART_RX_TOUT_DEFAULT * uart_symbol_bits(options));
  }


  if (uart->rx_buffer) {
    uart_ll_ena_intr_mask(uart->dev, UART_RX_INTR_MASK);
  }

  taskEXIT_CRITICAL(&uart->mux);

  if (!reset) {
    LOG_WARN("xStreamBufferReset: RX buffer busy");
  }

  return 0;
}

enum uart_rx_event uart_rx_event(struct uart *uart)
{
  enum uart_rx_event event;

  taskENTER_CRITICAL(&uart->mux);

  if (!uart->rx_buffer) {
    event = UART_RX_DISABLED;

  } else if (xStreamBufferBytesAvailable(uart->rx_buffer)) {
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
    uart_ll_ena_intr_mask(uart->dev, UART_RX_READ_INTR_MASK);
  }

  taskEXIT_CRITICAL(&uart->mux);

  return event;
}

int uart_rx_read(struct uart *uart, void *buf, size_t size, TickType_t timeout)
{
  if (!uart->rx_buffer) {
    LOG_ERROR("rx_buffer disabled");
    return -1;
  }

  // block on ISR RX FIFO -> buffer copy
  return xStreamBufferReceive(uart->rx_buffer, buf, size, timeout);
}

void uart_rx_abort(struct uart *uart)
{
  taskENTER_CRITICAL(&uart->mux);

  // pause reading from RX FIFO
  uart_ll_disable_intr_mask(uart->dev, UART_RX_READ_INTR_MASK);

  uart->rx_abort = true;

  // TODO: wakeup uart_rx_read() via ISR?
  // vStreamBufferSendCompleted(uart->rx_buffer);

  taskEXIT_CRITICAL(&uart->mux);
}
