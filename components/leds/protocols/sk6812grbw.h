#pragma once

#include <leds.h>
#include "../protocol.h"
#include "../limit.h"

// 32 bits per pixel, 2 bits per I2S byte
#define SK6812_GRBW_I2S_SIZE (4 * 4)

#define SK6812_GRBW_PIXEL_TOTAL_DIVISOR (4 * 255) // one pixel at full brightness

static inline size_t leds_protocol_sk6812grbw_i2s_buffer_size(unsigned count)
{
  return SK6812_GRBW_I2S_SIZE * count;
}

union sk6812grbw_pixel {
  struct {
    uint8_t w, b, r, g;
  };

  // aligned with 0xGGRRBBWW on little-endian architectures
  uint32_t grbw;
};

static inline bool sk6812grbw_pixel_active(const union sk6812grbw_pixel pixel)
{
  return pixel.w || pixel.b || pixel.r || pixel.g;
}

static inline unsigned sk6812grbw_pixel_total(const union sk6812grbw_pixel pixel)
{
  return pixel.w + pixel.b + pixel.r + pixel.g;
}

static inline union sk6812grbw_pixel sk6812grbw_pixel_limit(const union sk6812grbw_pixel pixel, struct leds_limit limit)
{
  return (union sk6812grbw_pixel) {
    .w  = leds_limit_uint8(limit, pixel.w),
    .b  = leds_limit_uint8(limit, pixel.b),
    .r  = leds_limit_uint8(limit, pixel.r),
    .g  = leds_limit_uint8(limit, pixel.g),
  };
}

struct leds_protocol_sk6812grbw {
  union sk6812grbw_pixel *pixels;
  unsigned count;
};

int leds_protocol_sk6812grbw_init(union leds_interface_state *interface, struct leds_protocol_sk6812grbw *protocol, const struct leds_options *options);
int leds_protocol_sk6812grbw_tx(union leds_interface_state *interface, struct leds_protocol_sk6812grbw *protocol, const struct leds_options *options);

void leds_protocol_sk6812grbw_set(struct leds_protocol_sk6812grbw *protocol, unsigned index, struct leds_color color);
void leds_protocol_sk6812grbw_set_all(struct leds_protocol_sk6812grbw *protocol, struct leds_color color);

unsigned leds_protocol_sk6812grbw_count_active(struct leds_protocol_sk6812grbw *protocol);
unsigned leds_protocol_sk6812grbw_count_total(struct leds_protocol_sk6812grbw *protocol);
