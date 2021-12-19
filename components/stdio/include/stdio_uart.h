#pragma once

#include <uart.h>

/**
 * Use UART driver for stdio read/write.
 */
void stdio_attach_uart(struct uart *uart);

/**
 * Stop using UART driver.
 */
void stdio_detach_uart();
