#pragma once

#include <leds.h>
#include "../protocol.h"

union sk6812grbw_pixel {
  struct {
    uint8_t w, b, r, g;
  };

  // aligned with 0xGGRRBBWW on little-endian architectures
  uint32_t grbw;
};

static inline union sk6812grbw_pixel sk6812grbw_pixel(struct leds_color color, unsigned index, const struct leds_limit *limit)
{
  return (union sk6812grbw_pixel) {
    .w  = leds_limit_uint8(limit, index, color.white),
    .b  = leds_limit_uint8(limit, index, color.b),
    .r  = leds_limit_uint8(limit, index, color.r),
    .g  = leds_limit_uint8(limit, index, color.g),
  };
}

extern struct leds_protocol_type leds_protocol_sk6812grbw;

#if CONFIG_LEDS_I2S_ENABLED
  #include "../interfaces/i2s.h"

  void leds_protocol_sk6812grbw_i2s_out(uint16_t buf[8], const struct leds_color *pixels, unsigned index, const struct leds_limit *limit);
#endif

#if CONFIG_LEDS_UART_ENABLED
  #include "../interfaces/uart.h"

  void leds_protocol_sk6812grbw_uart_out(uint16_t buf[8], const struct leds_color *pixels, unsigned index, const struct leds_limit *limit);
#endif
