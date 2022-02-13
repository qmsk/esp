#pragma once

#include <dmx.h>
#include <uart.h>

// fit one complete DMX frame into the uart RX/TX buffers
#define DMX_UART_RX_BUFFER_SIZE (512 + 1)
#define DMX_UART_TX_BUFFER_SIZE (512 + 1)

#define DMX_UART_MTBP_UNIT (8 * (1000000 / 250000))
#define DMX_UART_MTBP_MIN (4 * DMX_UART_MTBP_UNIT)
#define DMX_UART_MTBP_MAX (UART_RX_TIMEOUT_MAX * DMX_UART_MTBP_UNIT)

struct dmx_uart_options {
  // buffer RX FIFO until line idle for this many ~8-bit periods
  // this must be short enough to trigger in the MTBP, or the final bytes in the packet will be lost...
  uint16_t mtbp_min; // use DMX_UART_MTBP_MIN

  // Acquire mutex before setting dev interrupts
  SemaphoreHandle_t dev_mutex;

  // Acquire mutex before setting pin funcs
  SemaphoreHandle_t pin_mutex;
};

int dmx_uart_setup(struct uart *uart, struct dmx_uart_options options);
