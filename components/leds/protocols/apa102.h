#pragma once

#include <leds.h>
#include "../protocol.h"
#include "../interface.h"
#include "../limit.h"

#define APA102_GLOBAL_BYTE(brightness) (0xE0 | ((brightness) >> 3))
#define APA102_BRIGHTNESS(global) ((global & 0x1F) << 3) // 0..255

union __attribute__((packed)) apa102_pixel {
  struct {
    uint8_t global;
    uint8_t b, g, r;
  };

  // aligned with 0xRRGGBBXX -> XX BB GG RR on little-endian architectures
  uint32_t rgbx;
};

static inline union apa102_pixel apa102_pixel(struct leds_color color, unsigned index, const struct leds_limit *limit)
{
  return (union apa102_pixel) {
    .global = APA102_GLOBAL_BYTE(color.dimmer),
    .b      = leds_limit_uint8(limit, index, color.b),
    .g      = leds_limit_uint8(limit, index, color.g),
    .r      = leds_limit_uint8(limit, index, color.r),
  };
}

extern struct leds_protocol_type leds_protocol_apa102;

#if CONFIG_LEDS_I2S_ENABLED
  #include "../interfaces/i2s.h"

  #define LEDS_PROTOCOL_APA102_INTERFACE_SPI_MODE LEDS_INTERFACE_SPI_MODE3_32BIT

  void leds_protocol_apa102_spi_out(uint32_t buf[1], const struct leds_color *pixels, unsigned index, const struct leds_limit *limit);
#endif
