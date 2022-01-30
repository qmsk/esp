#include "../os.h"

#include <esp_rom_uart.h>

void os_write(const char *data, size_t size)
{
  for (size_t len = 0; len < size; len++) {
    esp_rom_uart_tx_one_char(*data++);
  }
}
