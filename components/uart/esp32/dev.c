#include <uart.h>
#include "../uart.h"

#include <logging.h>

#include <driver/periph_ctrl.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <hal/uart_ll.h>
#include <soc/uart_struct.h>

#define UART_SCLK_DEFAULT (UART_SCLK_APB)
#define UART_TX_IDLE_NUM_DEFAULT (0)
#define UART_HW_FLOW_CONTROL_RX_THRS_DEFAULT (100)
#define UART_MODE_DEFAULT (UART_MODE_UART)

static uart_dev_t *uart_dev[UART_PORT_MAX] = {
  [UART_0]    = &UART0,
  [UART_1]    = &UART1,
  [UART_2]    = &UART2,
};

static const periph_module_t uart_periph_module[UART_PORT_MAX] = {
  [UART_0]    = PERIPH_UART0_MODULE,
  [UART_1]    = PERIPH_UART1_MODULE,
  [UART_2]    = PERIPH_UART2_MODULE,
};

int uart_dev_setup(struct uart *uart, struct uart_options options)
{
  if (options.dev_mutex) {
    LOG_DEBUG("take dev_mutex=%p", options.dev_mutex);

    if (!xSemaphoreTake(options.dev_mutex, portMAX_DELAY)) {
      LOG_ERROR("xSemaphoreTake");
      return -1;
    } else {
      uart->dev_mutex = options.dev_mutex;

      LOG_DEBUG("have dev_mutex=%p", uart->dev_mutex);
    }
  }

  if (!(uart->dev = uart_dev[uart->port & UART_PORT_MASK])) {
    LOG_ERROR("invalid uart_dev[%d]", (uart->port & UART_PORT_MASK));
    return -1;
  }

  LOG_DEBUG("port=%x: baud_rate=%d data_bits=%x parity_bits=%x stop_bits=%x rx_inverted=%d tx_inverted=%d",
    uart->port,
    options.baud_rate,
    options.data_bits,
    options.parity_bits,
    options.stop_bits,
    options.rx_inverted, options.tx_inverted
  );

  taskENTER_CRITICAL(&uart->mux);

  periph_module_enable(uart_periph_module[uart->port & UART_PORT_MASK]);

  uart_ll_set_sclk(uart->dev, UART_SCLK_DEFAULT);
  uart_ll_set_baudrate(uart->dev, options.baud_rate);
  uart_ll_set_stop_bits(uart->dev, options.stop_bits);
  uart_ll_set_parity(uart->dev, options.parity_bits);
  uart_ll_set_tx_idle_num(uart->dev, UART_TX_IDLE_NUM_DEFAULT);
  uart_ll_set_hw_flow_ctrl(uart->dev, UART_HW_FLOWCTRL_DISABLE, UART_HW_FLOW_CONTROL_RX_THRS_DEFAULT);
  uart_ll_set_data_bit_num(uart->dev, options.data_bits);
  uart_ll_set_mode(uart->dev, UART_MODE_DEFAULT);
  uart_ll_inverse_signal(uart->dev,
      (options.rx_inverted ? UART_SIGNAL_RXD_INV : 0)
    | (options.tx_inverted ? UART_SIGNAL_TXD_INV : 0)
  );

  taskEXIT_CRITICAL(&uart->mux);

  return 0;
}

void uart_dev_teardown(struct uart *uart)
{
  taskENTER_CRITICAL(&uart->mux);

  periph_module_disable(uart_periph_module[uart->port & UART_PORT_MASK]);

  taskEXIT_CRITICAL(&uart->mux);

  uart->dev = NULL;

  if (uart->dev_mutex) {
    LOG_DEBUG("give dev_mutex=%p", uart->dev_mutex);

    xSemaphoreGive(uart->dev_mutex);

    uart->dev_mutex = NULL;
  }
}
