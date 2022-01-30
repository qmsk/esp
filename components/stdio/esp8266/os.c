#include "../os.h"

#include <esp_libc.h>

void os_write(const char *data, size_t size)
{
  for (size_t len = 0; len < size; len++) {
    ets_putc(*data++);
  }
}
