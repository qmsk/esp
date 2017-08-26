#include <stdarg.h>
#include <drivers/uart.h>
#include <esp_misc.h>

#include "logging.h"
#include "uart.h"

#define LOGGING_BUFFER 128

struct logging {
  bool os_write;
  char buf[LOGGING_BUFFER];
} logging;

void logging_os_putc(char c)
{
  if (!logging.os_write) {
    logging.os_write = true;

    UART_WriteOne(UART0, 'O');
    UART_WriteOne(UART0, 'S');
    UART_WriteOne(UART0, ':');
    UART_WriteOne(UART0, ' ');
  }

  UART_WriteOne(UART0, c);

  if (c == '\n') {
    logging.os_write = false;
  }
}

void logging_printf(const char *prefix, const char *func, const char *fmt, ...)
{
  va_list vargs;
  char *buf = logging.buf;
  char *ptr = logging.buf;
  char *end = logging.buf + sizeof(logging.buf);

  va_start(vargs, fmt);
  if (ptr < end)
    ptr += snprintf(ptr, end - ptr, "%s%s: ", prefix, func);
  if (ptr < end)
    ptr += vsnprintf(ptr, end - ptr, fmt, vargs);
  if (ptr < end)
    ptr += snprintf(ptr, end - ptr, "\n");
  va_end(vargs);

  if (ptr >= end) {
    ptr = end - 2;
    *ptr++ = '\\';
    *ptr++ = '\n';
  }

  // XXX: blocking
  while (buf < ptr) {
    buf += uart_write(buf, ptr - buf);
  }
}

int init_logging(struct user_config *config)
{
  os_install_putc1(logging_os_putc);

  LOG_INFO("initialized");

  return 0;
}
