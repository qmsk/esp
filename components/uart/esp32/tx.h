#pragma once

#include <hal/uart_ll.h>
#include <soc/soc_caps.h>
#include <soc/uart_reg.h>

// size of write buffer used by ISR
#define UART_TX_BUF_SIZE (SOC_UART_FIFO_LEN / 2)
#define UART_TX_EMPTY_THRD_DEFAULT (SOC_UART_FIFO_LEN / 2)

#define UART_TX_WRITE_INTR_MASK (UART_INTR_TXFIFO_EMPTY)

/* Custom, HAL does not provide any method to read a single byte */
static inline void uart_tx_write_txfifo_byte(struct uart *uart, uint8_t byte)
{
  WRITE_PERI_REG(UART_FIFO_AHB_REG(uart->port & UART_PORT_MASK), byte);
}
