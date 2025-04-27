#pragma once

#include <dmx.h>
#include <uart.h>

// fit one complete DMX frame into the uart RX/TX buffers
#define DMX_UART_RX_BUFFER_SIZE (512 + 1)
#define DMX_UART_TX_BUFFER_SIZE (512 + 1)

#define DMX_UART_MTBP_UNIT (8 * (1000000 / 250000)) // us per frame
#define DMX_UART_MTBP_MIN (4 * DMX_UART_MTBP_UNIT) // 128us
#define DMX_UART_MTBP_MAX (UART_RX_TIMEOUT_MAX * DMX_UART_MTBP_UNIT)

// SOC supports mapping IO pins
#define DMX_UART_IO_PINS_SUPPORTED UART_IO_PINS_SUPPORTED

struct dmx_uart_options {
  // buffer RX FIFO until line idle for this many us
  // this must be short enough to trigger in the MTBP, or the final bytes in the packet will be lost...
  uint16_t mtbp_min; // us

  // Acquire mutex before setting dev interrupts
  SemaphoreHandle_t dev_mutex;

  // Acquire mutex before setting pin funcs
  SemaphoreHandle_t pin_mutex;

#if DMX_UART_IO_PINS_SUPPORTED
  // -1 to disable, 0 to use iomux direct io
  gpio_num_t rx_pin, tx_pin;
#endif
};

int dmx_uart_setup(struct uart *uart, struct dmx_uart_options options);
