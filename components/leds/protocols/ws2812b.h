#pragma once

#include <leds.h>
#include "../protocol.h"

union ws2812b_pixel {
  struct {
    uint8_t b, r, g;
  };

  // aligned with 0xXXGGRRBB on little-endian architectures
  uint32_t _grb;
};

static inline union ws2812b_pixel ws2812b_pixel(struct leds_color color, unsigned index, const struct leds_limit *limit)
{
  return (union ws2812b_pixel) {
    .b  = leds_limit_uint8(limit, index, color.b),
    .r  = leds_limit_uint8(limit, index, color.r),
    .g  = leds_limit_uint8(limit, index, color.g),
  };
}

extern struct leds_protocol_type leds_protocol_ws2812b;

#if CONFIG_LEDS_I2S_ENABLED
  #include "../interfaces/i2s.h"

  void leds_protocol_ws2812b_i2s_out(uint16_t buf[6], const struct leds_color *pixels, unsigned index, const struct leds_limit *limit);
#endif

#if CONFIG_LEDS_UART_ENABLED
  #include "../interfaces/uart.h"

  void leds_protocol_ws2812b_uart_out(uint16_t buf[4], const struct leds_color *pixels, unsigned index, const struct leds_limit *limit);
#endif
