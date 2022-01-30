#include <uart.h>
#include "../uart.h"
#include "tx.h"

#include <logging.h>

#include <hal/uart_ll.h>

int uart_tx_init(struct uart *uart, size_t tx_buffer_size)
{
  if (!(uart->tx_buffer = xStreamBufferCreate(tx_buffer_size, 1))) {
    LOG_ERROR("xStreamBufferCreate");
    return -1;
  }

  return 0;
}

int uart_tx_setup(struct uart *uart, struct uart_options options)
{
  int reset;

  taskENTER_CRITICAL(&uart->mux);

  // XXX: this is only implemented for UART0?
  uart_ll_txfifo_rst(uart->dev);

  reset = xStreamBufferReset(uart->tx_buffer);

  taskEXIT_CRITICAL(&uart->mux);

  if (!reset) {
    LOG_WARN("xStreamBufferReset: TX buffer busy");
  }

  return 0;
}

int uart_tx_one(struct uart *uart, uint8_t byte)
{
  int ret;

  taskENTER_CRITICAL(&uart->mux);

  if (uart_ll_get_txfifo_len(uart->dev) > 0) {
    uart_tx_write_txfifo_byte(uart, byte);

    LOG_ISR_DEBUG("tx fifo");

    ret = 0;

  } else if (xStreamBufferSend(uart->tx_buffer, &byte, 1, portMAX_DELAY) > 0) {
    LOG_ISR_DEBUG("tx buffer");

    // byte was written
    uart_ll_set_txfifo_empty_thr(uart->dev, UART_TX_EMPTY_THRD_DEFAULT);
    uart_ll_ena_intr_mask(uart->dev, UART_TX_WRITE_INTR_MASK);

    ret = 0;

  } else {
    LOG_ISR_DEBUG("failed");

    ret = -1;
  }

  taskEXIT_CRITICAL(&uart->mux);

  return ret;
}

static size_t uart_tx_raw(struct uart *uart, const uint8_t *buf, size_t size)
{
  size_t len = uart_ll_get_txfifo_len(uart->dev);

  if (len > size) {
    len = size;
  }

  // assume no crtical section required, as uart is locked and ISR should not be running
  uart_ll_write_txfifo(uart->dev, buf, len);

  return len;
}

size_t uart_tx_fast(struct uart *uart, const uint8_t *buf, size_t len)
{
  size_t write = 0;

  // assume no crtical section required, as uart is locked and ISR should not be running
  if (xStreamBufferIsEmpty(uart->tx_buffer)) {
    // fastpath via HW FIFO
    write = uart_tx_raw(uart, buf, len);

    LOG_ISR_DEBUG("raw len=%u: write=%u", len, write);
  }

  if (!write) {
    // write as many bytes as possible, ensure tx buffer is not empty
    write = xStreamBufferSend(uart->tx_buffer, buf, len, 0);

    LOG_ISR_DEBUG("buf len=%u: write=%u", len, write);

    // enable ISR to consume stream buffer
    uart_ll_set_txfifo_empty_thr(uart->dev, UART_TX_EMPTY_THRD_DEFAULT);
    uart_ll_ena_intr_mask(uart->dev, UART_TX_WRITE_INTR_MASK);
  }

  return write;
}


size_t uart_tx_buffered(struct uart *uart, const uint8_t *buf, size_t len)
{
  size_t write;

  // write as many bytes as possible, ensure tx buffer is not empty
  write = xStreamBufferSend(uart->tx_buffer, buf, len, 0);

  LOG_ISR_DEBUG("buf len=%u: write=%u", len, write);

  if (write == 0) {
    // TX buffer full, enable ISR
    uart_ll_set_txfifo_empty_thr(uart->dev, UART_TX_EMPTY_THRD_DEFAULT);
    uart_ll_ena_intr_mask(uart->dev, UART_TX_WRITE_INTR_MASK);
  }

  return write;
}

size_t uart_tx_slow(struct uart *uart, const uint8_t *buf, size_t len)
{
  size_t write;

  // does not use a critical section, inter enable racing with stream send / ISR is harmless
  write = xStreamBufferSend(uart->tx_buffer, buf, len, portMAX_DELAY);

  LOG_ISR_DEBUG("xStreamBufferSend len=%u: write=%u", len, write);

  // enable ISR to consume stream buffer
  uart_ll_set_txfifo_empty_thr(uart->dev, UART_TX_EMPTY_THRD_DEFAULT);
  uart_ll_ena_intr_mask(uart->dev, UART_TX_WRITE_INTR_MASK);

  return write;
}

int uart_tx_flush(struct uart *uart)
{
  TaskHandle_t task = xTaskGetCurrentTaskHandle();
  bool idle = false, done = false;

  LOG_DEBUG("");

  taskENTER_CRITICAL(&uart->mux);

  if (!xStreamBufferIsEmpty(uart->tx_buffer)) {
    // notify task once complete
    uart->tx_done_notify_task = task;

    // enable ISR to consume stream buffer
    uart_ll_set_txfifo_empty_thr(uart->dev, UART_TX_EMPTY_THRD_DEFAULT);
    uart_ll_ena_intr_mask(uart->dev, UART_TX_WRITE_INTR_MASK);

  } else if (!uart_ll_is_tx_idle(uart->dev)) {
    // notify task once complete
    uart->tx_done_notify_task = task;

    // enable TX done interrupts
    uart_ll_ena_intr_mask(uart->dev, UART_INTR_TX_DONE);

    done = true;
  } else {
    // no need to wait
    idle = true;
  }

  taskEXIT_CRITICAL(&uart->mux);

  if (idle) {
    LOG_DEBUG("idle");
  } else {
    LOG_DEBUG("wait done=%d...", done);

    // wait for tx to complete and break to start
    if (!ulTaskNotifyTake(true, portMAX_DELAY)) {
      LOG_WARN("timeout");
      return -1;
    }

    LOG_DEBUG("done");
  }

  return 0;
}

// TODO: using UART_TX_BRK_NUM + UART_TX_BRK_DONE_INT?
void uart_tx_break(struct uart *uart)
{
  taskENTER_CRITICAL(&uart->mux);

  uart_ll_tx_break(uart->dev, 255); // XXX: arbitrary number, needs interface change

  taskEXIT_CRITICAL(&uart->mux);
}

// TODO: using UART_TX_IDLE_NUM + UART_TX_BRK_IDLE_DONE_INT?
void uart_tx_mark(struct uart *uart)
{
  taskENTER_CRITICAL(&uart->mux);

  uart_ll_tx_break(uart->dev, 0);

  taskEXIT_CRITICAL(&uart->mux);
}
