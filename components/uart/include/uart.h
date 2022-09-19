#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

#include <stdbool.h>
#include <stddef.h>
#include <sys/types.h>

#if CONFIG_IDF_TARGET_ESP8266

  typedef int uart_port_t;

  #define UART_0    (0) // GPIO1 TX, GPIO3 RX
  #define UART_1    (1) // GPIO2 TX

  // number of physical UART ports
  #define UART_PORT_MAX   (2)
  #define UART_PORT_MASK  0x0ff

  #define UART_IO_PINS_SUPPORTED 0

  // flag bits
  #define UART_SWAP_BIT   0x100   // swap RTS/CST AND RX/TX pins
  #define UART_TXDBK_BIT  0x200   // use alternate TX pin
  #define UART_RXONLY_BIT 0x400   // skip TX pin
  #define UART_TXONLY_BIT 0x800   // skip RX pin

  #define UART_0_SWAP         (UART_0 | UART_SWAP_BIT)                    // GPIO15 TX, GPIO13 RX
  #define UART_0_SWAP_RXONLY  (UART_0 | UART_SWAP_BIT | UART_RXONLY_BIT)  // GPIO13 RX
  #define UART_0_SWAP_TXONLY  (UART_0 | UART_SWAP_BIT | UART_TXONLY_BIT)  // GPIO15 TX
  #define UART_0_TXDBK        (UART_0 | UART_TXDBK_BIT)                   // GPIO1+GPIO2 TX, GPIO3 RX

  typedef enum {
    UART_DATA_5_BITS = 0,
    UART_DATA_6_BITS = 1,
    UART_DATA_7_BITS = 2,
    UART_DATA_8_BITS = 3,
  } uart_word_length_t;

  typedef enum {
    UART_PARITY_DISABLE = 0x0,
    UART_PARITY_EVEN    = 0x2,
    UART_PARITY_ODD     = 0x3,
  } uart_parity_t;

  typedef enum {
    UART_STOP_BITS_1   = 0x1,
    UART_STOP_BITS_1_5 = 0x2,
    UART_STOP_BITS_2   = 0x3,
  } uart_stop_bits_t;

#elif CONFIG_IDF_TARGET_ESP32
  #include <hal/uart_types.h>
  #include <hal/gpio_types.h>

  // uart_port_t values
  #define UART_0  (0)     // GPIO1 TX, GPIO3 RX
  #define UART_1  (1)     // GPIO10 TX, GPIO9 RX
  #define UART_2  (2)     // GPIO17 TX, GPIO16 RX

  // number of physical UART ports
  #define UART_PORT_MAX 3
  #define UART_PORT_MASK  0x0ff

  #define UART_IO_PINS_SUPPORTED 1

#else
  #error Unsupported target
#endif

typedef enum {
  UART_BAUD_115200  = 115200,
  UART_BAUD_250000  = 250000,
  UART_BAUD_2500000 = 2500000,
  UART_BAUD_3333333 = 3333333,
  UART_BAUD_4000000 = 4000000,
} uart_baud_t;

#define UART_RX_TIMEOUT_MAX ((1 << 7) - 1)

struct uart_options {
  uart_baud_t baud_rate;
  uart_word_length_t data_bits;
  uart_parity_t parity_bits;
  uart_stop_bits_t stop_bits;

  // flush RX buffers after timeout frames (~8 bits) idle
  //  1 -> instant (interrupt on each frame)
  uint32_t rx_timeout : 7;

  // flush RX buffers after buffered frames (start/data/stop bits) available
  //  1 -> unbuffered (interrupt on each frame)
  uint32_t rx_buffered : 7;

  // invert RX signals
  uint32_t rx_inverted : 1;

  // invert TX signals
  uint32_t tx_inverted : 1;

  // optional read() timeout, 0 -> portMAX_DELAY
  TickType_t read_timeout;

  // Acquire mutex before setting dev interrupts
  SemaphoreHandle_t dev_mutex;

  // Acquire mutex before setting pin funcs
  SemaphoreHandle_t pin_mutex;

#ifdef UART_IO_PINS_SUPPORTED
  // -1 to disable, 0 to use iomux direct io
  gpio_num_t rx_pin, tx_pin;
#endif
};

struct uart;

/**
 * Allocate memory for UART driver.
 *
 * Does not touch the UART device.
 */
int uart_new(struct uart **uartp, uart_port_t port, size_t rx_buffer_size, size_t tx_buffer_size);

/**
 * Setup UART interrupts, flush TX, setup UART device, reset RX.
 */
int uart_setup(struct uart *uart, struct uart_options options);

/**
 * Setup, and keep rx/tx mutex acquired for calling task.
 *
 * Use uart_close() to release.
 */
int uart_open(struct uart *uart, struct uart_options options);

/**
 * Acquire RX mutex for calling task. The UART must be setup.
 *
 * Use uart_close_rx() to release.
 *
 * @return <0 on error, 0 on success, >0 if UART not setup.
 */
int uart_open_rx(struct uart *uart);

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
 * @return -EOVERFLOW RX FIFO full
 * @return -EBADMSG RX framing/parity error
 * @return -ESPIPE RX break desynchronized, try reducing `rx_timeout` or other interrupt load
 * @return -EINTR interrupted using uart_abort_read()
 * @return -EINVAL RX disabled (rx_buffer_size=0)
 */
int uart_read(struct uart *uart, void *buf, size_t size);

/**
 * Cause the following uart_read() call, or any currently pending call, to return an error.
 *
 * TODO: make this persistent until close/open?
 * XXX: interrupt of currently pending uart_read() call is not implemented on ESP32, it will continue blocking
 */
int uart_abort_read(struct uart *uart);

/**
 * Releaes RX mutex for calling task.
 */
int uart_close_rx(struct uart *uart);

/**
 * Acquire TX mutex for calling task. The UART must be setup.
 *
 * Use uart_close_tx() to release.
 *
 * @return <0 on error, 0 on success, >0 if UART not setup.
 */
int uart_open_tx(struct uart *uart);

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
 * Flush -> idle, hold line low for >= `break_bits` bauds to signal break, and return line high (idle) for >= `mark_bits`.
 *
 * Blocks until TX idle, break and mark are complete.
 * Timing is approximate, not bit-exact.
 *
 * Return <0 on error.
 */
int uart_break(struct uart *uart, unsigned break_bits, unsigned mark_bits);

/**
 * Flush, and hold mark for >= `mark_bits` bauds once TX is complete.
 *
 * Timing is approximate, not bit-exact.
 *
 * Return <0 on error.
 */
int uart_mark(struct uart *uart, unsigned mark_bits);

/**
 * Flush TX and release TX mutex acquire dusing `uart_open_tx()`.
 */
int uart_close_tx(struct uart *uart);

/**
 * Flush TX/RX and release rx/tx mutex acquired using `uart_open()`.
 *
 * WARNING: This does not release any intr, dev or pin state acquired by `uart_open()` -> `uart_setup()`! Use `uart_teardown()`!
 */
int uart_close(struct uart *uart);

/*
 * Stop UART interrupts and disassocate from UART device.
 */
int uart_teardown(struct uart *uart);
