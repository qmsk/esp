#pragma once

#include <leds.h>
#include "../protocol.h"
#include "../interface.h"
#include "../interfaces/i2s.h"
#include "../limit.h"

#define LEDS_PROTOCOL_WS2812B_INTERFACE_I2S_MODE LEDS_INTERFACE_I2S_MODE_24BIT_1U250_4X4_80UL

#define WS2812B_PIXEL_TOTAL_DIVISOR (3 * 255) // one pixel at full brightness

union ws2812b_pixel {
  struct {
    uint8_t b, r, g;
  };

  // aligned with 0xXXGGRRBB on little-endian architectures
  uint32_t _grb;
};


static inline bool ws2812b_pixel_active(const union ws2812b_pixel pixel)
{
  return pixel.b || pixel.r || pixel.g;
}

static inline unsigned ws2812b_pixel_total(const union ws2812b_pixel pixel)
{
  return pixel.b + pixel.r + pixel.g;
}

static inline union ws2812b_pixel ws2812b_pixel_limit(const union ws2812b_pixel pixel, struct leds_limit limit)
{
  return (union ws2812b_pixel) {
    .b  = leds_limit_uint8(limit, pixel.b),
    .r  = leds_limit_uint8(limit, pixel.r),
    .g  = leds_limit_uint8(limit, pixel.g),
  };
}

struct leds_protocol_ws2812b {
  union ws2812b_pixel *pixels;
  unsigned count;
};

int leds_protocol_ws2812b_init(struct leds_protocol_ws2812b *protocol, union leds_interface_state *interface, const struct leds_options *options);
int leds_protocol_ws2812b_tx(struct leds_protocol_ws2812b *protocol, union leds_interface_state *interface, const struct leds_options *options, struct leds_limit limit);

void leds_protocol_ws2812b_set(struct leds_protocol_ws2812b *protocol, unsigned index, struct leds_color color);
void leds_protocol_ws2812b_set_all(struct leds_protocol_ws2812b *protocol, struct leds_color color);

unsigned leds_protocol_ws2812b_count_active(struct leds_protocol_ws2812b *protocol);
unsigned leds_protocol_ws2812b_count_total(struct leds_protocol_ws2812b *protocol);
