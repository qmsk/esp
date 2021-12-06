#include "uart0.h"

#include <logging.h>

#include <driver/uart.h>
#include <esp_err.h>

#include <esp8266/uart_struct.h>
#include <esp8266/uart_register.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#define UART0_INTR_MASK 0x1ff
#define UART0_RX_BUFFERING 64 // on ISR stack
#define UART0_RX_TIMEOUT 1 // immediately

/*
 * Reset the RX fifo, discarding any data not yet copied into the RX FIFO.
 */
static inline void uart0_intr_rx_reset()
{
  uart0.conf0.rxfifo_rst = 1;
  uart0.conf0.rxfifo_rst = 0;
}

static inline void uart0_intr_rx_enable()
{
  uart0.int_ena.val |= (UART_RXFIFO_TOUT_INT_ENA | UART_BRK_DET_INT_ENA | UART_RXFIFO_OVF_INT_ENA | UART_FRM_ERR_INT_ENA | UART_PARITY_ERR_INT_ENA | UART_RXFIFO_FULL_INT_ENA);
}

static inline void uart0_intr_rx_disable()
{
  uart0.int_ena.val &= ~(UART_RXFIFO_TOUT_INT_ENA | UART_RXFIFO_FULL_INT_ENA);
}

static inline void uart0_intr_rx_overflow_disable()
{
  uart0.int_ena.val &= ~(UART_RXFIFO_OVF_INT_ENA);
}

static inline void uart0_intr_rx_error_disable()
{
  uart0.int_ena.val &= ~(UART_FRM_ERR_INT_ENA | UART_PARITY_ERR_INT_ENA);
}

static inline void uart0_intr_rx_break_disable()
{
  uart0.int_ena.val &= ~(UART_BRK_DET_INT_ENA);
}

/*
 * Flush the RX buffer, i.e. stop consuming the RX FIFO and allow the RX buffer to drain.
 */
static void uart0_intr_rx_flush(struct uart0 *uart, BaseType_t *task_woken)
{
  xStreamBufferSendCompletedFromISR(uart->rx_buffer, task_woken);

  // stop copying bytes from RX FIFO -> buffer, allow rx_buffer to drain
  uart0_intr_rx_disable();
}

static inline void uart0_intr_rx_overflow_clear()
{
  uart0.int_clr.val |= (UART_RXFIFO_OVF_INT_CLR);
}

static void uart0_intr_rx_overflow_handler(struct uart0 *uart, BaseType_t *task_woken)
{
  LOG_ISR_DEBUG("rx fifo overflow");

  // reset RX fifo to clear interrupt
  uart0_intr_rx_reset();

  // mark for uart0_read() return
  uart->rx_overflow = true;

  uart0_intr_rx_flush(uart, task_woken);

  uart0_intr_rx_overflow_clear();
}

static inline void uart0_intr_rx_error_clear()
{
  uart0.int_clr.val |= (UART_FRM_ERR_INT_CLR | UART_PARITY_ERR_INT_CLR);
}

static void uart0_intr_rx_error_handler(struct uart0 *uart, BaseType_t *task_woken)
{
  LOG_ISR_DEBUG("rx error");

  // mark for uart0_read() return
  uart->rx_error = true;

  uart0_intr_rx_flush(uart, task_woken);

  uart0_intr_rx_error_clear();
}

static inline void uart0_intr_rx_break_clear()
{
  uart0.int_clr.val |= (UART_BRK_DET_INT_CLR);
}

static void uart0_intr_rx_break_handler(struct uart0 *uart, BaseType_t *task_woken)
{
  LOG_ISR_DEBUG("rx break");

  // reset RX fifo to avoid coalescing RX buffer data across breaks
  // BUG: workaround spurious 0-byte in RX FIFO?
  uart0_intr_rx_reset();

  // mark for uart0_read() return
  uart->rx_break = true;

  uart0_intr_rx_flush(uart, task_woken);

  uart0_intr_rx_break_clear();
}

static inline void uart0_intr_rx_clear()
{
  uart0.int_clr.val |= (UART_RXFIFO_TOUT_INT_CLR | UART_RXFIFO_FULL_INT_CLR);
}

static void uart0_intr_rx_handler(struct uart0 *uart, BaseType_t *task_woken)
{
  uint8_t buf[UART0_RX_BUFFERING];
  size_t size = xStreamBufferSpacesAvailable(uart->rx_buffer);
  size_t len = uart0.status.rxfifo_cnt;

  if (len == 0) {
    LOG_ISR_DEBUG("rx fifo empty");

    uart0_intr_rx_clear();

    return;
  }

  if (size == 0) {
    LOG_ISR_DEBUG("rx buffer full");

    // wait until UART_RXFIFO_OVF_INT_ST to reset FIFO
    uart0_intr_rx_disable();
    uart0_intr_rx_clear();

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
    buf[i] = uart0.fifo.rw_byte;
  }

  if ((size = xStreamBufferSendFromISR(uart->rx_buffer, buf, len, task_woken)) < len) {
    // should never happen per xStreamBufferSpacesAvailable()
    LOG_ISR_DEBUG("xStreamBufferSendFromISR len=%u: size=%u", len, size);

    uart->rx_overflow = true;

    uart0_intr_rx_flush(uart, task_woken);
  }

  uart0_intr_rx_clear();
}

void uart0_intr_handler(void *ctx)
{
  struct uart0 *uart = ctx;
  uint32_t int_st = uart0.int_st.val;
  BaseType_t task_woken = pdFALSE;

  if (int_st & UART_RXFIFO_OVF_INT_ST) {
    uart0_intr_rx_overflow_handler(uart, &task_woken);
  }
  if (int_st & (UART_FRM_ERR_INT_ST | UART_PARITY_ERR_INT_ST)) {
    uart0_intr_rx_error_handler(uart, &task_woken);
  }
  if (int_st & UART_BRK_DET_INT_ST) {
    uart0_intr_rx_break_handler(uart, &task_woken);
  }
  if (int_st & (UART_RXFIFO_TOUT_INT_ST | UART_RXFIFO_FULL_INT_ST)) {
    uart0_intr_rx_handler(uart, &task_woken);
  }

  if (task_woken == pdTRUE) {
    portYIELD_FROM_ISR();
  }
}

int uart0_intr_init(struct uart0 *uart)
{
  esp_err_t err;

  taskENTER_CRITICAL();

  // clear all interrupts
  uart0.int_clr.val = UART0_INTR_MASK;

  // clear enable mask
  uart0.int_ena.val = 0;

  taskEXIT_CRITICAL();

  // use standard uart driver ISR for uart0/1 muxing
  if ((err = uart_isr_register(UART_NUM_0, uart0_intr_handler, uart))) {
    LOG_ERROR("uart_isr_register: %s", esp_err_to_name(err));
  }

  return 0;
}

void uart0_intr_setup(struct uart0_options options)
{
  if (!options.rx_buffering) {
    options.rx_buffering = UART0_RX_BUFFERING;
  }

  if (!options.rx_timeout) {
    options.rx_timeout = UART0_RX_TIMEOUT;
  }

  taskENTER_CRITICAL();

  uart0.conf1.rxfifo_full_thrhd = options.rx_buffering;

  if (options.rx_timeout > 0) {
    uart0.conf1.rx_tout_thrhd = options.rx_timeout;
    uart0.conf1.rx_tout_en = 1;
  } else {
    uart0.conf1.rx_tout_thrhd = 0;
    uart0.conf1.rx_tout_en = 0;
  }

  uart0_intr_rx_enable();

  taskEXIT_CRITICAL();
}
