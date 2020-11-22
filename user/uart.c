#include "uart.h"

#include <lib/uart.h>
#include <lib/logging.h>

int init_uart()
{
  int err;

  UART_Config uart_config = {
    .baud_rate  = UART_BAUD_RATE,
    .data_bits  = UART_WordLength_8b,
    .parity     = UART_Parity_None,
    .stop_bits  = UART_StopBits_1,
  };

  if ((err = uart_init(&uart0, UART_TX_QUEUE_SIZE, UART_RX_QUEUE_SIZE))) {
    LOG_ERROR("uart_init uart0");
    return err;
  }

  // TODO: move to dmx module?
  if ((err = uart_init(&uart1, UART_TX_QUEUE_SIZE, 0))) {
    LOG_ERROR("uart_init uart1");
    return err;
  }

  if ((err = uart_setup(&uart0, &uart_config))) {
    LOG_ERROR("uart_setup uart0");
    return err;
  }

  uart_disable(&uart1);

  uart_interrupts_enable();

  LOG_INFO("setup uart0 baud=%u", uart_config.baud_rate);

  return 0;
}
