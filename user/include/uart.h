#ifndef __USER_UART_H__
#define __USER_UART_H__

#include "user_config.h"
#include <stdarg.h>

int uart_printf(const char *fmt, ...);
int uart_vprintf(const char *fmt, va_list vargs);

int init_uart(struct user_config *config);

#endif
