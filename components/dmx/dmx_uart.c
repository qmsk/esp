#include <dmx_uart.h>

#include <logging.h>

int dmx_uart_setup(struct uart *uart, struct dmx_uart_options options)
{
  struct uart_options uart_options = {
    .baud_rate   = UART_BAUD_250000,
    .data_bits   = UART_DATA_8_BITS,
    .parity_bits = UART_PARITY_DISABLE,
    .stop_bits   = UART_STOP_BITS_2,

    .dev_mutex   = options.dev_mutex,

    .pin_mutex   = options.pin_mutex,
  #ifdef DMX_UART_IO_PINS_SUPPORTED
    .rx_pin      = options.rx_pin,
    .tx_pin      = options.tx_pin,
  #endif
  };
  int err;

  if (options.mtbp_min >= DMX_UART_MTBP_UNIT) {
    // after N frame periods
    uart_options.rx_timeout = options.mtbp_min / DMX_UART_MTBP_UNIT;
  } else {
    // after each frame
    uart_options.rx_buffered = 1;
  }

  LOG_DEBUG("baud_rate=%u, data_bits=%x parity=%x stop_bits=%x : rx_timeout=%u rx_buffered=%u",
    uart_options.baud_rate,
    uart_options.data_bits,
    uart_options.parity_bits,
    uart_options.stop_bits,
    uart_options.rx_timeout,
    uart_options.rx_buffered
  );
#ifdef DMX_UART_IO_PINS_SUPPORTED
  LOG_DEBUG("rx_pin=%d, tx_pin=%d", uart_options.rx_pin, uart_options.tx_pin);
#endif

  if ((err = uart_setup(uart, uart_options))) {
    LOG_ERROR("uart_setup");
    return err;
  }

  return 0;
}
