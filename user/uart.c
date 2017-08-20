#include <drivers/uart.h>
#include <stdio.h>

#include "uart.h"
#include "user_config.h"
#include "logging.h"

#define UART_RX_STACK 256

struct uart {
  char tx_buf[1024];
  size_t tx_off;
} uart;

void uart_flush()
{
  // TODO: blocking?
  UART_Write(UART0, uart.tx_buf, uart.tx_off);

  uart.tx_off = 0;
}

int uart_vprintf(const char *fmt, va_list vargs)
{
  char *tx_ptr = uart.tx_buf + uart.tx_off;
  size_t tx_size = sizeof(uart.tx_buf) - uart.tx_off;
  int ret;

  if ((ret = vsnprintf(tx_ptr, tx_size, fmt, vargs)) < 0) {
    // error
  } else if (ret < tx_size) {
    uart.tx_off += ret;
    uart_flush();
  } else {
    // overflow, write truncated...
    uart_flush();
  }

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
  xQueueHandle rx_queue = arg;
  static bool rx_overflow;
  uint32 intr_status = UART_GetIntrStatus(UART0);
  portBASE_TYPE task_woken;

  if (intr_status & (UART_RXFIFO_FULL_INT_ST | UART_RXFIFO_TOUT_INT_ST)) {
    struct uart_rx_event rx_event = {.flags = 0};

    if (rx_overflow) {
      rx_event.flags |= UART_RX_OVERFLOW;
    }

    // must read to clear the interrupt, even if queue overflow
    rx_event.len = UART_Read(UART0, rx_event.buf, sizeof(rx_event.buf));

    if (xQueueSendFromISR(rx_queue, &rx_event, &task_woken) <= 0) {
      rx_overflow = true;
    } else {
      rx_overflow = false;
    }
  }

  if (intr_status & (UART_RXFIFO_OVF_INT_ST)) {
    // lost data after last read
    rx_overflow = true;
  }

  UART_ClearIntrStatus(UART0, UART_RXFIFO_OVF_INT_ST | UART_RXFIFO_FULL_INT_ST | UART_RXFIFO_TOUT_INT_ST);

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

  UART_WaitTxFifoEmpty(UART0);
  UART_Setup(UART0, &uart_config);

  printf("INFO uart: setup baud=%u\n", uart_config.baud_rate);

  return 0;
}

int uart_start_recv(xQueueHandle rx_queue)
{
  UART_IntrConfig intr_config = {
    .enable_mask        = UART_RXFIFO_FULL_INT_ENA | UART_RXFIFO_OVF_INT_ST | UART_RXFIFO_TOUT_INT_ENA,
    .rx_full_thresh     = 32, // bytes
    .rx_timeout_thresh  = 2, // time = 8-bits/baud-rate
  };
  UART_SetupIntr(UART0, &intr_config);

  UART_RegisterIntrHandler(&uart_intr_handler, rx_queue);

  ETS_UART_INTR_ENABLE();

  return 0;
}
