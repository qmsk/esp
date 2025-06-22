#pragma once

#include <leds.h>
#include "../protocol.h"

extern struct leds_protocol_type leds_protocol_sk6812grbw;

#if CONFIG_LEDS_I2S_ENABLED
  #include "../interfaces/i2s.h"

  void leds_protocol_sk6812grbw_i2s_out(uint16_t buf[8], const struct leds_color *pixels, unsigned index, const struct leds_limit *limit);
#endif

#if CONFIG_LEDS_UART_ENABLED
  #include "../interfaces/uart.h"

  void leds_protocol_sk6812grbw_uart_out(uint16_t buf[8], const struct leds_color *pixels, unsigned index, const struct leds_limit *limit);
#endif
