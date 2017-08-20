#ifndef __USER_UART_H__
#define __USER_UART_H__

#include "user_config.h"
#include <stdarg.h>
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

int uart_printf(const char *fmt, ...);
int uart_vprintf(const char *fmt, va_list vargs);

int init_uart(struct user_config *config);

/** Start reading UART data to `struct uart_rx_event` queue.
 *
 */
int uart_start_recv(xQueueHandle rx_queue);

#endif
