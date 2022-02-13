#pragma once

#include <leds.h>

union leds_interface_state *interface;

// 24 bits per pixel, 2 bits per I2S byte
#define WS2811_I2S_SIZE (3 * 4)

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

struct leds_protocol_ws2811 {
  union ws2811_pixel *pixels;
};

int leds_protocol_ws2811_init(union leds_interface_state *interface, struct leds_protocol_ws2811 *protocol, const struct leds_options *options);
int leds_protocol_ws2811_tx(union leds_interface_state *interface, struct leds_protocol_ws2811 *protocol, const struct leds_options *options);

void leds_protocol_ws2811_set_frame(struct leds_protocol_ws2811 *protocol, unsigned index, struct spi_led_color color);
void leds_protocol_ws2811_set_frames(struct leds_protocol_ws2811 *protocol, unsigned count, struct spi_led_color color);

unsigned leds_protocol_ws2811_count_active(struct leds_protocol_ws2811 *protocol, unsigned count);
