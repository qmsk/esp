#include "ets.h"

#include <esp_libc.h>

void ets_write(const char *data, size_t size)
{
  size_t len;

  for (len = 0; len < size; len++) {
    ets_putc(*data++);
  }
}
