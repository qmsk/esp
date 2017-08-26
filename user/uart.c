#include <drivers/uart.h>
#include <stdio.h>

#include "uart.h"
#include "user_config.h"
#include "logging.h"

#define UART_TX_BUFFER 1024

struct uart {
  xQueueHandle rx_queue;
  bool rx_overflow;

  char tx_buf[UART_TX_BUFFER], *tx_ptr;
} uart;

static void uart_tx() // ISR UART_TXFIFO_EMPTY_INT_ST
{
  size_t len = uart.tx_ptr - uart.tx_buf;
  size_t write;

  write = UART_Write(UART0, uart.tx_buf, len);

  if (write < len) {
    memmove(uart.tx_buf, uart.tx_buf + write, len - write);

    uart.tx_ptr -= write;

    uart.tx_buf[0] = '#'; // XXX: TX overflow marker

    // buffer waiting for tx fifo empty
    UART_EnableIntr(UART0, UART_TXFIFO_EMPTY_INT_ENA);

  } else {
    uart.tx_ptr = uart.tx_buf;

    // buffer empty
    UART_DisableIntr(UART0, UART_TXFIFO_EMPTY_INT_ENA);
  }
}

static void uart_rx(portBASE_TYPE *task_wokenp) // ISR UART_RXFIFO_FULL_INT_ST | UART_RXFIFO_TOUT_INT_ST
{
  struct uart_rx_event rx_event = {.flags = 0};

  if (uart.rx_overflow) {
    rx_event.flags |= UART_RX_OVERFLOW;
  }

  // must read to clear the interrupt, even if queue overflow
  rx_event.len = UART_Read(UART0, rx_event.buf, sizeof(rx_event.buf));

  if (!uart.rx_queue) {
    uart.rx_overflow = true;
  } else if (xQueueSendFromISR(uart.rx_queue, &rx_event, task_wokenp) <= 0) {
    uart.rx_overflow = true;
  } else {
    uart.rx_overflow = false;
  }
}

static void uart_rx_overflow() // ISR UART_RXFIFO_OVF_INT_ST
{
  // lost data after last read
  uart.rx_overflow = true;
}

int uart_putc(char c) // XXX: unused
{
  int err = 0;

  if (uart.tx_ptr < uart.tx_buf + sizeof(uart.tx_buf)) {
    *uart.tx_ptr++ = c;
  } else {
    err = -1;
  }

  uart_tx();

  return err;
}

int uart_vprintf(const char *fmt, va_list vargs)
{
  size_t tx_size = uart.tx_buf + sizeof(uart.tx_buf) - uart.tx_ptr;
  int ret;

  if (tx_size == 0) {
    ret = -1;
    uart.tx_ptr[-1] = '\\'; // XXX: print overflow marker
  } else if ((ret = vsnprintf(uart.tx_ptr, tx_size, fmt, vargs)) < 0) {
    // error
  } else if (ret < tx_size) {
    uart.tx_ptr += ret;
  } else {
    // overflow, write truncated...
    uart.tx_ptr = uart.tx_buf + sizeof(uart.tx_buf);
    uart.tx_ptr[-1] = '\\'; // XXX: print overflow marker
  }
  uart_tx();

  return ret;
}

int uart_printf(const char *fmt, ...)
{
  int ret;
  va_list vargs;

  va_start(vargs, fmt);
  ret = uart_vprintf(fmt, vargs);
  va_end(vargs);

  return ret;
}

static void uart_intr_handler(void *arg)
{
  uint32 intr_status = UART_GetIntrStatus(UART0);
  portBASE_TYPE task_woken;

  if (intr_status & (UART_TXFIFO_EMPTY_INT_ST)) {
    uart_tx();
  }

  if (intr_status & (UART_RXFIFO_FULL_INT_ST | UART_RXFIFO_TOUT_INT_ST)) {
    uart_rx(&task_woken);
  }

  if (intr_status & (UART_RXFIFO_OVF_INT_ST)) {
    uart_rx_overflow();
  }

  UART_ClearIntrStatus(UART0, UART_TXFIFO_EMPTY_INT_CLR | UART_RXFIFO_OVF_INT_CLR | UART_RXFIFO_FULL_INT_CLR | UART_RXFIFO_TOUT_INT_CLR);

  portEND_SWITCHING_ISR(task_woken);
}

int init_uart(struct user_config *config)
{
  UART_Config uart_config = {
    .baud_rate  = USER_CONFIG_UART_BAUD_RATE,
    .data_bits  = UART_WordLength_8b,
    .parity     = USART_Parity_None,
    .stop_bits  = USART_StopBits_1,
    .flow_ctrl  = USART_HardwareFlowControl_None,
    .flow_rx_thresh = 0,
    .inverse_mask = UART_None_Inverse,
  };
  UART_IntrConfig intr_config = {
    .enable_mask        = UART_RXFIFO_FULL_INT_ENA | UART_RXFIFO_OVF_INT_ENA | UART_RXFIFO_TOUT_INT_ENA, // XXX: zero INTR_ENA mask bugs out
    .rx_full_thresh     = UART_RX_EVENT_SIZE, // more than bytes in rx fifo
    .rx_timeout_thresh  = 2, // time = 8-bits/baud-rate
    .tx_empty_thresh    = 32, // fewer than bytes in tx fifo
  };

  uart.tx_ptr = uart.tx_buf;

  UART_WaitTxFifoEmpty(UART0);
  UART_Setup(UART0, &uart_config);
  UART_SetupIntr(UART0, &intr_config);

  UART_RegisterIntrHandler(&uart_intr_handler, NULL);

  printf("INFO uart: setup baud=%u\n", uart_config.baud_rate);

  ETS_UART_INTR_ENABLE();

  return 0;
}

int uart_start_recv(xQueueHandle rx_queue)
{
  // direct uart_rx() ISR
  uart.rx_queue = rx_queue;

  return 0;
}
