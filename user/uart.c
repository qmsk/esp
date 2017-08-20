#include <drivers/uart.h>
#include <stdio.h>

#include "uart.h"
#include "user_config.h"

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
