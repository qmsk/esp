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

int start_dmx_uart()
{
  struct dmx_uart_options options = {
    .mtbp_min = DMX_UART_MTBP_MIN,
  };
  int err;

  if (!dmx_uart) {
    LOG_INFO("disabled");
    return 1;
  }

  if (dmx_input_config.enabled && dmx_input_config.mtbp_min) {
    LOG_INFO("dmx-input: use uart mtbp_min=%u", dmx_input_config.mtbp_min);

    options.mtbp_min = dmx_input_config.mtbp_min;
  }

  LOG_INFO("setup mtbp_min=%d", options.mtbp_min);

  if ((err = dmx_uart_setup(dmx_uart, options))) {
    LOG_ERROR("dmx_uart_setup");
    return err;
  }

  return 0;
}

int open_dmx_input_uart(struct dmx_input *input)
{
  if (!dmx_uart) {
    LOG_INFO("disabled");
    return 1;
  }

  return dmx_input_open(input, dmx_uart);
}

int open_dmx_output_uart(struct dmx_output *output)
{
  if (!dmx_uart) {
    LOG_INFO("disabled");
    return 1;
  }

  return dmx_output_open(output, dmx_uart);
}
