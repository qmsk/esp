#include <uart1.h>
#include "uart1.h"

#include <esp8266/uart_struct.h>
#include <esp8266/uart_register.h>
#include <esp8266/pin_mux_register.h>
#include <esp8266/eagle_soc.h>
#include <esp8266/rom_functions.h>

#define UART1_TXFIFO_SIZE 128
#define UART1_TXBUF_SIZE 128 // on ISR stack

void uart1_tx_setup(struct uart1_options options)
{
  uart1.clk_div.div_int = options.clock_div;

  uart1.conf0.parity = options.parity_bits & 0x1;
  uart1.conf0.parity_en = options.parity_bits ? 1 : 0;
  uart1.conf0.bit_num = options.data_bits;
  uart1.conf0.stop_bit_num = options.stop_bits;
  uart1.conf0.txd_inv = options.inverted ? 1 : 0;

  // GPIO2 UART1 TX
  PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO2_U, FUNC_UART1_TXD_BK);
}

size_t uart1_tx_raw(const uint8_t *buf, size_t size)
{
  size_t len = 0;

  while (uart1.status.txfifo_cnt < UART1_TXFIFO_SIZE && len < size) {
    uart1.fifo.rw_byte = *buf;

    buf++;
    len++;
  }

  return len;
}

size_t uart1_tx_fast(struct uart1 *uart, const uint8_t *buf, size_t len)
{
  size_t write = 0;

  if (xStreamBufferIsEmpty(uart->tx_buffer)) {
    // fastpath
    write = uart1_tx_raw(buf, len);
  }

  return write;
}

void uart1_tx_intr_enable(int empty_threshold)
{
  uart1.conf1.txfifo_empty_thrhd = empty_threshold;
  uart1.int_ena.txfifo_empty = 1;
}

size_t uart1_tx_slow(struct uart1 *uart, const uint8_t *buf, size_t len)
{
  size_t write;

  write = xStreamBufferSend(uart->tx_buffer, buf, len, portMAX_DELAY);

  // enable ISR to consume stream buffer
  uart1_tx_intr_enable(UART1_TXFIFO_SIZE / 2);

  return write;
}

size_t uart1_tx_size()
{
  return UART1_TXFIFO_SIZE - uart1.status.txfifo_cnt;
}

void uart1_tx_intr_disable()
{
  uart1.int_ena.txfifo_empty = 0;
}

void uart1_tx_intr_handler(struct uart1 *uart, BaseType_t *task_woken)
{
  uint8_t tx_buf[UART1_TXBUF_SIZE];
  size_t tx_size;
  size_t tx_len;

  if ((tx_size = uart1_tx_size()) > sizeof(tx_buf)) {
    // partially fill TX queue
    tx_size = sizeof(tx_buf);
  }

  tx_len = xStreamBufferReceiveFromISR(uart->tx_buffer, tx_buf, tx_size, task_woken);

  if (tx_len == 0) {
    // buffer is empty, nothing to queue, allow it to empty
    uart1_tx_intr_disable();

    return;
  }

  // this should always happen in a single call due to the uart1_tx_size() check
  for (uint8_t *tx_ptr = tx_buf; tx_len > 0; ) {
    size_t tx = uart1_tx_raw(tx_ptr, tx_len);

    tx_ptr += tx;
    tx_len -= tx;
  }
}

void uart1_tx_intr_clear()
{
  uart1.int_clr.txfifo_empty = 1;
}
