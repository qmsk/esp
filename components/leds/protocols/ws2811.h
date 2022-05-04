#pragma once

#include <leds.h>
#include "../protocol.h"
#include "../interface.h"
#include "../interfaces/i2s.h"
#include "../limit.h"

#define LEDS_PROTOCOL_WS2811_INTERFACE_I2S_MODE LEDS_INTERFACE_I2S_MODE_24BIT_1U250_4X4_80UL

#define WS2811_PIXEL_TOTAL_DIVISOR (3 * 255) // one pixel at full brightness

union ws2811_pixel {
  struct {
    uint8_t b, g, r;
  };

  // aligned with 0xXXRRGGBB on little-endian architectures
  uint32_t _rgb;
};

static inline bool ws2811_pixel_active(const union ws2811_pixel pixel)
{
  return pixel.b || pixel.g || pixel.r;
}

static inline unsigned ws2811_pixel_total(const union ws2811_pixel pixel)
{
  return pixel.b + pixel.r + pixel.g;
}

static inline union ws2811_pixel ws2811_pixel_limit(const union ws2811_pixel pixel, struct leds_limit limit)
{
  return (union ws2811_pixel) {
    .b  = leds_limit_uint8(limit, pixel.b),
    .g  = leds_limit_uint8(limit, pixel.g),
    .r  = leds_limit_uint8(limit, pixel.r),
  };
}

struct leds_protocol_ws2811 {
  union ws2811_pixel *pixels;
  unsigned count;
};

int leds_protocol_ws2811_init(struct leds_protocol_ws2811 *protocol, union leds_interface_state *interface, const struct leds_options *options);
int leds_protocol_ws2811_tx(struct leds_protocol_ws2811 *protocol, union leds_interface_state *interface, const struct leds_options *options, struct leds_limit limit);

void leds_protocol_ws2811_set(struct leds_protocol_ws2811 *protocol, unsigned index, struct leds_color color);
void leds_protocol_ws2811_set_all(struct leds_protocol_ws2811 *protocol, struct leds_color color);

unsigned leds_protocol_ws2811_count_active(struct leds_protocol_ws2811 *protocol);
unsigned leds_protocol_ws2811_count_total(struct leds_protocol_ws2811 *protocol);
