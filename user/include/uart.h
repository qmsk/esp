#ifndef __USER_UART_H__
#define __USER_UART_H__

#include "user_config.h"
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

#define UART_RX_EVENT_SIZE 32

enum uart_rx_flags {
  UART_RX_OVERFLOW = 1, // data was lost before this event
};

struct uart_rx_event {
  uint16_t flags;
  uint16_t len;
  char buf[UART_RX_EVENT_SIZE];
};

int init_uart(struct user_config *config);

/**
 * @param c byte to copy to tx buffer
 * @return 0 if copied, >0 if tx buffer is full
 */
int uart_putc(char c);

/**
 * @param ptr copy bytes from ptr to tx buffer
 * @param len number of bytes to copy
 * @return number of bytes written, 0 if tx buffer is full
 */
int uart_write(const char *ptr, size_t len);

/** Start reading UART data to `struct uart_rx_event` queue.
 *
 * @param rx_queue <struct uart_rx_event>
 */
int uart_start_recv(xQueueHandle rx_queue);

#endif
