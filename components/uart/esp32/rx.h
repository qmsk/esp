#pragma once

#include <hal/uart_ll.h>
#include <soc/soc_caps.h>
#include <soc/uart_reg.h>

// size of read buffer used by ISR
#define UART_RX_BUF_SIZE (SOC_UART_FIFO_LEN / 2)
#define UART_RX_FULL_THRD_DEFAULT (SOC_UART_FIFO_LEN / 2)

#define UART_RX_INTR_MASK (UART_INTR_RXFIFO_FULL | UART_INTR_PARITY_ERR | UART_INTR_FRAM_ERR | UART_INTR_RXFIFO_OVF | UART_INTR_BRK_DET | UART_INTR_RXFIFO_TOUT)
#define UART_RX_ERROR_INTR_MASK (UART_INTR_PARITY_ERR | UART_INTR_FRAM_ERR)
#define UART_RX_READ_INTR_MASK (UART_INTR_RXFIFO_FULL | UART_INTR_RXFIFO_TOUT)

/* Custom, HAL does not provide any method to read a single byte */
static inline uint8_t uart_rx_read_rxfifo_byte(struct uart *uart)
{
  return READ_PERI_REG(UART_FIFO_REG(uart->port & UART_PORT_MASK));
}
