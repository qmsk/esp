#pragma once

#include <leds.h>
#include "../protocol.h"

#define SK9822_GLOBAL_BYTE(brightness) (0xE0 | ((brightness) >> 3))
#define SK9822_BRIGHTNESS(global) (global & 0x1F) // 0..31

union __attribute__((packed)) sk9822_pixel {
  struct {
    uint8_t r, g, b;
    uint8_t global;
  };

  // aligned with 0xXXBBGGRR on little-endian architectures
  uint32_t xbgr;
};

static inline union sk9822_pixel sk9822_pixel(struct leds_color color, unsigned index, const struct leds_limit *limit)
{
  // TODO: use driving current instead of PWM for power limit?
  return (union sk9822_pixel) {
    .global = SK9822_GLOBAL_BYTE(color.dimmer),
    .b      = leds_limit_uint8(limit, index, color.b),
    .g      = leds_limit_uint8(limit, index, color.g),
    .r      = leds_limit_uint8(limit, index, color.r),
  };
}

extern struct leds_protocol_type leds_protocol_sk9822;

#if CONFIG_LEDS_I2S_ENABLED
  #include "../interfaces/i2s.h"

  #define LEDS_PROTOCOL_SK9822_INTERFACE_I2S_MODE LEDS_INTERFACE_I2S_MODE_32BIT_BCK

  void leds_protocol_sk9822_i2s_out(uint32_t buf[1], const struct leds_color *pixels, unsigned index, const struct leds_limit *limit);
#endif
