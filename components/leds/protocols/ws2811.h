#pragma once

#include <leds.h>
#include "../protocol.h"
#include "../limit.h"

// 24 bits per pixel, 2 bits per I2S byte
#define WS2811_I2S_SIZE (3 * 4)

#define WS2811_PIXEL_TOTAL_DIVISOR (3 * 255) // one pixel at full brightness

static inline size_t leds_protocol_ws2811_i2s_buffer_size(unsigned count)
{
  return WS2811_I2S_SIZE * count;
}

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
};

int leds_protocol_ws2811_init(union leds_interface_state *interface, struct leds_protocol_ws2811 *protocol, const struct leds_options *options);
int leds_protocol_ws2811_tx(union leds_interface_state *interface, struct leds_protocol_ws2811 *protocol, const struct leds_options *options);

void leds_protocol_ws2811_set_frame(struct leds_protocol_ws2811 *protocol, unsigned index, struct leds_color color);
void leds_protocol_ws2811_set_frames(struct leds_protocol_ws2811 *protocol, unsigned count, struct leds_color color);

unsigned leds_protocol_ws2811_count_active(struct leds_protocol_ws2811 *protocol, unsigned count);
unsigned leds_protocol_ws2811_count_total(struct leds_protocol_ws2811 *protocol, unsigned count);
