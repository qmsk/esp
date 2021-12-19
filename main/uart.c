#include "uart.h"

#include <logging.h>

#include <FreeRTOS.h>
#include <driver/uart.h>
#include <esp_log.h>
#include <esp_vfs_dev.h>

#include <stdio.h>

#include <sdkconfig.h>

#define UART_RX_LINE_ENDINGS_MODE ESP_LINE_ENDINGS_CRLF // convert CRLF to LF
#define UART_TX_LINE_ENDINGS_MODE ESP_LINE_ENDINGS_CRLF // convert LF to CRLF

#define UART_RX_BUFFER_SIZE 256
#define UART_TX_BUFFER_SIZE 1024

int init_uart()
{
  uart_port_t uart_port = CONFIG_ESP_CONSOLE_UART_NUM;
  uart_config_t uart_config = {
    .baud_rate  = CONFIG_ESP_CONSOLE_UART_BAUDRATE,
    .data_bits  = UART_DATA_8_BITS,
    .parity     = UART_PARITY_DISABLE,
    .stop_bits  = UART_STOP_BITS_1,
  };
  esp_err_t err;

  LOG_INFO("port=%d baud_rate=%d, data_bits=%#x parity=%#x stop_bits=%#x",
    uart_port,
    uart_config.baud_rate,
    uart_config.data_bits,
    uart_config.parity,
    uart_config.stop_bits
  );

  if ((err = uart_param_config(uart_port, &uart_config))) {
    LOG_ERROR("uart_param_config port=%d", uart_port);
    return -1;
  }

  // XXX: sets U0TXD pin mux to U0TXD if using bootloader CONFIG_ESP_UART0_SWAP_IO
  if ((err = uart_driver_install(uart_port, UART_RX_BUFFER_SIZE, UART_TX_BUFFER_SIZE, 0, NULL, 0))) {
    LOG_ERROR("uart_driver_install");
    return -1;
  }

  // configure stdio to use buffered/interrupt-driven uart driver with CRLF <-> LF conversion
  esp_vfs_dev_uart_use_driver(CONFIG_ESP_CONSOLE_UART_NUM);
  esp_vfs_dev_uart_set_rx_line_endings(UART_RX_LINE_ENDINGS_MODE);
  esp_vfs_dev_uart_set_tx_line_endings(UART_TX_LINE_ENDINGS_MODE);

  return 0;
}
