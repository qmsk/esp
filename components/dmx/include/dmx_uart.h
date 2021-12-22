#pragma once

#include <dmx.h>
#include <uart.h>

static const struct uart_options dmx_uart_options = {
  .clock_div   = UART_BAUD_250000,
  .data_bits   = UART_DATA_BITS_8,
  .parity_bits = UART_PARITY_DISABLE,
  .stop_bits   = UART_STOP_BITS_2,
};
