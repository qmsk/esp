#ifndef __USER_UART_H__
#define __USER_UART_H__

#include <stdarg.h>

int uart_printf(const char *fmt, ...);
int uart_vprintf(const char *fmt, va_list vargs);

#endif
