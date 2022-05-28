#pragma once

#include <leds.h>
#include "../protocol.h"
#include "../interface.h"
#include "../interfaces/i2s.h"
#include "../limit.h"

#define LEDS_PROTOCOL_SK6812_GRBW_INTERFACE_I2S_MODE LEDS_INTERFACE_I2S_MODE_32BIT_1U250_4X4_80UL

#define SK6812_GRBW_PIXEL_POWER_DIVISOR (4 * 255) // one pixel at full brightness

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

static inline unsigned sk6812grbw_pixel_power(const union sk6812grbw_pixel pixel)
{
  return pixel.w + pixel.b + pixel.r + pixel.g;
}

static inline union sk6812grbw_pixel sk6812grbw_pixel_limit(const union sk6812grbw_pixel pixel, unsigned index, const struct leds_limit *limit)
{
  return (union sk6812grbw_pixel) {
    .w  = leds_limit_uint8(limit, index, pixel.w),
    .b  = leds_limit_uint8(limit, index, pixel.b),
    .r  = leds_limit_uint8(limit, index, pixel.r),
    .g  = leds_limit_uint8(limit, index, pixel.g),
  };
}

struct leds_protocol_sk6812grbw {
  union sk6812grbw_pixel *pixels;
  unsigned count;
};

int leds_protocol_sk6812grbw_init(struct leds_protocol_sk6812grbw *protocol, union leds_interface_state *interface, const struct leds_options *options);
int leds_protocol_sk6812grbw_tx(struct leds_protocol_sk6812grbw *protocol, union leds_interface_state *interface, const struct leds_options *options, const struct leds_limit *limit);

void leds_protocol_sk6812grbw_set(struct leds_protocol_sk6812grbw *protocol, unsigned index, struct leds_color color);
void leds_protocol_sk6812grbw_set_all(struct leds_protocol_sk6812grbw *protocol, struct leds_color color);

unsigned leds_protocol_sk6812grbw_count_active(struct leds_protocol_sk6812grbw *protocol);
unsigned leds_protocol_sk6812grbw_count_power(struct leds_protocol_sk6812grbw *protocol, unsigned index, unsigned count);
