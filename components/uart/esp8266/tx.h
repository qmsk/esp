#pragma once

#include <esp8266/uart_struct.h>
#include <esp8266/uart_register.h>
#include <esp8266/pin_mux_register.h>
#include <esp8266/eagle_soc.h>
#include <esp8266/rom_functions.h>

#define UART_TXFIFO_SIZE 128
#define UART_TXBUF_SIZE 64 // on ISR stack


/*
 * Reset the TX fifo, discarding any data not yet copied into the RX FIFO.
 */
static inline void uart_tx_fifo_reset(uart_dev_t *dev)
{
  dev->conf0.txfifo_rst = 1;
  dev->conf0.txfifo_rst = 0;
}

static inline void uart_tx_intr_enable(uart_dev_t *dev, uint32_t empty_threshold)
{
  dev->conf1.txfifo_empty_thrhd = empty_threshold;

  dev->int_ena.txfifo_empty = 1;
}

static inline uint32_t uart_tx_fifo_count(uart_dev_t *dev)
{
  return dev->status.txfifo_cnt;
}

static inline void uart_tx_fifo_write(uart_dev_t *dev, uint8_t byte)
{
  dev->fifo.rw_byte = byte;
}

static inline void uart_tx_break_enable(uart_dev_t *dev)
{
  dev->conf0.txd_brk = 1;
}

static inline void uart_tx_break_disable(uart_dev_t *dev)
{
  dev->conf0.txd_brk = 0;
}
