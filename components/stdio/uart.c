#include <stdio_uart.h>
#include "uart.h"

struct uart *vfs_uart;

int stdio_attach_uart(struct uart *uart)
{
  vfs_uart = uart;

  return 0;
}
