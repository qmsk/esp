#pragma once

#include <leds.h>
#include "../protocol.h"
#include "../limit.h"

// 24 bits per pixel, 2 bits per I2S byte
#define WS2812B_I2S_SIZE (3 * 4)

#define WS2812B_PIXEL_TOTAL_DIVISOR (3 * 255) // one pixel at full brightness

static inline size_t leds_protocol_ws2812b_i2s_buffer_size(unsigned count)
{
  return WS2812B_I2S_SIZE * count;
}

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

int leds_protocol_ws2812b_init(union leds_interface_state *interface, struct leds_protocol_ws2812b *protocol, const struct leds_options *options);
int leds_protocol_ws2812b_tx(union leds_interface_state *interface, struct leds_protocol_ws2812b *protocol, const struct leds_options *options);

void leds_protocol_ws2812b_set(struct leds_protocol_ws2812b *protocol, unsigned index, struct leds_color color);
void leds_protocol_ws2812b_set_all(struct leds_protocol_ws2812b *protocol, struct leds_color color);

unsigned leds_protocol_ws2812b_count_active(struct leds_protocol_ws2812b *protocol);
unsigned leds_protocol_ws2812b_count_total(struct leds_protocol_ws2812b *protocol);
