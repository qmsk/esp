#include <uart.h>
#include "../uart.h"
#include "dev.h"

#include <logging.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

static uart_dev_t *uart_dev[UART_PORT_MAX] = {
  [UART_0]  = &uart0,
  [UART_1]  = &uart1,
};

int uart_dev_setup(struct uart *uart, struct uart_options options)
{
  int err = 0;

  if (!uart_dev[uart->port & UART_PORT_MASK]) {
    LOG_ERROR("invalid uart_dev[%d]", (uart->port & UART_PORT_MASK));
    return -1;
  }

  if (options.dev_mutex) {
    LOG_DEBUG("take dev_mutex=%p", options.dev_mutex);

    if (!xSemaphoreTake(options.dev_mutex, portMAX_DELAY)) {
      LOG_ERROR("xSemaphoreTake");
      return -1;
    } else {
      uart->dev_mutex = options.dev_mutex;

      LOG_DEBUG("have dev_mutex=%p", uart->dev_mutex);
    }
  }

  LOG_DEBUG("port=%x: clock_div=%d data_bits=%x parity_bits=%x stop_bits=%x rx(timeout=%u, buffered=%u) inverted(rx=%d, tx=%d)",
    uart->port,
    options.clock_div,
    options.data_bits,
    options.parity_bits,
    options.stop_bits,
    options.rx_timeout, options.rx_buffered,
    options.rx_inverted, options.tx_inverted
  );

  uart->dev = uart_dev[uart->port & UART_PORT_MASK];

  taskENTER_CRITICAL();

  uart->dev->clk_div.div_int = options.clock_div;

  uart->dev->conf0.parity = options.parity_bits & 0x1;
  uart->dev->conf0.parity_en = options.parity_bits ? 1 : 0;
  uart->dev->conf0.bit_num = options.data_bits;
  uart->dev->conf0.stop_bit_num = options.stop_bits;
  uart->dev->conf0.rxd_inv = options.rx_inverted ? 1 : 0;
  uart->dev->conf0.txd_inv = options.tx_inverted ? 1 : 0;

  taskEXIT_CRITICAL();

  return err;
}

void uart_dev_teardown(struct uart *uart)
{
  uart->dev = NULL;

  if (uart->dev_mutex) {
    LOG_DEBUG("give dev_mutex=%p", uart->dev_mutex);

    xSemaphoreGive(uart->dev_mutex);

    uart->dev_mutex = NULL;
  }
}
