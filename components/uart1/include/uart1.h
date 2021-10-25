#ifndef __UART1_H__
#define __UART1_H__

#include <stdbool.h>
#include <stddef.h>
#include <sys/types.h>
#include <esp8266/eagle_soc.h>

/*
 * NOTE: uart1 is only safe for use by a single task.
 */
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
};

int uart1_new(struct uart1 **uart1p, struct uart1_options options, size_t tx_buffer_size);

/*
 * Acquire mutex on uart, flush to ensure idle, and set mode.
 */
int uart1_open(struct uart1 *uart1, struct uart1_options options);

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
/*
 * Write len bytes from buf. Blocks if TX buffer is full.
 *
 * Returns 0, or <0 on error.
 */
ssize_t uart1_write_all(struct uart1 *uart1, const void *buf, size_t len);

/*
 * Wait for TX buffer + FIFO to empty.
 */
int uart1_flush(struct uart1 *uart1);

/*
 * Send break once TX completes, and hold for >= break_us.
 * After break, hold mark for >= mark_us.
 *
 * Blocks until TX, break and mark are complete.
 * Timing is approximate, not bit-exact.
 *
 * Return <0 on error.
 */
int uart1_break(struct uart1 *uart1, unsigned break_us, unsigned mark_us);

/*
 * Flush TX and release mutex on uart.
 */
int uart1_close(struct uart1 *uart1);

#endif
