#pragma once

#include <esp8266/uart_struct.h>
#include <esp8266/uart_register.h>
#include <esp8266/pin_mux_register.h>
#include <esp8266/eagle_soc.h>
#include <esp8266/rom_functions.h>

#define UART_RXFIFO_SIZE 128
#define UART_RXBUF_SIZE 64 // on ISR stack

/*
 * Reset the RX fifo, discarding any data not yet copied into the RX FIFO.
 */
static inline void uart_rx_fifo_reset(uart_dev_t *dev)
{
  dev->conf0.rxfifo_rst = 1;
  dev->conf0.rxfifo_rst = 0;
}

static inline uint32_t uart_rx_fifo_count(uart_dev_t *dev)
{
  return dev->status.rxfifo_cnt;
}

static inline uint8_t uart_rx_fifo_read(uart_dev_t *dev)
{
  return dev->fifo.rw_byte;
}

static inline void uart_rx_conf_full_threshold(uart_dev_t *dev, uint32_t value)
{
  dev->conf1.rxfifo_full_thrhd = value;
}

static inline void uart_rx_conf_tout_threshold(uart_dev_t *dev, uint32_t value, bool enabled)
{
  dev->conf1.rx_tout_thrhd = value;
  dev->conf1.rx_tout_en = enabled;
}

static inline void uart_rx_intr_enable(uart_dev_t *dev)
{
  dev->int_ena.val |= (UART_RXFIFO_TOUT_INT_ENA | UART_BRK_DET_INT_ENA | UART_RXFIFO_OVF_INT_ENA | UART_FRM_ERR_INT_ENA | UART_PARITY_ERR_INT_ENA | UART_RXFIFO_FULL_INT_ENA);
}

static inline void uart_rx_intr_pause(uart_dev_t *dev)
{
  dev->int_ena.val &= ~(UART_RXFIFO_TOUT_INT_ENA | UART_RXFIFO_FULL_INT_ENA);
}

static inline void uart_rx_intr_resume(uart_dev_t *dev)
{
  dev->int_ena.val |= (UART_RXFIFO_TOUT_INT_ENA | UART_RXFIFO_FULL_INT_ENA);
}
