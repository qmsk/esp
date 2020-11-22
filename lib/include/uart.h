#ifndef __LIB_UART_H__
#define __LIB_UART_H__

#include <drivers/uart_config.h>

#include <stddef.h>
#include <stdint.h>

#define UART_IO_SIZE 32
#define UART_TX_QUEUE_SIZE 32
#define UART_RX_QUEUE_SIZE 8

struct uart;

enum uart_event_type {
  UART_IO,
  UART_RX_OVERFLOW, // data was lost before this event
  UART_TX_BREAK, // send break
};

struct uart_io {
  uint16_t len;
  uint8_t buf[UART_IO_SIZE];
};

struct uart_event {
  enum uart_event_type type;
  union {
    struct uart_io io;
    struct uart_tx_break {
      uint16_t break_us, mark_us;
    } tx_break;
  };
};

extern struct uart uart0;
extern struct uart uart1;

/** Initialize uart state
 *
 * @return <0 on error
 */
 int uart_init(struct uart *uart, size_t tx_queue, size_t rx_queue);

/** Configure UART for RX/TX
 *
 * @param uart configure UART
 * @param uart_config
 * @return <0 on error
 */
int uart_setup(struct uart *uart, const UART_Config *uart_config);

/** Disable interrupts for uart.
 *
 */
int uart_disable(struct uart *uart);

/** Write one byte to UART.
 *
 * Yields the task if the TX buffer is full.
 *
 * @param uart write to UART
 * @param c byte to copy to TX buffer
 * @return 0 if copied, >0 on timeout/abort if TX buffer is full
 */
int uart_putc(struct uart *uart, int c);

/** Write len bytes from buffer to UART.
 *
 * Yields the task if the TX buffer is full.
 *
 * @param uart write to UART
 * @param ptr copy bytes from ptr to TX buffer
 * @param len number of bytes to copy
 * @return number of bytes written, 0 on timeout/abort if Tx buffer is full
 */
size_t uart_write(struct uart *uart, const void *buf, size_t len);

/** Read up to size bytes from UART to buf.
 *
 * Must be prepared to read at least UART_EVENT_SIZE bytes!
 * Yields the task if the RX buffer is empty.
 *
 * @param uart read from UART
 * @param buf copy bytes from rx buffer to ptr
 * @param size maximum number of bytes to copy; must be at least UART_EVENT_SIZE
 * @return <0 on error, 0 on timeuot/abort if RX buffer is empty, >0 number of bytes read
 */
int uart_read(struct uart *uart, void *buf, size_t size);

/** Send line break (idle low).
 *
 * XXX: This should only be used for short breaks <1ms: it blocks the TX interrupt.
 * TODO: better timing accuracy: with 128+12us actual measured 134+23us
 *
 * @param uart break on uart
 * @param break_us break (low) for us delay
 * @param mark_us mark (high) for us delay
 */
int uart_break(struct uart *uart, uint16_t break_us, uint16_t mark_us);

/**
 * Enable global UART interrupts.
 */
void uart_interrupts_enable();

#endif
