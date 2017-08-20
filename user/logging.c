#include <drivers/uart.h>
#include <esp_misc.h>

#include "logging.h"
#include "uart.h"

const static char logging_os_prefix[] = { 'O', 'S', ':', ' '};

struct logging {
  bool os_write;
} logging;

void logging_os_putc(char c)
{
  if (!logging.os_write) {
    logging.os_write = true;

    UART_Write(UART0, logging_os_prefix, sizeof(logging_os_prefix));
  }

  UART_WriteOne(UART0, c);

  if (c == '\n') {
    logging.os_write = false;
  }
}

void logging_printf(const char *prefix, const char *func, const char *fmt, ...)
{
  va_list vargs;

  va_start(vargs, fmt);
  uart_printf("%s%s: ", prefix, func);
  uart_vprintf(fmt, vargs);
  uart_printf("\n");
  va_end(vargs);
}

int init_logging(struct user_config *config)
{
  os_install_putc1(logging_os_putc);

  LOG_INFO("initialized");

  return 0;
}
