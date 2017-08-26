#ifndef __USER_UART_H__
#define __USER_UART_H__

#include "user_config.h"
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

#define UART_EVENT_SIZE 32
#define UART_TX_QUEUE_SIZE 32

enum uart_rx_flags {
  UART_RX_OVERFLOW = 1, // data was lost before this event
};

struct uart_event {
  uint16_t flags;
  uint16_t len;
  uint8_t buf[UART_EVENT_SIZE];
};

int init_uart(struct user_config *config);

/** Write one byte to UART.
 *
 * Yields the task if the TX buffer is full.
 *
 * @param c byte to copy to tx buffer
 * @return 0 if copied, >0 if tx buffer is full
 */
int uart_putc(int c);

/** Write len bytes from buffer to UART.
 *
 * Yields the task if the TX buffer is full.
 *
 * @param ptr copy bytes from ptr to tx buffer
 * @param len number of bytes to copy
 * @return number of bytes written, 0 if tx buffer is full
 */
size_t uart_write(const void *ptr, size_t len);

/** Start reading UART data to `struct uart_rx_event` queue.
 *
 * @param rx_queue <struct uart_event>
 */
int uart_start_recv(xQueueHandle rx_queue);

#endif
