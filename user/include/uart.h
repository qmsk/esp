#ifndef __USER_UART_H__
#define __USER_UART_H__

#include "user_config.h"
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

#define UART_IO_SIZE 32
#define UART_TX_QUEUE_SIZE 32
#define UART_RX_QUEUE_SIZE 8

struct uart;

enum uart_io_flags {
  UART_RX_OVERFLOW = 1, // data was lost before this event
};

struct uart_io {
  uint16_t flags;
  uint16_t len;
  uint8_t buf[UART_IO_SIZE];
};

extern struct uart uart0;

int init_uart(struct user_config *config);

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

#endif
