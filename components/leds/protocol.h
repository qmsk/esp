#pragma once

#include <leds.h>

#include "interface.h"
#include "limit.h"

struct leds_protocol_type {
#if CONFIG_LEDS_I2S_ENABLED
  enum leds_interface_i2s_mode i2s_interface_mode;
#endif
#if CONFIG_LEDS_SPI_ENABLED
  enum leds_interface_spi_mode spi_interface_mode;
#endif
#if CONFIG_LEDS_UART_ENABLED
  enum leds_interface_uart_mode uart_interface_mode;
#endif
  enum leds_parameter_type parameter_type;
  enum leds_power_mode power_mode;

  int (*init)(union leds_interface_state *interface, const struct leds_options *options);
  int (*tx)(union leds_interface_state *interface, const struct leds_options *options, const struct leds_color *pixels, const struct leds_limit *limit);
};

extern const struct leds_protocol_type *leds_protocol_types[LEDS_PROTOCOLS_COUNT];

const struct leds_protocol_type *leds_protocol_type(enum leds_protocol protocol);
