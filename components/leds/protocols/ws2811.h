#pragma once

#include <leds.h>
#include "../protocol.h"

extern struct leds_protocol_type leds_protocol_ws2811_rgb;
extern struct leds_protocol_type leds_protocol_ws2811_grb;

#if CONFIG_LEDS_I2S_ENABLED
  #include "../interfaces/i2s.h"

  void leds_protocol_ws2811_i2s_rgb_out(uint16_t buf[6], const struct leds_color *pixels, unsigned index, const struct leds_limit *limit);
  void leds_protocol_ws2811_i2s_grb_out(uint16_t buf[6], const struct leds_color *pixels, unsigned index, const struct leds_limit *limit);
#endif

#if CONFIG_LEDS_UART_ENABLED
  #include "../interfaces/uart.h"

  void leds_protocol_ws2811_uart_rgb_out(uint16_t buf[6], const struct leds_color *pixels, unsigned index, const struct leds_limit *limit);
#endif
