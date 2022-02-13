#include "dmx.h"
#include "dmx_config.h"
#include "dmx_state.h"

#include <dmx_uart.h>
#include <logging.h>

struct uart *dmx_uart;

int init_dmx_uart()
{
  const struct dmx_uart_config *uart_config = &dmx_uart_config;
  bool input_enabled = false;
  bool outputs_enabled = false;
  int err;

  if (uart_config->port < 0) {
    LOG_INFO("dmx-uart: uart disabled");
    return 0;
  }

  if (dmx_input_config.enabled) {
    input_enabled = true;

    LOG_INFO("dmx-input: uart%d configured", uart_config->port);
  }

  for (int i = 0; i < DMX_OUTPUT_COUNT; i++) {
    const struct dmx_output_config *output_config = &dmx_output_configs[i];

    if (!output_config->enabled) {
      continue;
    };

    LOG_INFO("dmx-output%d: uart%d configured", i+1, uart_config->port);

    outputs_enabled = true;
  }

  if (!input_enabled && !outputs_enabled) {
    LOG_INFO("dmx: uart%d not configured", uart_config->port);
    return 0;
  }

  // setup
  size_t rx_buffer_size = input_enabled ? DMX_UART_RX_BUFFER_SIZE : 0;
  size_t tx_buffer_size = outputs_enabled ? DMX_UART_TX_BUFFER_SIZE : 0;

  LOG_INFO("dmx: uart%d configured with rx=%u tx=%u buffers", uart_config->port, rx_buffer_size, tx_buffer_size);

  if ((err = uart_new(&dmx_uart, uart_config->port, rx_buffer_size, tx_buffer_size))) {
    LOG_ERROR("uart_new(port=%d)", uart_config->port);
    return err;
  }

  return 0;
}
