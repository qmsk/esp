#include <stdio_uart.h>
#include "uart.h"

struct uart *stdio_uart;

int stdio_attach_uart(struct uart *uart)
{
  stdio_uart = uart;

  return 0;
}
