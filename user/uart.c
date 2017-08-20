#include <drivers/uart.h>

#include "user_config.h"

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
