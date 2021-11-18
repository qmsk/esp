#include <uart1.h>
#include "uart1.h"

#include <logging.h>

#include <esp8266/uart_struct.h>
#include <esp8266/uart_register.h>
#include <esp8266/pin_mux_register.h>
#include <esp8266/eagle_soc.h>
#include <esp8266/rom_functions.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#define UART1_TXFIFO_SIZE 128
#define UART1_TXBUF_SIZE 64 // on ISR stack

void uart1_tx_setup(struct uart1_options options)
{
  LOG_DEBUG("clock_div=%d data_bits=%x parity_bits=%x stop_bits=%x inverted=%x",
    options.clock_div,
    options.data_bits,
    options.parity_bits,
    options.stop_bits,
    options.inverted
  );

  taskENTER_CRITICAL();

  uart1.clk_div.div_int = options.clock_div;

  uart1.conf0.parity = options.parity_bits & 0x1;
  uart1.conf0.parity_en = options.parity_bits ? 1 : 0;
  uart1.conf0.bit_num = options.data_bits;
  uart1.conf0.stop_bit_num = options.stop_bits;
  uart1.conf0.txd_inv = options.inverted ? 1 : 0;

  // GPIO2 UART1 TX
  PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO2_U, FUNC_UART1_TXD_BK);

  taskEXIT_CRITICAL();
}

static inline void uart1_tx_intr_enable(int empty_threshold)
{
  LOG_ISR_DEBUG("empty_threshold=%u", empty_threshold);

  uart1.conf1.txfifo_empty_thrhd = empty_threshold;
  uart1.int_ena.txfifo_empty = 1;
}

int uart1_tx_one(struct uart1 *uart, uint8_t byte)
{
  int ret;

  taskENTER_CRITICAL();

  if (uart1.status.txfifo_cnt < UART1_TXFIFO_SIZE) {
    uart1.fifo.rw_byte = byte;

    LOG_ISR_DEBUG("tx fifo");

    ret = 0;

  } else if (xStreamBufferSend(uart->tx_buffer, &byte, 1, portMAX_DELAY) > 0) {
    LOG_ISR_DEBUG("tx buffer");

    // byte was written
    uart1_tx_intr_enable(UART1_TXBUF_SIZE);

    ret = 0;

  } else {
    LOG_ISR_DEBUG("failed");

    ret = -1;
  }

  taskEXIT_CRITICAL();

  return ret;
}

static size_t uart1_tx_raw(const uint8_t *buf, size_t size)
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

  taskENTER_CRITICAL();

  if (xStreamBufferIsEmpty(uart->tx_buffer)) {
    // fastpath via HW FIFO
    write = uart1_tx_raw(buf, len);

    LOG_ISR_DEBUG("raw len=%u: write=%u", len, write);
  }

  if (!write) {
    // write as many bytes as possible, ensure tx buffer is not empty
    write = xStreamBufferSend(uart->tx_buffer, buf, len, 0);

    LOG_ISR_DEBUG("buf len=%u: write=%u", len, write);

    // enable ISR to consume stream buffer
    uart1_tx_intr_enable(UART1_TXBUF_SIZE);
  }

  taskEXIT_CRITICAL();

  return write;
}

size_t uart1_tx_buf(struct uart1 *uart, const uint8_t *buf, size_t len)
{
  size_t write;

  // write as many bytes as possible, ensure tx buffer is not empty
  write = xStreamBufferSend(uart->tx_buffer, buf, len, 0);

  LOG_ISR_DEBUG("buf len=%u: write=%u", len, write);

  if (write == 0) {
    // TX buffer full, enable ISR
    uart1_tx_intr_enable(UART1_TXBUF_SIZE);
  }

  return write;
}

size_t uart1_tx_slow(struct uart1 *uart, const uint8_t *buf, size_t len)
{
  size_t write;

  // does not use a critical section, inter enable racing with stream send / ISR is harmless
  write = xStreamBufferSend(uart->tx_buffer, buf, len, portMAX_DELAY);

  LOG_ISR_DEBUG("xStreamBufferSend len=%u: write=%u", len, write);

  // enable ISR to consume stream buffer
  uart1_tx_intr_enable(UART1_TXBUF_SIZE);

  return write;
}

int uart1_tx_flush(struct uart1 *uart)
{
  TaskHandle_t task = xTaskGetCurrentTaskHandle();

  taskENTER_CRITICAL();

  // notify task once complete
  uart->txfifo_empty_notify_task = task;

  LOG_ISR_DEBUG("wait tx break task=%p", uart->txfifo_empty_notify_task);

  if (xStreamBufferIsEmpty(uart->tx_buffer)) {
    // enable TX interrupts with low empty threshold, to interrupt once TX queue is empty
    uart1_tx_intr_enable(1);
  } else {
    // enable ISR to consume stream buffer
    uart1_tx_intr_enable(UART1_TXBUF_SIZE);
  }

  taskEXIT_CRITICAL();

  // wait for tx to complete and break to start
  if (!ulTaskNotifyTake(true, portMAX_DELAY)) {
    LOG_WARN("timeout");
    return -1;
  }

  LOG_DEBUG("done");

  return 0;
}

void uart1_tx_break(struct uart1 *uart)
{
  taskENTER_CRITICAL();

  LOG_ISR_DEBUG(" ");

  uart1.conf0.txd_brk = 1;

  taskEXIT_CRITICAL();
}


void uart1_tx_mark(struct uart1 *uart)
{
  taskENTER_CRITICAL();

  LOG_ISR_DEBUG(" ");

  uart1.conf0.txd_brk = 0;

  taskEXIT_CRITICAL();
}

/* Bytes waiting in TX buffer */
static inline size_t uart1_tx_len()
{
  return uart1.status.txfifo_cnt;
}

/* Space available in tx buffer */
static inline size_t uart1_tx_size()
{
  return UART1_TXFIFO_SIZE - uart1.status.txfifo_cnt;
}

static inline void uart1_tx_intr_disable()
{
  LOG_ISR_DEBUG(" ");

  uart1.int_ena.txfifo_empty = 0;
}

static inline void uart1_tx_intr_clear()
{
  uart1.int_clr.txfifo_empty = 1;
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

  LOG_ISR_DEBUG("xStreamBufferReceiveFromISR size=%u: len=%u", tx_size, tx_len);

  if (tx_len == 0) {
    if (!uart->txfifo_empty_notify_task) {
      LOG_ISR_DEBUG("buffer empty, no txfifo_empty_notify_task");

      // buffer is empty, nothing to queue, allow it to empty
      uart1_tx_intr_disable();

    } else if (uart1_tx_len() > 0) {
      LOG_ISR_DEBUG("wait txfifo_empty_notify_task=%p fifo empty", uart->txfifo_empty_notify_task);

      // task waiting for FIFO to empty
      uart1_tx_intr_enable(1);

    } else {
      LOG_ISR_DEBUG("notify txfifo_empty_notify_task=%p", uart->txfifo_empty_notify_task);

      // FIFO is empty, notify task
      vTaskNotifyGiveFromISR(uart->txfifo_empty_notify_task, task_woken);

      // only once
      uart->txfifo_empty_notify_task = NULL;

      uart1_tx_intr_disable();
    }
  } else {
    // this should always happen in a single call due to the uart1_tx_size() check
    for (uint8_t *tx_ptr = tx_buf; tx_len > 0; ) {
      size_t tx = uart1_tx_raw(tx_ptr, tx_len);

      tx_ptr += tx;
      tx_len -= tx;
    }
  }

  uart1_tx_intr_clear();
}
