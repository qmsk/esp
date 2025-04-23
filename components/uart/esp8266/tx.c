#include <uart.h>
#include "../uart.h"
#include "dev.h"
#include "tx.h"

#include <logging.h>

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

  taskENTER_CRITICAL();

  uart_tx_fifo_reset(uart->dev);

  reset = xStreamBufferReset(uart->tx_buffer);

  taskEXIT_CRITICAL();

  if (!reset) {
    LOG_WARN("xStreamBufferReset: TX buffer busy");
  }

  return 0;
}

int uart_tx_one(struct uart *uart, uint8_t byte, TickType_t timeout)
{
  int ret;

  taskENTER_CRITICAL();

  if (uart_tx_fifo_count(uart->dev) < UART_TXFIFO_SIZE) {
    uart_tx_fifo_write(uart->dev, byte);

    LOG_ISR_DEBUG("tx fifo");

    ret = 0;

  } else if (xStreamBufferSend(uart->tx_buffer, &byte, 1, timeout) > 0) {
    LOG_ISR_DEBUG("tx buffer");

    // byte was written
    uart_tx_intr_enable(uart->dev, UART_TXBUF_SIZE);

    ret = 0;

  } else {
    LOG_ISR_DEBUG("failed");

    ret = -1;
  }

  taskEXIT_CRITICAL();

  return ret;
}

static size_t uart_tx_raw(struct uart *uart, const uint8_t *buf, size_t size)
{
  size_t len = 0;

  // assume no crtical section required, as uart is locked and ISR should not be running
  while (uart_tx_fifo_count(uart->dev) < UART_TXFIFO_SIZE && len < size) {
    uart_tx_fifo_write(uart->dev, *buf);

    buf++;
    len++;
  }

  return len;
}

size_t uart_tx_fast(struct uart *uart, const uint8_t *buf, size_t len)
{
  size_t write = 0;

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
    uart_tx_intr_enable(uart->dev, UART_TXBUF_SIZE);
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
    uart_tx_intr_enable(uart->dev, UART_TXBUF_SIZE);
  }

  return write;
}

size_t uart_tx_slow(struct uart *uart, const uint8_t *buf, size_t len, TickType_t timeout)
{
  size_t write;

  // does not use a critical section, inter enable racing with stream send / ISR is harmless
  write = xStreamBufferSend(uart->tx_buffer, buf, len, timeout);

  LOG_ISR_DEBUG("xStreamBufferSend len=%u: write=%u", len, write);

  // enable ISR to consume stream buffer
  uart_tx_intr_enable(uart->dev, UART_TXBUF_SIZE);

  return write;
}

int uart_tx_flush(struct uart *uart, TickType_t timeout)
{
  TaskHandle_t task = xTaskGetCurrentTaskHandle();

  xTaskNotifyStateClear(task);

  taskENTER_CRITICAL();

  // notify task once complete
  uart->txfifo_empty_notify_task = task;

  LOG_ISR_DEBUG("wait tx break task=%p", uart->txfifo_empty_notify_task);

  if (xStreamBufferIsEmpty(uart->tx_buffer)) {
    // enable TX interrupts with low empty threshold, to interrupt once TX queue is empty
    uart_tx_intr_enable(uart->dev, 1);
  } else {
    // enable ISR to consume stream buffer
    uart_tx_intr_enable(uart->dev, UART_TXBUF_SIZE);
  }

  taskEXIT_CRITICAL();

  // wait for tx to complete and break to start
  if (!xTaskNotifyWait(0, 0, NULL, timeout)) {
    uart->tx_done_notify_task = NULL;

    LOG_WARN("timeout");
    return -1;
  }

  LOG_DEBUG("done");

  return 0;
}

static void uart_tx_wait(struct uart *uart, unsigned bits)
{
  uint32_t baud = uart_dev_get_baudrate(uart->dev);
  uint32_t us = 1000000 * bits / baud;

  LOG_DEBUG("baud=%u bits=%u -> us=%u", baud, bits, us);

  // ESP8266_RTOS_SDK timers are shit:
  // * FreeRTOS timers only do 10ms ticks
  // * esp_timer is just a wrapper for FreeRTOS timers
  // * os_timer only does msec
  // * there's no shared timer implementation for FRC1/2
  // * there's no FRC2?
  // so we just busyloop, 'cause that's what everybody else does. It's really dumb.
  ets_delay_us(us);
}

// called after uart_tx_flush() with tx mutex held
int uart_tx_break(struct uart *uart, unsigned bits)
{
  LOG_DEBUG("bits=%u", bits);

  uart_tx_break_enable(uart->dev);

  uart_tx_wait(uart, bits);

  uart_tx_break_disable(uart->dev);

  return 0;
}

// called after uart_tx_flush() with tx mutex held
int uart_tx_mark(struct uart *uart, unsigned bits)
{
  LOG_DEBUG("bits=%u", bits);

  uart_tx_wait(uart, bits);

  return 0;
}
