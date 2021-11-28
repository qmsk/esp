#ifndef __UART0_H__
#define __UART0_H__

#include <stdbool.h>
#include <stddef.h>
#include <sys/types.h>
#include <esp8266/eagle_soc.h>

struct uart0;

enum uart0_baud_rate {
  UART0_BAUD_250000   = UART_CLK_FREQ / 250000,
  UART0_BAUD_2500000  = UART_CLK_FREQ / 2500000,
  UART0_BAUD_3333333  = UART_CLK_FREQ / 3333333,
  UART0_BAUD_4000000  = UART_CLK_FREQ / 4000000,
};

enum uart0_data_bits {
  UART0_DATA_BITS_5 = 0,
  UART0_DATA_BITS_6 = 1,
  UART0_DATA_BITS_7 = 2,
  UART0_DATA_BITS_8 = 3,
};

enum uart0_parity_bits {
  UART0_PARTIY_DISABLE = 0x0,
  UART0_PARTIY_EVEN    = 0x2,
  UART0_PARTIY_ODD     = 0x3,
};

enum uart0_stop_bits {
  UART0_STOP_BITS_1   = 0x1,
  UART0_STOP_BITS_1_5 = 0x2,
  UART0_STOP_BITS_2   = 0x3,
};

struct uart0_options {
  enum uart0_baud_rate clock_div;
  enum uart0_data_bits data_bits;
  enum uart0_parity_bits parity_bits;
  enum uart0_stop_bits stop_bits;

  // flush RX buffers after timeout frames (start/data/stop bits) idle.
  uint32_t rx_timeout : 7;

  // flush RX buffers after buffering frames (start/data/stop bits) available
  uint32_t rx_buffering : 7;

  bool rx_inverted;
  bool swap; // swap GPIO13 UART0 CTS <-> GPIO3 UART0 RX
};

int uart0_new(struct uart0 **uart0p, size_t rx_buffer_size);

/*
 * Acquire mutex on uart, flush to ensure idle, and set mode.
 */
int uart0_open(struct uart0 *uart0, struct uart0_options options);

/*
 * Read data from UART, copying up to size bytes into buf.
 *
 * @return <0 on error, 0 on break, otherwise number of bytes copied into buf.
 */
int uart0_read(struct uart0 *uart0, void *buf, size_t size);

/*
 * Flush RX and release mutex on uart.
 */
int uart0_close(struct uart0 *uart0);

#endif
