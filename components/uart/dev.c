#include <uart.h>
#include "uart.h"
#include "dev.h"

#include <logging.h>

#include <esp8266/uart_struct.h>
#include <esp8266/uart_register.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#define UART0_SWAP_BIT 0x4

void uart_dev_setup(struct uart *uart, struct uart_options options)
{
  LOG_DEBUG("port=%x: clock_div=%d data_bits=%x parity_bits=%x stop_bits=%x rx(timeout=%u, buffering=%u) inverted(rx=%d, tx=%d)",
    uart->port,
    options.clock_div,
    options.data_bits,
    options.parity_bits,
    options.stop_bits,
    options.rx_timeout, options.rx_buffering,
    options.rx_inverted,  options.tx_inverted
  );

  taskENTER_CRITICAL();

  uart->dev->clk_div.div_int = options.clock_div;

  uart->dev->conf0.parity = options.parity_bits & 0x1;
  uart->dev->conf0.parity_en = options.parity_bits ? 1 : 0;
  uart->dev->conf0.bit_num = options.data_bits;
  uart->dev->conf0.stop_bit_num = options.stop_bits;
  uart->dev->conf0.rxd_inv = options.rx_inverted ? 1 : 0;
  uart->dev->conf0.txd_inv = options.tx_inverted ? 1 : 0;

  switch(uart->port) {
    case UART_0:
      // reset UART0 CTS <-> RX and RTS <-> TX
      CLEAR_PERI_REG_MASK(UART_SWAP_REG, UART0_SWAP_BIT);

      // GPIO1 UART0 RX
      IDEMPOTENT_PIN_FUNC_SELECT(PERIPHS_IO_MUX_U0TXD_U, FUNC_U0TXD);

      // GPIO3 UART0 RX
      IDEMPOTENT_PIN_FUNC_SELECT(PERIPHS_IO_MUX_U0RXD_U, FUNC_U0RXD);

      break;

    case UART_0_SWAP:
      // swap UART0 CTS <-> RX and RTS <-> TX
      SET_PERI_REG_MASK(UART_SWAP_REG, UART0_SWAP_BIT);

      // GPIO13 UART0 CTS -> RX
      IDEMPOTENT_PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTCK_U, FUNC_UART0_CTS);

      // GPIO15 UART0 RTS -> TX
      IDEMPOTENT_PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDO_U, FUNC_UART0_RTS);

      break;

    case UART_1:
      // GPIO2 UART1 TX
      IDEMPOTENT_PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO2_U, FUNC_UART1_TXD_BK);

      break;

    default:
      LOG_ERROR("invalid port=%x", uart->port);
  }

  taskEXIT_CRITICAL();
}
