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

static void uart0_rx_intr_enable()
{
  uart0.int_ena.val |= (UART_RXFIFO_TOUT_INT_ENA | UART_BRK_DET_INT_ENA | UART_RXFIFO_OVF_INT_ENA | UART_FRM_ERR_INT_ST | UART_PARITY_ERR_INT_ST | UART_RXFIFO_FULL_INT_ENA);
}

static void uart0_rx_intr_disable()
{
  uart0.int_ena.val = 0;
}

void uart0_rx_setup(struct uart0 *uart)
{
  taskENTER_CRITICAL();

  uart0.conf0.rxfifo_rst = 1;
  uart0.conf0.rxfifo_rst = 0;

  uart0_rx_intr_disable();

  taskEXIT_CRITICAL();

  if (!xStreamBufferReset(uart->rx_buffer)) {
    LOG_WARN("xStreamBufferReset");
  }

  uart->rx_overflow = false;
  uart->rx_break = false;
  uart->rx_error = false;
}

int uart0_rx_errors(struct uart0 *uart, int *ret)
{
  if (uart->rx_overflow) {
    LOG_WARN("RX overflow detected");

    uart->rx_overflow = false;

    *ret = -1;

    return 1;
  }

  if (uart->rx_error) {
    LOG_WARN("RX error detected");

    uart->rx_error = false;

    *ret = -1;

    return 1;
  }

  if (uart->rx_break) {
    LOG_DEBUG("RX break detected");

    uart->rx_break = false;

    *ret = 0;

    return 1;
  }

  return 0;
}

int uart0_rx_read(struct uart0 *uart, void *buf, size_t size)
{
  int ret;

  // first consume any bytes immediately available to drain the buffer
  if ((ret = xStreamBufferReceive(uart->rx_buffer, buf, size, 0)) > 0) {
    return ret;
  }

  // report error/special cases
  if (uart0_rx_errors(uart, &ret)) {
    return ret;
  }

  // read from fifo via interrupt
  uart0_rx_intr_enable();

  if ((ret = xStreamBufferReceive(uart->rx_buffer, buf, size, portMAX_DELAY)) > 0) {
    return ret;
  }

  // report error/special cases
  uart0_rx_errors(uart, &ret);

  return ret;
}
