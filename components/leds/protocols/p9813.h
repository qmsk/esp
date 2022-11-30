#pragma once

#include <leds.h>
#include "../protocol.h"
#include "../limit.h"

#define P9813_CONTROL_BYTE(b, g, r) (0xC0 | ((~(b) & 0xC0) >> 2) | ((~(g) & 0xC0) >> 4) | ((~(r) & 0xC0) >> 6))

union __attribute__((packed)) p9813_pixel {
  struct {
    uint8_t control;
    uint8_t b, g, r;
  };

  // aligned with 0xRRGGBBXX -> XX BB GG RR on little-endian architectures
  uint32_t rgbx;
};

static inline union p9813_pixel p9813_pixel(struct leds_color color, unsigned index, const struct leds_limit *limit)
{
  return (union p9813_pixel) {
    .control = P9813_CONTROL_BYTE(color.b, color.g, color.r),
    .b  = leds_limit_uint8(limit, index, color.b),
    .g  = leds_limit_uint8(limit, index, color.g),
    .r  = leds_limit_uint8(limit, index, color.r),
  };
}

extern struct leds_protocol_type leds_protocol_p9813;

#if CONFIG_LEDS_SPI_ENABLED
  #include "../interfaces/spi.h"

  #define LEDS_PROTOCOL_P9813_INTERFACE_SPI_MODE LEDS_INTERFACE_SPI_MODE0_32BIT

  void leds_protocol_p9813_spi_out(uint32_t buf[1], const struct leds_color *pixels, unsigned index, const struct leds_limit *limit);
#endif
