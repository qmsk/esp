#pragma once

#include <dmx.h>
#include <uart.h>

// fit one complete DMX frame into the uart RX/TX buffers
#define DMX_UART_RX_BUFFER_SIZE (512 + 1)
#define DMX_UART_TX_BUFFER_SIZE (512 + 1)

static const struct uart_options dmx_uart_options = {
  .baud_rate   = UART_BAUD_250000,
  .data_bits   = UART_DATA_8_BITS,
  .parity_bits = UART_PARITY_DISABLE,
  .stop_bits   = UART_STOP_BITS_2,
};
