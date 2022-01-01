#include <uart.h>
#include "uart.h"
#include "pin.h"

#include <logging.h>

int uart_pin_setup(struct uart *uart, struct uart_options options)
{
  int err = 0;

  if (options.pin_mutex) {
    LOG_DEBUG("wait pin_mutex=%p", options.pin_mutex);

    if (!xSemaphoreTake(options.pin_mutex, portMAX_DELAY)) {
      LOG_ERROR("xSemaphoreTake");
      return -1;
    } else {
      uart->pin_mutex = options.pin_mutex;
    }
  }

  taskENTER_CRITICAL();

  switch(uart->port) {
    case UART_0:
      // reset UART0 CTS <-> RX and RTS <-> TX
      CLEAR_PERI_REG_MASK(UART_SWAP_REG, UART0_SWAP_BIT);

      // GPIO1 UART0 TX
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

    case UART_0_SWAP_RXONLY:
      // swap UART0 CTS <-> RX and RTS <-> TX
      SET_PERI_REG_MASK(UART_SWAP_REG, UART0_SWAP_BIT);

      // GPIO13 UART0 CTS -> RX
      IDEMPOTENT_PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTCK_U, FUNC_UART0_CTS);

      break;

    case UART_0_SWAP_TXONLY:
      // swap UART0 CTS <-> RX and RTS <-> TX
      SET_PERI_REG_MASK(UART_SWAP_REG, UART0_SWAP_BIT);

      // GPIO15 UART0 RTS -> TX
      IDEMPOTENT_PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDO_U, FUNC_UART0_RTS);

      break;

    case UART_0_TXDBK:
      // reset UART0 CTS <-> RX and RTS <-> TX
      CLEAR_PERI_REG_MASK(UART_SWAP_REG, UART0_SWAP_BIT);

      // GPIO1 UART0 TX
      IDEMPOTENT_PIN_FUNC_SELECT(PERIPHS_IO_MUX_U0TXD_U, FUNC_U0TXD);

      // GPIO2 UART0 TX
      IDEMPOTENT_PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO2_U, FUNC_UART0_TXD_BK);

      // GPIO3 UART0 RX
      IDEMPOTENT_PIN_FUNC_SELECT(PERIPHS_IO_MUX_U0RXD_U, FUNC_U0RXD);

      break;

    case UART_1:
      // GPIO2 UART1 TX
      IDEMPOTENT_PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO2_U, FUNC_UART1_TXD_BK);

      break;

    default:
      err = -1;

      break;
  }

  taskEXIT_CRITICAL();

  return err;
}

void uart_pin_teardown(struct uart *uart)
{
  if (uart->pin_mutex) {
    xSemaphoreGive(uart->pin_mutex);

    uart->pin_mutex = NULL;
  }
}
