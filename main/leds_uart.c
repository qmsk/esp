#include "leds.h"
#include "leds_config.h"
#include "leds_state.h"

#include <logging.h>

#if CONFIG_LEDS_UART_ENABLED
  // config
  struct leds_uart_config leds_uart_config = {

  };

  const struct config_enum leds_uart_port_enum[] = {
    { "",              -1       },
  # if defined(UART_0) && CONFIG_ESP_CONSOLE_UART_NUM != 0
    { "UART0",         UART_0   },
  # endif
  # if defined(UART_1) && CONFIG_ESP_CONSOLE_UART_NUM != 1
    { "UART1",         UART_1   },
  # endif
  # if defined(UART_2) && CONFIG_ESP_CONSOLE_UART_NUM != 2
    { "UART2",         UART_2   },
  # endif
    {},
  };

  const struct configtab leds_uart_configtab[] = {
    { CONFIG_TYPE_ENUM, "port",
      .description = "Select host peripherial for UART interface.",
      .enum_type = { .value = &leds_uart_config.port, .values = leds_uart_port_enum, .default_value = -1 },
    },
    {},
  };

  // state
  struct uart *leds_uart;

  int init_leds_uart()
  {
    const struct leds_uart_config *uart_config = &leds_uart_config;
    bool enabled = false;
    int err;

    if (uart_config->port < 0) {
      LOG_INFO("leds: uart disabled");
      return 0;
    }

    for (int i = 0; i < LEDS_COUNT; i++)
    {
      const struct leds_config *config = &leds_configs[i];

      if (!config->enabled) {
        continue;
      }

      if (config->interface != LEDS_INTERFACE_UART) {
        continue;
      }

      LOG_INFO("leds%d: uart%d configured", i, uart_config->port);
      enabled = true;
    }

    if (!enabled) {
      LOG_INFO("leds: uart%d not configured", uart_config->port);
      return 0;
    }

    if ((err = uart_new(&leds_uart, uart_config->port, LEDS_UART_RX_BUFFER_SIZE, LEDS_UART_TX_BUFFER_SIZE))) {
      LOG_ERROR("uart_new(port=%d)", uart_config->port);
      return err;
    }

    return 0;
  }

  int config_leds_uart(struct leds_state *state, const struct leds_config *config, struct leds_options *options)
  {
    if (!leds_uart) {
      LOG_ERROR("leds%d: uart not initialized", state->index + 1);
      return -1;
    }

    options->uart = leds_uart;
    // TODO: uart_pin_mutex?

    LOG_INFO("leds%d: uart=%p", state->index + 1, options->uart);

    return 0;
  }
#endif
