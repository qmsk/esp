#include <uart0.h>
#include "uart0.h"

#include <logging.h>

#include <esp8266/uart_struct.h>
#include <esp8266/uart_register.h>
#include <esp8266/pin_mux_register.h>
#include <esp8266/eagle_soc.h>
#include <esp8266/rom_functions.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

int uart0_rx_init(struct uart0 *uart, size_t rx_buffer_size)
{
  LOG_DEBUG("rx_buffer_size=%u", rx_buffer_size);

  if (!(uart->rx_buffer = xStreamBufferCreate(rx_buffer_size, 1))) {
    LOG_ERROR("xStreamBufferCreate");
    return -1;
  }

  return 0;
}

void uart0_rx_setup(struct uart0 *uart)
{
  int reset;

  taskENTER_CRITICAL();

  uart0.conf0.rxfifo_rst = 1;
  uart0.conf0.rxfifo_rst = 0;

  reset = xStreamBufferReset(uart->rx_buffer);

  uart->rx_overflow = false;
  uart->rx_break = false;
  uart->rx_error = false;

  taskEXIT_CRITICAL();

  if (!reset) {
    LOG_WARN("xStreamBufferReset: failed, RX buffer busy");
  }
}

enum uart0_rx_event {
  UART0_RX_NONE = 0,
  UART0_RX_DATA,
  UART0_RX_OVERFLOW,
  UART0_RX_ERROR,
  UART0_RX_BREAK,
};

enum uart0_rx_event uart0_rx_event(struct uart0 *uart)
{
  enum uart0_rx_event event = 0;

  taskENTER_CRITICAL();

  if (!event && xStreamBufferBytesAvailable(uart->rx_buffer)) {
    event = UART0_RX_DATA;
  }

  if (!event && uart->rx_overflow) {
    uart->rx_overflow = false;

    event = UART0_RX_OVERFLOW;
  }

  if (!event && uart->rx_error) {
    uart->rx_error = false;

    event = UART0_RX_ERROR;
  }

  if (!event && uart->rx_break) {
    uart->rx_break = false;

    event = UART0_RX_BREAK;
  }

  if (!event) {
    // RX buffer emptied and flags cleared, resume copying from RX FIFO if disabled
    uart0.int_ena.val |= (UART_RXFIFO_TOUT_INT_ENA | UART_RXFIFO_FULL_INT_ENA);
  }

  taskEXIT_CRITICAL();

  return event;
}

int uart0_rx_read(struct uart0 *uart, void *buf, size_t size)
{
  int ret = 0;

  while (!ret) {
    // report any error/special cases
    switch (uart0_rx_event(uart)) {
      case UART0_RX_DATA:
        // RX buffer has data available
        break;

      case UART0_RX_OVERFLOW:
        LOG_WARN("RX overflow detected");
        return -1;

      case UART0_RX_ERROR:
        LOG_WARN("RX error detected");
        return -1;

      case UART0_RX_BREAK:
        LOG_DEBUG("RX break detected");
        return 0;

      case UART0_RX_NONE:
        // block on RX buffer
        break;

      default:
        LOG_ERROR("unknown event");
        return -1;
    }

    // block on ISR RX FIFO -> buffer copy
    ret = xStreamBufferReceive(uart->rx_buffer, buf, size, portMAX_DELAY);
  }

  return ret;
}
