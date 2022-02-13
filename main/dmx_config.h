#pragma once

#include "dmx.h"

#include <config.h>
#include <uart.h>

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
