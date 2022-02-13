#pragma once

#include "dmx.h"

#include <config.h>
#include <uart.h>

// buffer RX FIFO until line idle for this many ~8-bit periods
// this must be short enough to trigger in the MTBP, or the final bytes in the packet will be lost...
#define DMX_UART_MTBP_UNIT (8 * (1000000 / 250000))
#define DMX_UART_MTBP_MIN (4 * DMX_UART_MTBP_UNIT)
#define DMX_UART_MTBP_MAX (UART_RX_TIMEOUT_MAX * DMX_UART_MTBP_UNIT)

enum dmx_gpio_mode {
  DMX_GPIO_MODE_DISABLED  = -1,
  DMX_GPIO_MODE_LOW       = 0,
  DMX_GPIO_MODE_HIGH      = 1,
};

extern const struct config_enum dmx_gpio_mode_enum[];

struct dmx_uart_config {
  int port;
};

struct dmx_input_config {
  bool enabled;

  uint16_t mtbp_min;

  bool artnet_enabled;
  uint16_t artnet_universe;
};

struct dmx_output_config {
  bool enabled;

  uint16_t gpio_pin;
  int gpio_mode;

  bool artnet_enabled;
  uint16_t artnet_universe;
};

extern struct dmx_uart_config dmx_uart_config;
extern struct dmx_input_config dmx_input_config;
extern struct dmx_output_config dmx_output_configs[DMX_OUTPUT_COUNT];
