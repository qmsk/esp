#pragma once

#include <esp8266/uart_struct.h>
#include <esp8266/uart_register.h>

#define UART_INT_CLR_BITS (UART_RXFIFO_TOUT_INT_CLR | UART_BRK_DET_INT_CLR | UART_CTS_CHG_INT_CLR | UART_DSR_CHG_INT_CLR | UART_RXFIFO_OVF_INT_CLR | UART_FRM_ERR_INT_CLR | UART_PARITY_ERR_INT_CLR | UART_TXFIFO_EMPTY_INT_CLR | UART_RXFIFO_FULL_INT_CLR)

static inline uint32_t uart_intr_status(uart_dev_t *dev)
{
  return dev->int_st.val;
}

static inline void uart_intr_rx_pause(uart_dev_t *dev)
{
  dev->int_ena.val &= ~(UART_RXFIFO_TOUT_INT_ENA | UART_RXFIFO_FULL_INT_ENA);
}

static inline void uart_intr_rx_clear(uart_dev_t *dev)
{
  dev->int_clr.val |= (UART_RXFIFO_TOUT_INT_CLR | UART_RXFIFO_FULL_INT_CLR);
}

static inline void uart_intr_rx_overflow_disable(uart_dev_t *dev)
{
  dev->int_ena.val &= ~(UART_RXFIFO_OVF_INT_ENA);
}

static inline void uart_intr_rx_overflow_clear(uart_dev_t *dev)
{
  dev->int_clr.val |= (UART_RXFIFO_OVF_INT_CLR);
}

static inline void uart_intr_rx_error_disable(uart_dev_t *dev)
{
  dev->int_ena.val &= ~(UART_FRM_ERR_INT_ENA | UART_PARITY_ERR_INT_ENA);
}

static inline void uart_intr_rx_error_clear(uart_dev_t *dev)
{
  dev->int_clr.val |= (UART_FRM_ERR_INT_CLR | UART_PARITY_ERR_INT_CLR);
}

static inline void uart_intr_rx_break_disable(uart_dev_t *dev)
{
  dev->int_ena.val &= ~(UART_BRK_DET_INT_ENA);
}

static inline void uart_intr_rx_break_clear(uart_dev_t *dev)
{
  dev->int_clr.val |= (UART_BRK_DET_INT_CLR);
}

static inline void uart_intr_tx_disable(uart_dev_t *dev)
{
  dev->int_ena.txfifo_empty = 0;
}

static inline void uart_intr_tx_clear(uart_dev_t *dev)
{
  dev->int_clr.txfifo_empty = 1;
}

static inline void uart_intr_disable(uart_dev_t *dev)
{
  dev->int_ena.val = 0;
}

static inline void uart_intr_clear(uart_dev_t *dev)
{
  dev->int_clr.val = UART_INT_CLR_BITS;
}
