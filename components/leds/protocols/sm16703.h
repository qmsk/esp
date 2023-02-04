#pragma once

#include <leds.h>
#include "../protocol.h"

union sm16703_pixel {
  struct {
    uint8_t b, g, r;
  };

  // aligned with 0xXXRRGGBB on little-endian architectures
  uint32_t _rgb;
};

static inline union sm16703_pixel sm16703_pixel(struct leds_color color, unsigned index, const struct leds_limit *limit)
{
  return (union sm16703_pixel) {
    .b  = leds_limit_uint8(limit, index, color.b),
    .g  = leds_limit_uint8(limit, index, color.g),
    .r  = leds_limit_uint8(limit, index, color.r),
  };
}

extern struct leds_protocol_type leds_protocol_sm16703;

#if CONFIG_LEDS_I2S_ENABLED
  #include "../interfaces/i2s.h"

  void leds_protocol_sm16703_i2s_out(uint16_t buf[6], const struct leds_color *pixels, unsigned index, const struct leds_limit *limit);
#endif
