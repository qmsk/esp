#ifndef __UART1_H__
#define __UART1_H__

#include <stdbool.h>
#include <stddef.h>
#include <sys/types.h>
#include <esp8266/eagle_soc.h>

struct uart1;

enum uart1_baud_rate {
  UART1_BAUD_250000 = UART_CLK_FREQ / 250000,
};

enum uart1_data_bits {
  UART1_DATA_BITS_5 = 0,
  UART1_DATA_BITS_6 = 1,
  UART1_DATA_BITS_7 = 2,
  UART1_DATA_BITS_8 = 3,
};

enum uart1_parity_bits {
  UART1_PARTIY_DISABLE = 0x0,
  UART1_PARTIY_EVEN    = 0x2,
  UART1_PARTIY_ODD     = 0x3,
};

enum uart1_stop_bits {
  UART1_STOP_BITS_1   = 0x1,
  UART1_STOP_BITS_1_5 = 0x2,
  UART1_STOP_BITS_2   = 0x3,
};

struct uart1_options {
  enum uart1_baud_rate clock_div;
  enum uart1_data_bits data_bits;
  enum uart1_parity_bits parity_bits;
  enum uart1_stop_bits stop_bits;
  bool inverted;

  size_t tx_buffer_size;
};

int uart1_new(struct uart1 **uart1p, struct uart1_options options);

/*
 * Write one byte. Blocks if TX buffer is full.
 *
 * Returns ch on success, <0 on error.
 */
int uart1_putc(struct uart1 *uart1, int ch);

/*
 * Write up to len bytes from buf. Blocks if TX buffer is full.
 *
 * Returns number of bytes written, or <0 on error.
 */
ssize_t uart1_write(struct uart1 *uart1, const void *buf, size_t len);

#endif
