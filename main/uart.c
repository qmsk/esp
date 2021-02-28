#include "uart.h"

#include <logging.h>

#include <FreeRTOS.h>
#include <driver/uart.h>
#include <esp_vfs_dev.h>

#define UART_RX_BUFFER_SIZE 256
#define UART_TX_BUFFER_SIZE 1024

static int putchar_stderr(int c)
{
  return fputc(c, stderr);
}

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

  if ((err = uart_driver_install(uart_port, UART_RX_BUFFER_SIZE, UART_TX_BUFFER_SIZE, 0, NULL, 0))) {
    LOG_ERROR("uart_driver_install");
    return -1;
  }

  // configure stdio to use buffered/interrupt-driven uart driver
  esp_vfs_dev_uart_use_driver(CONFIG_ESP_CONSOLE_UART_NUM);

  // unbuffered logging via stderr
  setvbuf(stderr, NULL, _IONBF, 0);
  esp_log_set_putchar(putchar_stderr);

  return 0;
}
