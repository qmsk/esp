#include <uart.h>
#include "../uart.h"
#include "rx.h"
#include "tx.h"

#include <logging.h>

#include <esp_err.h>
#include <esp_intr_alloc.h>
#include <hal/uart_ll.h>
#include <soc/soc.h>

// use a non-shared intr
#define UART_INTR_ALLOC_FLAGS (0)


static const int uart_irq[UART_PORT_MAX] = {
  [UART_0]    = ETS_UART0_INTR_SOURCE,
  [UART_1]    = ETS_UART1_INTR_SOURCE,
  [UART_2]    = ETS_UART2_INTR_SOURCE,
};

/*
 * Flush the RX buffer, i.e. stop consuming the RX FIFO and allow the RX buffer to drain.
 */
static inline void uart_intr_rx_flush(struct uart *uart, BaseType_t *task_woken)
{
  xStreamBufferSendCompletedFromISR(uart->rx_buffer, task_woken);

  // stop copying bytes from RX FIFO -> buffer, allow rx_buffer to drain
  uart_ll_disable_intr_mask(uart->dev, UART_RX_READ_INTR_MASK);
}

static inline bool uart_intr_rx_paused(struct uart *uart)
{
  return uart->rx_overflow || uart->rx_break || uart->rx_error || uart->rx_abort;
}

static void IRAM_ATTR uart_intr_rx_overflow_handler(struct uart *uart, BaseType_t *task_woken)
{
  LOG_ISR_DEBUG("rx fifo overflow");

  // reset RX fifo to clear interrupt
  uart_ll_rxfifo_rst(uart->dev);

  // mark for uart_read() return
  uart->rx_overflow = true;

  // invalidate any break state, we will have lost data after the break
  uart->rx_break = false;

  uart_intr_rx_flush(uart, task_woken);

  uart_ll_clr_intsts_mask(uart->dev, UART_INTR_RXFIFO_OVF);
}

static void IRAM_ATTR uart_intr_rx_error_handler(struct uart *uart, BaseType_t *task_woken)
{
  LOG_ISR_DEBUG("rx error");

  // mark for uart_read() return
  uart->rx_error = true;

  uart_intr_rx_flush(uart, task_woken);

  uart_ll_clr_intsts_mask(uart->dev, UART_RX_ERROR_INTR_MASK);
}

static void IRAM_ATTR uart_intr_rx_break_handler(struct uart *uart, BaseType_t *task_woken)
{
  LOG_ISR_DEBUG("rx break");

  // The BREAK condition decodes as a 0x00 byte, with a framing error
  if (uart_ll_get_rxfifo_len(uart->dev) == 1 && uart_rx_read_rxfifo_byte(uart) == 0x00) {
    // clean break, mark for uart_read() return
    uart->rx_break = true;
  } else {
    // break triggered with old data remaining in the FIFO, or was delayed until new data in the FIFO
    // impossible to delinate where the break happened
    uart->rx_break = true;
    uart->rx_overflow = true;

    // reset RX fifo to avoid coalescing RX buffer data across breaks
    uart_ll_rxfifo_rst(uart->dev);
  }

  // flush RX buffer, and pause
  uart_intr_rx_flush(uart, task_woken);

  uart_ll_clr_intsts_mask(uart->dev, UART_INTR_BRK_DET);
}

static void IRAM_ATTR uart_intr_rx_handler(struct uart *uart, BaseType_t *task_woken)
{
  uint8_t buf[UART_RX_BUF_SIZE];
  size_t size = xStreamBufferSpacesAvailable(uart->rx_buffer);
  size_t len = uart_ll_get_rxfifo_len(uart->dev);

  if (uart_intr_rx_paused(uart)) {
    LOG_ISR_DEBUG("rx paused");

    uart_ll_clr_intsts_mask(uart->dev, UART_RX_READ_INTR_MASK);

    return;
  }

  if (len == 0) {
    // likely due to some earlier interrupt condition's handler
    LOG_ISR_DEBUG("rx fifo empty");

    uart_ll_clr_intsts_mask(uart->dev, UART_RX_READ_INTR_MASK);

    return;
  }

  if (size == 0) {
    LOG_ISR_DEBUG("rx buffer full");

    // wait until UART_RXFIFO_OVF_INT_ST to reset FIFO
    uart_ll_disable_intr_mask(uart->dev, UART_RX_READ_INTR_MASK);
    uart_ll_clr_intsts_mask(uart->dev, UART_RX_READ_INTR_MASK);

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
    buf[i] = uart_rx_read_rxfifo_byte(uart);
  }

  if ((size = xStreamBufferSendFromISR(uart->rx_buffer, buf, len, task_woken)) < len) {
    // should never happen per xStreamBufferSpacesAvailable()
    LOG_ISR_DEBUG("xStreamBufferSendFromISR len=%u: size=%u", len, size);

    uart->rx_overflow = true;

    uart_intr_rx_flush(uart, task_woken);
  }

  uart_ll_clr_intsts_mask(uart->dev, UART_RX_READ_INTR_MASK);
}

void IRAM_ATTR uart_intr_tx_done_handler(struct uart *uart, BaseType_t *task_woken)
{
  if (uart->tx_done_notify_task) {
    LOG_ISR_DEBUG("notify tx_done_notify_task=%p", uart->tx_done_notify_task);

    // FIFO is empty, notify task
    vTaskNotifyGiveFromISR(uart->tx_done_notify_task, task_woken);

    // only once
    uart->tx_done_notify_task = NULL;

    uart_ll_disable_intr_mask(uart->dev, UART_INTR_TX_DONE);
  }

  uart_ll_clr_intsts_mask(uart->dev, UART_INTR_TX_DONE);
}

void IRAM_ATTR uart_intr_tx_handler(struct uart *uart, BaseType_t *task_woken)
{
  uint8_t buf[UART_TX_BUF_SIZE];
  size_t size = uart_ll_get_txfifo_len(uart->dev);
  size_t len;

  if (size > sizeof(buf)) {
    // partially fill TX queue
    size = sizeof(buf);
  }

  len = xStreamBufferReceiveFromISR(uart->tx_buffer, buf, size, task_woken);

  LOG_ISR_DEBUG("xStreamBufferReceiveFromISR size=%u: len=%u", size, len);

  if (len) {
    uart_ll_write_txfifo(uart->dev, buf, len);

  } else if (uart->tx_done_notify_task) {
    LOG_ISR_DEBUG("wait tx_done_notify_task=%p TX done", uart->tx_done_notify_task);

    // trigger TX done isr once TX fifo is empty
    uart_ll_ena_intr_mask(uart->dev, UART_INTR_TX_DONE);

    // buffer is empty, nothing to write, allow tx fifo to empty
    uart_ll_disable_intr_mask(uart->dev, UART_TX_WRITE_INTR_MASK);

  } else {
    LOG_ISR_DEBUG("buffer empty, go idle");

    // buffer is empty, nothing to write, allow tx fifo to empty
    uart_ll_disable_intr_mask(uart->dev, UART_TX_WRITE_INTR_MASK);
  }

  uart_ll_clr_intsts_mask(uart->dev, UART_TX_WRITE_INTR_MASK);
}

void IRAM_ATTR uart_intr_handler(void *ctx)
{
  struct uart *uart = ctx;
  uint32_t int_st = uart_ll_get_intsts_mask(uart->dev);
  BaseType_t task_woken = pdFALSE;

  if (!int_st) {
    return;
  }

  taskENTER_CRITICAL_ISR(&uart->mux);

  if (int_st & UART_INTR_RXFIFO_OVF) {
    uart_intr_rx_overflow_handler(uart, &task_woken);
  }
  if (int_st & UART_RX_ERROR_INTR_MASK) {
    uart_intr_rx_error_handler(uart, &task_woken);
  }
  if (int_st & UART_INTR_BRK_DET) {
    uart_intr_rx_break_handler(uart, &task_woken);
  }
  if (int_st & UART_RX_READ_INTR_MASK) {
    uart_intr_rx_handler(uart, &task_woken);
  }

  if (int_st & UART_TX_WRITE_INTR_MASK) {
    uart_intr_tx_handler(uart, &task_woken);
  }
  if (int_st & UART_INTR_TX_DONE) {
    uart_intr_tx_done_handler(uart, &task_woken);
  }

  taskEXIT_CRITICAL_ISR(&uart->mux);

  if (task_woken == pdTRUE) {
    portYIELD_FROM_ISR();
  }
}

int uart_intr_setup(struct uart *uart)
{
  esp_err_t err;

  LOG_DEBUG("");

  taskENTER_CRITICAL(&uart->mux);

  uart_ll_disable_intr_mask(uart->dev, UART_LL_INTR_MASK);
  uart_ll_clr_intsts_mask(uart->dev, UART_LL_INTR_MASK);

  err = esp_intr_alloc(uart_irq[uart->port & UART_PORT_MASK], UART_INTR_ALLOC_FLAGS, uart_intr_handler, uart, &uart->intr);

  taskEXIT_CRITICAL(&uart->mux);

  if (err) {
    LOG_ERROR("esp_intr_alloc: %s", esp_err_to_name(err));
    return -1;
  }

  return 0;
}

void uart_intr_teardown(struct uart *uart)
{
  esp_err_t err;

  taskENTER_CRITICAL(&uart->mux);

  uart_ll_disable_intr_mask(uart->dev, UART_LL_INTR_MASK);
  uart_ll_clr_intsts_mask(uart->dev, UART_LL_INTR_MASK);

  err = esp_intr_free(uart->intr);

  taskEXIT_CRITICAL(&uart->mux);

  uart->intr = NULL;

  if (err) {
    LOG_WARN("esp_intr_free: %s", esp_err_to_name(err));
  }
}
