#pragma once

#include <esp8266/eagle_soc.h>
#include <esp8266/uart_struct.h>
#include <esp8266/uart_register.h>

static inline void uart_dev_set_baudrate(uart_dev_t *dev, uart_baud_t baudrate)
{
  dev->clk_div.div_int = UART_CLK_FREQ / baudrate;
}

static inline uart_baud_t uart_dev_get_baudrate(uart_dev_t *dev)
{
  return UART_CLK_FREQ / dev->clk_div.div_int;
}
