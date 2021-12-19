#pragma once

#include <esp8266/eagle_soc.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

#include <stdbool.h>
#include <stddef.h>
#include <sys/types.h>

enum uart_port {
  UART_0,     // GPIO1 TX, GPIO3 RX
  UART_1,     // GPIO2 TX

  // number of physical UART ports
  UART_PORT_MAX,

  UART_PORT_MASK  = 0x0ff,
  UART_SWAP_BIT   = 0x100,

  UART_0_SWAP     = UART_0 | UART_SWAP_BIT, // GPIO15 TX, GPIO13 RX
  // TODO: UART_0 with GPIO1+GPIO2 TX?
};

enum uart_baud_rate {
  UART_BAUD_250000   = UART_CLK_FREQ / 250000,
  UART_BAUD_2500000  = UART_CLK_FREQ / 2500000,
  UART_BAUD_3333333  = UART_CLK_FREQ / 3333333,
  UART_BAUD_4000000  = UART_CLK_FREQ / 4000000,
};

enum uart_data_bits {
  UART_DATA_BITS_5 = 0,
  UART_DATA_BITS_6 = 1,
  UART_DATA_BITS_7 = 2,
  UART_DATA_BITS_8 = 3,
};

enum uart_parity_bits {
  UART_PARITY_DISABLE = 0x0,
  UART_PARITY_EVEN    = 0x2,
  UART_PARITY_ODD     = 0x3,
};

enum uart_stop_bits {
  UART_STOP_BITS_1   = 0x1,
  UART_STOP_BITS_1_5 = 0x2,
  UART_STOP_BITS_2   = 0x3,
};

struct uart_options {
  enum uart_baud_rate clock_div;
  enum uart_data_bits data_bits;
  enum uart_parity_bits parity_bits;
  enum uart_stop_bits stop_bits;

  // flush RX buffers after timeout frames (start/data/stop bits) idle
  uint32_t rx_timeout : 7;

  // flush RX buffers after buffering frames (start/data/stop bits) available
  uint32_t rx_buffering : 7;

  // invert RX signals
  uint32_t rx_inverted : 1;

  // invert TX signals
  uint32_t tx_inverted : 1;

  // optional read() timeout, 0 -> portMAX_DELAY
  TickType_t read_timeout;

  // Acquire mutex before setting pin funcs
  SemaphoreHandle_t pin_mutex;
};

struct uart;

/**
 * Allocate memory for UART driver.
 *
 * Does not touch the UART device.
 */
int uart_new(struct uart **uartp, enum uart_port port, size_t rx_buffer_size, size_t tx_buffer_size);

/**
 * Setup UART interrupts, flush TX, setup UART device, reset RX.
 */
int uart_setup(struct uart *uart, struct uart_options options);

/**
 * Setup, but keep rx/tx lock acquired for calling task.
 *
 * Use uart_close() to release.
 */
int uart_open(struct uart *uart, struct uart_options options);

/**
 * Set timeout for read().
 *
 * @param timeout or portMAX_DELAY to disable
 */
int uart_set_read_timeout(struct uart *uart, TickType_t timeout);

/**
 * Read data from UART, copying up to size bytes into buf.
 *
 * @return <0 on error, 0 on timeout or break, otherwise number of bytes copied into buf.
 */
int uart_read(struct uart *uart, void *buf, size_t size);

/**
 * Write one byte. Blocks if TX buffer is full.
 *
 * Returns ch on success, <0 on error.
 */
int uart_putc(struct uart *uart, int ch);

/**
 * Write up to len bytes from buf. Blocks if TX buffer is full.
 *
 * Returns number of bytes written, or <0 on error.
 */
ssize_t uart_write(struct uart *uart, const void *buf, size_t len);

/**
 * Write len bytes from buf. Blocks if TX buffer is full.
 *
 * Returns 0, or <0 on error.
 */
int uart_write_all(struct uart *uart, const void *buf, size_t len);

/**
 * Write len bytes from buf into the TX buffer.
 *
 * TX is only started once the TX buffer is full, or uart_flush() is called.
 *
 * Returns 0, or <0 on error.
 */
ssize_t uart_write_buffered(struct uart *uart, const void *buf, size_t len);

/**
 * Wait for TX buffer + FIFO to empty.
 */
int uart_flush_write(struct uart *uart);

/**
 * Flush, send break and hold for >= break_us.
 * After break, hold mark for >= mark_us.
 *
 * Blocks until TX, break and mark are complete.
 * Timing is approximate, not bit-exact.
 *
 * Return <0 on error.
 */
int uart_break(struct uart *uart, unsigned break_us, unsigned mark_us);

/**
 * Flush, and hold mark for >= break_us once TX is complete.
 *
 * Timing is approximate, not bit-exact.
 *
 * Return <0 on error.
 */
int uart_mark(struct uart *uart, unsigned mark_us);

/**
 * Flush TX/RX and release rx/tx mutex acquired using `uart_open()`.
 *
 * WARNING: This does not release any pin_mutex acquired by `uart_open()`  -> `uart_setup()`! Use `uart_teardown()`!
 */
int uart_close(struct uart *uart);

/*
 * Stop UART interrupts and disassocate from UART device.
 */
int uart_teardown(struct uart *uart);
