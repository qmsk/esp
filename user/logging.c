#include <drivers/uart.h>
#include <esp_misc.h>

const char logging_os_prefix[] = { 'O', 'S', ':', ' '};

struct logging {
  bool os_newline;
} logging;

void logging_os_putc(char c)
{
  if (logging.os_newline) {
    logging.os_newline = false;

    UART_Write(UART0, logging_os_prefix, sizeof(logging_os_prefix));
  }

  UART_WriteOne(UART0, c);

  if (c == '\n') {
    logging.os_newline = true;
  }
}

int init_logging()
{
  os_install_putc1(logging_os_putc);

  printf("INFO logging: installed os handler\n");

  return 0;
}
