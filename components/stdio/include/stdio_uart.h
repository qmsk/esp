#pragma once

#include <uart.h>

/**
 * Use UART driver for stdio read/write.
 */
int stdio_attach_uart(struct uart *uart);
