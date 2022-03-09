#include "dmx.h"
#include "dmx_config.h"
#include "dmx_state.h"
#include "dev_mutex.h"

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
  uart_port_t uart_port = uart_config->port;
  size_t rx_buffer_size = input_enabled ? DMX_UART_RX_BUFFER_SIZE : 0;
  size_t tx_buffer_size = outputs_enabled ? DMX_UART_TX_BUFFER_SIZE : 0;

  // TODO: fully configurable IO pins?
#ifdef UART_TXONLY_BIT
    if (!input_enabled) {
      uart_port |= UART_TXONLY_BIT;
    }
#endif
#ifdef UART_RXONLY_BIT
  if (!outputs_enabled) {
    uart_port |= UART_RXONLY_BIT;
  }
#endif

  LOG_INFO("dmx: uart%d configured with port=%04x rx_buffer_size=%u tx_buffer_size=%u", uart_config->port, uart_port, rx_buffer_size, tx_buffer_size);

  if ((err = uart_new(&dmx_uart, uart_port, rx_buffer_size, tx_buffer_size))) {
    LOG_ERROR("uart_new(port=%d)", uart_port);
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

  switch(dmx_uart_config.port & UART_PORT_MASK) {
    case UART_0:
      options.dev_mutex = dev_mutex[DEV_MUTEX_UART0];
      break;
  }

  if (!options.dev_mutex) {
    LOG_INFO("UART dev_mutex is not set, assume UART is available");
  } else if (uxSemaphoreGetCount(options.dev_mutex) > 0) {
    LOG_INFO("UART dev_mutex=%p is available", options.dev_mutex);
  } else {
    LOG_WARN("UART dev_mutex=%p will wait for UART to become available...", options.dev_mutex);
  }

  if (dmx_input_config.enabled && dmx_input_config.mtbp_min) {
    LOG_INFO("dmx-input: use uart mtbp_min=%u", dmx_input_config.mtbp_min);

    options.mtbp_min = dmx_input_config.mtbp_min;
  }

  LOG_INFO("setup mtbp_min=%d dev_mutex=%p", options.mtbp_min, options.dev_mutex);

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
