#pragma once

#include <leds.h>
#include "../protocol.h"

union ws2811_pixel {
  struct {
    uint8_t b, g, r;
  };

  // aligned with 0xXXRRGGBB on little-endian architectures
  uint32_t _rgb;
};

static inline union ws2811_pixel ws2811_pixel(struct leds_color color, unsigned index, const struct leds_limit *limit)
{
  return (union ws2811_pixel) {
    .b  = leds_limit_uint8(limit, index, color.b),
    .g  = leds_limit_uint8(limit, index, color.g),
    .r  = leds_limit_uint8(limit, index, color.r),
  };
}

extern struct leds_protocol_type leds_protocol_ws2811;

#if CONFIG_LEDS_I2S_ENABLED
  #include "../interfaces/i2s.h"

  #define LEDS_PROTOCOL_WS2811_INTERFACE_I2S_MODE LEDS_INTERFACE_I2S_MODE_32BIT_1U250_4X4_80UL

  void leds_protocol_ws2811_i2s_out(uint16_t buf[6], const struct leds_color *pixels, unsigned index, const struct leds_limit *limit);
#endif

#if CONFIG_LEDS_UART_ENABLED
  #include "../interfaces/uart.h"

  #define LEDS_PROTOCOL_WS2811_INTERFACE_UART_MODE LEDS_INTERFACE_UART_MODE_24B2I8_0U25_50U

  void leds_protocol_ws2811_uart_out(uint16_t buf[6], const struct leds_color *pixels, unsigned index, const struct leds_limit *limit);
#endif
