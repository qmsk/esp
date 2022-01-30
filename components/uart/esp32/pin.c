#include <uart.h>
#include "../uart.h"

#include <logging.h>

#include <hal/gpio_ll.h>
#include <soc/uart_channel.h>

int uart_pin_setup(struct uart *uart, struct uart_options options)
{
  int err = 0;

  if (options.pin_mutex) {
    LOG_DEBUG("wait pin_mutex=%p", options.pin_mutex);

    if (!xSemaphoreTake(options.pin_mutex, portMAX_DELAY)) {
      LOG_ERROR("xSemaphoreTake");
      return -1;
    } else {
      uart->pin_mutex = options.pin_mutex;
    }
  }

  LOG_DEBUG("port=%d", (uart->port & UART_PORT_MASK));

  taskENTER_CRITICAL(&uart->mux);

  switch(uart->port & UART_PORT_MASK) {
    case UART_0:
      gpio_ll_iomux_func_sel(PERIPHS_IO_MUX_U0TXD_U, FUNC_U0TXD_U0TXD);
      gpio_ll_iomux_func_sel(PERIPHS_IO_MUX_U0RXD_U, FUNC_U0RXD_U0RXD);

      gpio_ll_iomux_in(&GPIO, UART_NUM_0_RXD_DIRECT_GPIO_NUM, U0RXD_IN_IDX);

      break;

    case UART_1:
      // XXX: not compatible with flash qio mode?
      gpio_ll_iomux_func_sel(PERIPHS_IO_MUX_SD_DATA2_U, FUNC_SD_DATA2_U1RXD);
      gpio_ll_iomux_func_sel(PERIPHS_IO_MUX_SD_DATA3_U, FUNC_SD_DATA3_U1TXD);

      gpio_ll_iomux_in(&GPIO, UART_NUM_1_RXD_DIRECT_GPIO_NUM, U1RXD_IN_IDX);

      break;

    case UART_2:
      gpio_ll_iomux_func_sel(PERIPHS_IO_MUX_GPIO16_U, FUNC_GPIO16_U2RXD);
      gpio_ll_iomux_func_sel(PERIPHS_IO_MUX_GPIO17_U, FUNC_GPIO17_U2TXD);

      gpio_ll_iomux_in(&GPIO, UART_NUM_2_RXD_DIRECT_GPIO_NUM, U2RXD_IN_IDX);

      break;

    default:
      err = -1;
      break;
  }

  taskEXIT_CRITICAL(&uart->mux);

  return err;
}

void uart_pin_teardown(struct uart *uart)
{
  if (uart->pin_mutex) {
    xSemaphoreGive(uart->pin_mutex);

    uart->pin_mutex = NULL;
  }
}
