#pragma once

#include <leds.h>

union leds_interface_state *interface;

// 24 bits per pixel, 2 bits per I2S byte
#define WS2812B_I2S_SIZE (3 * 4)

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

struct leds_protocol_ws2812b {
  union ws2812b_pixel *pixels;
};

int leds_protocol_ws2812b_init(union leds_interface_state *interface, struct leds_protocol_ws2812b *protocol, const struct leds_options *options);
int leds_protocol_ws2812b_tx(union leds_interface_state *interface, struct leds_protocol_ws2812b *protocol, const struct leds_options *options);

void leds_protocol_ws2812b_set_frame(struct leds_protocol_ws2812b *protocol, unsigned index, struct leds_color color);
void leds_protocol_ws2812b_set_frames(struct leds_protocol_ws2812b *protocol, unsigned count, struct leds_color color);

unsigned leds_protocol_ws2812b_count_active(struct leds_protocol_ws2812b *protocol, unsigned count);
