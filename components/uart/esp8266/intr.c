#include "uart.h"
#include "../uart.h"
#include "intr.h"
#include "rx.h"
#include "tx.h"

#include <logging.h>

#include <esp8266/uart_struct.h>
#include <esp8266/uart_register.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

/*
 * Flush the RX buffer, i.e. stop consuming the RX FIFO and allow the RX buffer to drain.
 */
static inline void uart_intr_rx_flush(struct uart *uart, BaseType_t *task_woken)
{
  xStreamBufferSendCompletedFromISR(uart->rx_buffer, task_woken);

  // stop copying bytes from RX FIFO -> buffer, allow rx_buffer to drain
  uart_intr_rx_disable(uart->dev);
}

static inline bool uart_intr_rx_paused(struct uart *uart)
{
  return uart->rx_overflow || uart->rx_break || uart->rx_error || uart->rx_abort;
}

static void IRAM_ATTR uart_intr_rx_overflow_handler(struct uart *uart, BaseType_t *task_woken)
{
  LOG_ISR_DEBUG("rx fifo overflow");

  // reset RX fifo to clear interrupt
  uart_rx_fifo_reset(uart->dev);

  // mark for uart_read() return
  uart->rx_overflow = true;

  // invalidate any break state, we will have lost data after the break
  uart->rx_break = false;

  uart_intr_rx_flush(uart, task_woken);

  uart_intr_rx_overflow_clear(uart->dev);
}

static void IRAM_ATTR uart_intr_rx_error_handler(struct uart *uart, BaseType_t *task_woken)
{
  LOG_ISR_DEBUG("rx error");

  // mark for uart_read() return
  uart->rx_error = true;

  uart_intr_rx_flush(uart, task_woken);

  uart_intr_rx_error_clear(uart->dev);
}

static void IRAM_ATTR uart_intr_rx_break_handler(struct uart *uart, BaseType_t *task_woken)
{
  LOG_ISR_DEBUG("rx break");

  // The BREAK condition decodes as a 0x00 byte, with a framing error
  if (uart_rx_fifo_count(uart->dev) == 1 && uart_rx_fifo_read(uart->dev) == 0x00) {
    // clean break, mark for uart_read() return
    uart->rx_break = true;
  } else {
    // break triggered with old data remaining in the FIFO, or was delayed until new data in the FIFO
    // impossible to delinate where the break happened
    uart->rx_break = true;
    uart->rx_overflow = true;

    // reset RX fifo to avoid coalescing RX buffer data across breaks
    uart_rx_fifo_reset(uart->dev);
  }

  // flush RX buffer, and pause
  uart_intr_rx_flush(uart, task_woken);

  uart_intr_rx_break_clear(uart->dev);
}

static void IRAM_ATTR uart_intr_rx_handler(struct uart *uart, BaseType_t *task_woken)
{
  uint8_t buf[UART_RXBUF_SIZE];
  size_t size = xStreamBufferSpacesAvailable(uart->rx_buffer);
  size_t len = uart_rx_fifo_count(uart->dev);

  if (uart_intr_rx_paused(uart)) {
    LOG_ISR_DEBUG("rx paused");

    uart_intr_rx_clear(uart->dev);

    return;
  }

  if (len == 0) {
    // likely due to some earlier interrupt condition's handler
    LOG_ISR_DEBUG("rx fifo empty");

    uart_intr_rx_clear(uart->dev);

    return;
  }

  if (size == 0) {
    LOG_ISR_DEBUG("rx buffer full");

    // wait until UART_RXFIFO_OVF_INT_ST to reset FIFO
    uart_intr_rx_disable(uart->dev);
    uart_intr_rx_clear(uart->dev);

    return;
  }

  if (size > sizeof(buf)) {
    size = sizeof(buf);
  }

  if (len > size) {
    len = size;
  }

  LOG_ISR_DEBUG("rx len=%u size=%u", len, size);

  // copy from FIFO to buffer
  for (unsigned i = 0; i < len; i++) {
    // consume hardware FIFO
    buf[i] = uart_rx_fifo_read(uart->dev);
  }

  if ((size = xStreamBufferSendFromISR(uart->rx_buffer, buf, len, task_woken)) < len) {
    // should never happen per xStreamBufferSpacesAvailable()
    LOG_ISR_DEBUG("xStreamBufferSendFromISR len=%u: size=%u", len, size);

    uart->rx_overflow = true;

    uart_intr_rx_flush(uart, task_woken);
  }

  uart_intr_rx_clear(uart->dev);
}

void IRAM_ATTR uart_intr_tx_handler(struct uart *uart, BaseType_t *task_woken)
{
  uint8_t buf[UART_TXBUF_SIZE];
  size_t size = UART_TXFIFO_SIZE - uart_tx_fifo_count(uart->dev);
  size_t len;

  if (size > sizeof(buf)) {
    // partially fill TX queue
    size = sizeof(buf);
  }

  len = xStreamBufferReceiveFromISR(uart->tx_buffer, buf, size, task_woken);

  LOG_ISR_DEBUG("xStreamBufferReceiveFromISR size=%u: len=%u", size, len);

  if (len == 0) {
    if (!uart->txfifo_empty_notify_task) {
      LOG_ISR_DEBUG("buffer empty, no txfifo_empty_notify_task");

      // buffer is empty, nothing to queue, allow it to empty
      uart_intr_tx_disable(uart->dev);

    } else if (uart_tx_fifo_count(uart->dev) > 0) {
      LOG_ISR_DEBUG("wait txfifo_empty_notify_task=%p fifo empty", uart->txfifo_empty_notify_task);

      // task waiting for FIFO to empty
      uart_tx_intr_enable(uart->dev, 1);

    } else {
      LOG_ISR_DEBUG("notify txfifo_empty_notify_task=%p", uart->txfifo_empty_notify_task);

      // FIFO is empty, notify task
      vTaskNotifyGiveFromISR(uart->txfifo_empty_notify_task, task_woken);

      // only once
      uart->txfifo_empty_notify_task = NULL;

      uart_intr_tx_disable(uart->dev);
    }
  } else {
    // this should never overflow the TX FIFO due to the uart_tx_size() check
    for (uint8_t *ptr = buf; ptr < buf + len; ptr++) {
      uart_tx_fifo_write(uart->dev, *ptr);
    }
  }

  uart_intr_tx_clear(uart->dev);
}

void IRAM_ATTR uart_intr_handler(void *ctx)
{
  struct uart *uart = ctx;
  uint32_t int_st = uart_intr_status(uart->dev);
  BaseType_t task_woken = pdFALSE;

  if (int_st & UART_RXFIFO_OVF_INT_ST) {
    uart_intr_rx_overflow_handler(uart, &task_woken);
  }
  if (int_st & (UART_FRM_ERR_INT_ST | UART_PARITY_ERR_INT_ST)) {
    uart_intr_rx_error_handler(uart, &task_woken);
  }
  if (int_st & UART_BRK_DET_INT_ST) {
    uart_intr_rx_break_handler(uart, &task_woken);
  }
  if (int_st & (UART_RXFIFO_TOUT_INT_ST | UART_RXFIFO_FULL_INT_ST)) {
    uart_intr_rx_handler(uart, &task_woken);
  }
  if (int_st & UART_TXFIFO_EMPTY_INT_ST) {
    uart_intr_tx_handler(uart, &task_woken);
  }

  if (task_woken == pdTRUE) {
    portYIELD_FROM_ISR();
  }
}

int uart_intr_setup(struct uart *uart)
{
  taskENTER_CRITICAL();

  uart_intr_disable(uart->dev);
  uart_intr_clear(uart->dev);

  uart_isr_init();
  uart_isr_setup(uart->port, uart_intr_handler, uart);

  taskEXIT_CRITICAL();

  return 0;
}

void uart_intr_teardown(struct uart *uart)
{
  taskENTER_CRITICAL();

  uart_intr_disable(uart->dev);
  uart_intr_clear(uart->dev);

  uart_isr_teardown(uart->port);

  taskEXIT_CRITICAL();
}
