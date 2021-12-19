#include <stdio_uart.h>
#include "uart.h"

struct uart *stdio_uart;

void stdio_attach_uart(struct uart *uart)
{
  stdio_uart = uart;
}

void stdio_detach_uart()
{
  stdio_uart = NULL;
}
